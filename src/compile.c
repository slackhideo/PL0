
   /*************** compile.c *************/

#include "getSource.h"
#ifndef TBL
#define TBL
#include "table.h"
#endif
#include "codegen.h"

#define MINERROR 3			/* It will execute object codes if the number of errors in the compilation is less than MINERROR. */
#define FIRSTADDR 2			/* The address of the first variable of each block. */

static Token token;				/* The next token */

static void block(int pIndex);	/* It compiles a block */
						/* pIndex is the index of the function name of this block */
static void constDecl();			/* It compiles a constant declaration. */
static void varDecl();				/* It compiles a variable declaration. */
static void funcDecl();			/* It compiles a function declaration. */
static void procDecl();             /* it compiles a procedure declaration */
static void statement();			/* It compiles a statement. */
static void expression();			/* It compiles an expression. */
static void term();				/* It compiles a term of an expression. */
static void factor();				/* It compiles a fcator of an expression. */
static void condition();			/* It compiles a conditional expression. */
static int isStBeginKey(Token t);		/* Is a token t one of starting tokens of statements? */

int compile()
{
	int i;
	printf("; start compilation\n");
	initSource();				/* Initialization for getSource */
	token = nextToken();			/* The first token */
	blockBegin(FIRSTADDR);		/* A new block starts. */
	block(0);					/* The constatnt "0" is a dummy. (The main block does not have any function name.) */
	finalSource();
	i = errorN();				/* The number of error messages */
	if (i!=0)
	  printf("; %d errors\n", i);
/*	listCode();	*/			/* It lists object codes if needed. */
	return i<MINERROR;		/* Is the number of error messages acceptable so as to execute the object code? */
}

void block(int pIndex)		/* pIndex is the index of the function name of this block */
{
	int backP;
	backP = genCodeV(jmp, 0);		/* It generates a jmp to skip internal functions. The address will be adjusted by backpatch. */
	while (1) {				/* It repeatedly compiles declaratins. */
		switch (token.kind){
		case Const:			/* A constant declaration */
			token = nextToken();
			constDecl(); continue;
		case Var:				/* A variable declaration */
			token = nextToken();
			varDecl(); continue;
		case Func:				/* A function declaration */
			token = nextToken();
			funcDecl(); continue;
        case Proc:              /* procedure declaration */
            token = nextToken();
            procDecl(); continue;
		default:				/* Otherwise, it is the end of declarations. */
			break;
		}
		break;
	}			
	backPatch(backP);			/* It adjusts the target address of the jmp to skip internal functions. */
	changeV(pIndex, nextCode());	/* It adjusts the starting address of this function. */
	genCodeV(ict, frameL());		/* A code to occupy the frame of this block on the stack. */
	statement();				/* The main statement of this block */		
	genCodeR();				/* The return code */
	blockEnd();				/* It declares the end of a block to the name table. */
}	

void constDecl()			/* It compiles a constant declaration. */
{
	Token temp;
	while(1){
		if (token.kind==Id){
			setIdKind(constId);				/* It sets the kind of the token for printing. */
			temp = token; 					/* It records the name of the token. */
			token = checkGet(nextToken(), Equal);		/* The next token should be "=". */
			if (token.kind==Num)
				enterTconst(temp.u.id, token.u.value);	/* It records both the constant name and its value in the name table. */
			else
				errorType("number");
			token = nextToken();
		}else
			errorMissingId();
		if (token.kind!=Comma){		/* If the next token is a comma, it will be followed by a constant declaration. */
			if (token.kind==Id){	/* If the next token is a name, it assumes there must be a comma. */
				errorInsert(Comma);
				continue;
			}else
				break;
		}
		token = nextToken();
	}
	token = checkGet(token, Semicolon);		/* It must end with ";". */
}

