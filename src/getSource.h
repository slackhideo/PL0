   /*************** getSource.h ***************/

#include <stdio.h>
#ifndef TBL
#define TBL
#include "table.h"
#endif
 
#define MAXNAME 31			/* The maximum length of names */
 
typedef  enum  keys {			/*¡¡Names of reserved words and characters */
	Begin, End,				/* Reserved words */
	If, Then,
	While, Do,
	Ret, Func, 
	Var, Const, Odd,
	Write, WriteLn,
	end_of_KeyWd,				/* The end of reserved words */
	Plus, Minus,				/* Operators and delimiters */
	Mult, Div,	
	Lparen, Rparen,
	Equal, Lss, Gtr,
	NotEq, LssEq, GtrEq, 
	Comma, Period, Semicolon,
	Assign,
	end_of_KeySym,				/* The end of operators and delimiters */
	Id, Num, nul,				/* Tokens */
	end_of_Token,
	letter, digit, colon, others		/* Other characters */
} KeyId;

typedef  struct  token {			/* The structure of tokens */
	KeyId kind;				/* The kind of a token or the name of a key */
	union {
		char id[MAXNAME]; 		/* The name if it is an identfier */
		int value;				/*¡¡The value if it is a number */
	} u;
}Token;

Token nextToken();				/* It returns the next token. */
Token checkGet(Token t, KeyId k);	/* t.kind==k ? */
	/* If t.kind==k, it returns the next token. */
	/* If t.kind!=k, it declares an error. In addition, if both t and k are symbols or reserved words, */
	/* it ignores t and returns the next token. This means that it replaces t with k. */
	/* Othewise, it assumes that it inserts k, and returns t. */

int openSource(char fileName[]); 	/* It opens a source file. */
void closeSource();			/* It closes a source file and the html (or tex) file. */
void initSource();			/* It initializes both the character class table and the html (or tex) file. */  
void finalSource(); 			/* It checks the end of the source file and outputs the end of the html (or tex) file. */  
void errorType(char *m);		/* It outputs a type error to the html (or tex) file. */
void errorInsert(KeyId k);		/* It outputs an error which means that there must be keyString(k). */
void errorMissingId();		/* It outputs a missing id error to the html (or tex) file. */
void errorMissingOp();		/* It outputs a missing operator error to the html (or tex) file. */
void errorDelete();			/* It outputs an extra token error to the html (or tex) file. */
void errorMessage(char *m);	/* It outputs an error message to the html (or tex) file. */
void errorF(char *m);			/* It outputs a fatal error and gives up the compilation. */
int errorN();				/* It returns the number of errors. */

void setIdKind(KindT k);     /* It sets the kind of the current token (id) for the html (or tex) file. */