void varDecl()				/* It compiles a variable declaration. */
{
    char storeId[MAXNAME]; /* stores the ident to put it in the table */
    int arraySize; /* stores the array size */

	while(1){
		if (token.kind==Id){
            cpystr(storeId, token.u.id);
            setIdKind(varId);		/* It sets the kind of the token for printing. */
			token = nextToken();
            if(token.kind == Lbrack) { /* array declaration */
                token = nextToken();
                if(token.kind == Num) {
                    arraySize = token.u.value; /* gets the array size */
                }
                token = checkGet(token, Num); /* it must be a number */
                token = checkGet(token, Rbrack); /* it must be a right square brackets */
                enterTarray(storeId, arraySize); /* puts the name of a variable in the name table */
            }
            else { /* simple variable */
                enterTvar(storeId);		/* It records the name of a variable in the name table whose address will be determined by the name table. */
            }
		}
        else
			errorMissingId();

		if (token.kind!=Comma){		/* If the next token is a commna, it will be followed by a variable declaration. */
			if (token.kind==Id){	/* If the next token is a name, it assumes there must be a comma. */
				errorInsert(Comma);
				continue;
			}
            else
				break;
		}
		token = nextToken();
	}
	token = checkGet(token, Semicolon);		/* It must end with ";". */
}

void funcDecl()			/* It compiles a function declaration. */
{
	int fIndex;
	if (token.kind==Id){
		setIdKind(funcId);				/* It sets the kind of the token for printing. */
		fIndex = enterTfunc(token.u.id, nextCode());		/* It records the function name in the name table. */
				/* It records the address of the next code as the starting address of this function at first. */
		token = checkGet(nextToken(), Lparen);
		blockBegin(FIRSTADDR);	/* The level of parameters is the same as the level of the block of the function. */
		while(1){
			if (token.kind==Id){			/* The function has parameters. */
				setIdKind(parId);		/* It sets the kind of the token for printing. */
				enterTpar(token.u.id);		/* It records the parameters in the name table. */
				token = nextToken();
			}else
				break;
			if (token.kind!=Comma){		/* If the next token is a comma, it will be followed by a parameter. */
				if (token.kind==Id){		/* If the next token is a name, it assumes there must be a comma. */
					errorInsert(Comma);
					continue;
				}else
					break;
			}
			token = nextToken();
		}
		token = checkGet(token, Rparen);		/* It must end with ")". */
		endpar();				/* It declares the end of parameters to the name table. */
		if (token.kind==Semicolon){
			errorDelete();
			token = nextToken();
		}
		block(fIndex);	/* It compiles a block. The parameter is the index of the function. */
		token = checkGet(token, Semicolon);		/* It must end with ";". */
	} else 
		errorMissingId();			/* No function name. */
}

void procDecl() { /* it compiles a procedure declaration */
    int pIndex; /* stores the procedure index in the name table */
    if(token.kind == Id) {
        setIdKind(procId); /* sets the kind of the token for printing */
        pIndex = enterTproc(token.u.id, nextCode()); /* records the procedure in the name table */
        token = checkGet(nextToken(), Lparen); /* it must be a "(" */
        blockBegin(FIRSTADDR); /* changes the level */
        while(1) {
            if(token.kind == Id) { /* procedure has parameters */
                setIdKind(parId); /* sets the kind of the token for printing */
                enterTpar(token.u.id); /* records the parameters in the name table */
                token = nextToken();
            }
            else {
                break;
            }

            if(token.kind != Comma) { /* if the next token is a comma, expect another parameter */
                if(token.kind == Id) { /* if the next token is an ident, assumes there must be a comma */
                    errorInsert(Comma);
                    continue;
                }
                else {
                    break;
                }
            }

            token = nextToken();
        }
        token = checkGet(token, Rparen); /* it must be a ")" */
        endpar(); /* end of parameters */
        if(token.kind == Semicolon) {
            errorDelete();
            token = nextToken();
        }
        block(pIndex); /* compiles a block using the procedure index
                        * in the name table as parameter */
        token = checkGet(token, Semicolon); /* it must end with ";" */
    }
    else {
        errorMissingId(); /* missing procedure ident */
    }
}

void statement()			/* It compiles a statement. */
{
	int tIndex;
	KindT k;
	int backP, backP2;	/* Variables to record addresses of codes whose address parts must be adjusted later */
    int i; /* for counting the argument number */

	while(1) {
		switch (token.kind) {
		case Id:					/* An assignment statement */
			tIndex = searchT(token.u.id, varId);	/* The index of a left-hand variable */
			setIdKind(k=kindT(tIndex));		/* It sets the kind of the left-hand variable for printing. */
			if (k != varId && k != parId && k != arrayId) /* The left-hand variable must be a variable, an array or a parameter. */
				errorType("var/array/par");
            if(k == arrayId) { /* in case of arrays */
                token = checkGet(nextToken(), Lbrack); /* it must be a "[" */
                expression();
                token = checkGet(token, Rbrack); /* it must be a "]" */
                token = checkGet(token, Assign); /* it must be a ":=" */
                expression();
                genCodeT(stt, tIndex); /* stores the right-hand value in the array position
                                        * specified by the expression in the left-hand variable (array) */
            }
            else {
                token = checkGet(nextToken(), Assign);			/* It must be ":=". */
                expression();					/* It compiles an expression. */
                genCodeT(sto, tIndex);	  /* A code to store the right-hand value in the left-hand variable */
            }
			return;
		case If:                           /* if-then-else statement */
			token = nextToken();
			condition();                   /* a conditional expression */
			token = checkGet(token, Then); /* it must be "then". */
			backP = genCodeV(jpc, 0);      /* a conditional jump */
			statement();                   /* a statement just after "then" */
            if(token.kind == Else) {       /* verify if it is an if-then-else */
                token = nextToken();
                backP2 = genCodeV(jmp, 0); /* jump to the end */
                backPatch(backP);          /* adjusts the jpc target address */
                statement();               /* a statement after "else" */
                backPatch(backP2);         /* adjusts the jmp target address */
            }
            else {
                backPatch(backP);          /* adjusts the jpc target address */
            }
			return;
		case Ret:					/* A return statement */
			token = nextToken();
            switch(token.kind) {
                case Plus: /* cases in which an expression is expected */
                case Minus:
                case Id:
                case Num:
                case Lparen:
                    expression(); /* an expression */
                    break;
                default: /* cases in which no return value is expected */
                    break;
            }
			genCodeR();					/* A return code */
			return;
		case Begin:				/* A begin-end statement */
			token = nextToken();
			while(1){
				statement();				/* A statement */
				while(1){
					if (token.kind==Semicolon){		/* If the next token is ";", it will be followed by a statement. */
						token = nextToken();
						break;
					}
					if (token.kind==End){			/* If the next token is "end", it is the end of the begin-end statement. */
						token = nextToken();
						return;
					}
					if (isStBeginKey(token)){		/* If the next token is one of the starting symbols of statements, */
						errorInsert(Semicolon);	 /* It assumes there must be ";". */
						break;
					}
					errorDelete();	/* Otherwise, it declares an error and ignores the token.¡¡*/
					token = nextToken();
				}
			}
		case While:				/* A while-statement */
			token = nextToken();
			backP2 = nextCode();  /* The target address of the jump at the end of the while-statment. */
			condition();				/* A condiional expression */
			token = checkGet(token, Do);	/* It must be "do". */
			backP = genCodeV(jpc, 0);		/* A conditonal jump which jumps when the condition is false */
			statement();				/* A statement (the body of the while-statement) */
			genCodeV(jmp, backP2);		/* A jump to the beginning of the while-statement */
			backPatch(backP);	/* It adjusts the target address of the conditional jump */
			return;
		case Write:			/* A write-statement */
			token = nextToken();
			expression();				/* An expression */
			genCodeO(wrt);				/* A code to write the value of the expression */
			return;
		case WriteLn:			/* A code to write a new line */
			token = nextToken();
			genCodeO(wrl);				/* A code to wirte a new line */
			return;
        case Unless:                       /* unless statement */
            token = nextToken();
            condition();                   /* a condition expression */
            backP = genCodeV(jpc, 0);      /* a conditional jump to statement */
            backP2 = genCodeV(jmp, 0);     /* a jump to skip the statement */
            token = checkGet(token, Then); /* next token must be "then" */
            backPatch(backP);              /* adjusts the jpc target address */
            statement();                   /* a statement */
            backPatch(backP2);             /* adjusts the jmp target address */
            return;
        case Do:                            /* do-while statement */
            token = nextToken();
            backP2 = nextCode();            /* target address of statement */
            statement();
            token = checkGet(token, While); /* next token must be "while" */
            condition();
            backP = genCodeV(jpc, 0);       /* a conditional jump to the end */
            genCodeV(jmp, backP2);          /* a jump to the statement */
            backPatch(backP);               /* adjusts the jpc target address */
            return;
        case Repeat:                        /* repeat-until statement */
            token = nextToken();
            backP = nextCode();             /* target address of statement */
            statement();
            token = checkGet(token, Until); /* next token must be "until" */
            condition();
            genCodeV(jpc, backP);           /* a jump to the statement */
            return;
        case For:                           /* for-do statement */
            token = nextToken();
            if(token.kind == Id) {
                tIndex = searchT(token.u.id, varId); /* the index of the for variable */
                setIdKind(k = kindT(tIndex));        /* sets the kind of the identifier for printing */
                if(k != varId) {                     /* it must be a variable identifier */
                    errorType("var");
                }
                token = checkGet(nextToken(), Assign); /* it must be a ":=" */
                expression();                    /* initial value of the for variable */
                genCodeT(sto, tIndex);           /* stores the initial value into the variable */
                if(token.kind == To) {           /* incrementing for case */
                    token = nextToken();
                    backP = nextCode();          /* target address of the jump at the final */
                    genCodeT(lod, tIndex);       /* loads the "for variable" value */
                    expression();                /* final value of the "for variable" */
                    genCodeO(lseq);              /* executes statement while var <= final value */
                    backP2 = genCodeV(jpc, 0);   /* conditional jump to exit the loop */
                    token = checkGet(token, Do); /* it must be a "Do" */
                    statement();                 /* "real" job executed in the for-do loop */
                    genCodeT(uad, tIndex);       /* increment the "for variable" by one */
                }
                else if(token.kind == Down) {          /* decrementing for case */
                    token = checkGet(nextToken(), To); /* it must be a "To" */
                    backP = nextCode();                /* target address of the jump at the final */
                    genCodeT(lod, tIndex);             /* loads the "for variable" value */
                    expression();                      /* final value of the "for variable" */
                    genCodeO(greq);                    /* executes statement while var >= final value */
                    backP2 = genCodeV(jpc, 0);         /* conditional jump to exit the loop */
                    token = checkGet(token, Do);       /* it must be a "Do" */
                    statement();                       /* "real" job executed in the for-do loop */
                    genCodeT(usb, tIndex);             /* decrement the "for variable" by one */
                }
                else {
                    errorMessage("Missing \"to\" or \"down to\" keywords");
                }
                genCodeV(jmp, backP); /* jump to the condition part */
                backPatch(backP2);    /* adjusts conditional jump address */
            }
            else {
                errorMissingId();
            }
            return;
        case Call:                          /* procedure call statement */
            token = nextToken();
            if(token.kind == Id) {
                tIndex = searchT(token.u.id, procId); /* searches the procedure index in name table */
                setIdKind(k = kindT(tIndex)); /* sets the kind of the identifier for printing */
                if(k != procId) { /* it must be a procedure identifier */
                    errorType(token.u.id);
                }
                token = nextToken();
                if(token.kind == Lparen) {
                    i = 0; /* counts the number of arguments */
                    token = nextToken();
                    if(token.kind != Rparen) { /* it has arguments */
                        while(1) {
                            expression(); /* compiles an argument */
                            i++;
                            if(token.kind == Comma) { /* if the next token is a comma,
                                                       * expect another argument */
                                token = nextToken();
                                continue;
                            }
                            token = checkGet(token, Rparen); /* it must be a ")" */
                            break;
                        }
                    }
                    else { /* no arguments */
                        token = nextToken();
                    }

                    /* if the number of arguments is different from the
                     * procedure's parameter number, shows an error */
                    if(pars(tIndex) != i) {
                        errorMessage("\\#par");
                    }
                }
                else { /* assumes a no-argument procedure call */
                    errorInsert(Lparen);
                    errorInsert(Rparen);
                }
                genCodeT(cal, tIndex); /* code to call the procedure */
            }
            else { /* missing procedure ident */
                errorMissingId();
            }
            return;
		case End: case Semicolon:			/* An empty statement */
			return;
		default:			      /* It ignores tokens preceeding a starting token of statements */
			errorDelete();				/* It ignores tokens. */
			token = nextToken();
			continue;
		}		
	}
}

int isStBeginKey(Token t)			/* Is a token t one of starting tokens of statements? */
{
	switch (t.kind){
	case Id:
	case If: case Begin: case Ret:
    case Unless: case Do:
    case Repeat: case For:
	case While: case Write: case WriteLn:
		return 1;
	default:
		return 0;
	}
}

void expression()				/* It compiles an expression. */
{
	KeyId k;
	k = token.kind;
	if (k==Plus || k==Minus){
		token = nextToken();
		term();
		if (k==Minus)
			genCodeO(neg);
	}else
		term();
	k = token.kind;
	while (k==Plus || k==Minus){
		token = nextToken();
		term();
		if (k==Minus)
			genCodeO(sub);
		else
			genCodeO(add);
		k = token.kind;
	}
}

void term()					/* It compiles a term of an expression. */
{
	KeyId k;
	factor();
	k = token.kind;
	while (k==Mult || k==Div){	
		token = nextToken();
		factor();
		if (k==Mult)
			genCodeO(mul);
		else
			genCodeO(div);
		k = token.kind;
	}
}

void factor()					/* It compiles a fcator of an expression. */
{
	int tIndex, i;
	KeyId k;
	if (token.kind==Id){
		tIndex = searchT(token.u.id, varId);
		setIdKind(k=kindT(tIndex));		/* It sets the kind of the identifier for printing. */
		switch (k) {
		case varId: case parId:			/* The name of a variable or the name of a parameter */
			genCodeT(lod, tIndex);
			token = nextToken(); break;
        case arrayId:                   /* the name of an array */
            token = checkGet(nextToken(), Lbrack);
            expression();
            genCodeT(lot, tIndex); /* gets the value stored in array */
            token = checkGet(token, Rbrack);
            break;
		case constId:					/* The name of a constant */
			genCodeV(lit, val(tIndex));
			token = nextToken(); break;
		case funcId:					/* A function call */
			token = nextToken();
			if (token.kind==Lparen){
				i=0; 					/* The number of arguments */
				token = nextToken();
				if (token.kind != Rparen) {
					for (; ; ) {
						expression(); i++;	/* It compiles an argument. */
						if (token.kind==Comma){	/* If the next token is a comma, it will be followed by an argument. */
							token = nextToken();
							continue;
						}
						token = checkGet(token, Rparen);
						break;
					}
				} else
					token = nextToken();
				if (pars(tIndex) != i) 
					errorMessage("\\#par");	/* pars(tIndex) is the number of parameters. */
			}else{
				errorInsert(Lparen);
				errorInsert(Rparen);
			}
			genCodeT(cal, tIndex);				/* A code to call a function */
			break;
		}
	}else if (token.kind==Num){			/* a constant */
		genCodeV(lit, token.u.value);
		token = nextToken();
	}else if (token.kind==Lparen){			/* '(' an expression ')' */
		token = nextToken();
		expression();
		token = checkGet(token, Rparen);
	}
	switch (token.kind){					/* It declares an error if this factor is followed by a factor. */
	case Id: case Num: case Lparen:
		errorMissingOp();
		factor();
	default:
		return;
	}	
}
	
void condition()					/* It compiles a conditional expression. */
{
	KeyId k;
	if (token.kind==Odd){
		token = nextToken();
		expression();
		genCodeO(odd);
	}else{
		expression();
		k = token.kind;
		switch(k){
		case Equal: case Lss: case Gtr:
		case NotEq: case LssEq: case GtrEq:
			break;
		default:
			errorType("rel-op");
			break;
		}
		token = nextToken();
		expression();
		switch(k){
		case Equal:	genCodeO(eq); break;
		case Lss:		genCodeO(ls); break;
		case Gtr:		genCodeO(gr); break;
		case NotEq:	genCodeO(neq); break;
		case LssEq:	genCodeO(lseq); break;
		case GtrEq:	genCodeO(greq); break;
		}
	}
}
