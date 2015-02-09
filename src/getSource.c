
   /**************getSource.c************/

#include <stdio.h>
#include <string.h>
#include "getSource.h"

#define MAXLINE 120		/* The maximum line length */
#define MAXERROR 30		/* The acceptable number of errors in one compilation */
#define MAXNUM  14		/* The maximum figures of a constant */
#define TAB   5				/* The number of white spaces for a tab */
#define INSERT_C  "#0000FF"  /* The color for inserted characters */
#define DELETE_C  "#FF0000"  /* The color for deleted characters */
#define TYPE_C  "#00FF00"  /* The color for type errors */

static FILE *fpi;				/* a source file */
static FILE *fptex;			/* an output file (html or tex) */
static char line[MAXLINE];	/* An input buffer for one line */
static int lineIndex;			/* The place of the next character in the input buffer. */
static char ch;				/* The next char */

static Token cToken;			/* The next token */
static KindT idKind;			/* A kind of the current token (id) */
static int spaces;			/* The number of white space preceding the token. */
static int CR;				/* The number of the preceding CR codes */
static int printed;			/* Has the token been printed? */

static int errorNo = 0;			/* The number of emitted errors */
static char nextChar();		/* It returns the next character. */
static int isKeySym(KeyId k);	/* Is t a symbol? */
static int isKeyWd(KeyId k);		/* Is t a reserved word? */
static void printSpaces();		/* It prints white space preceding the token. */
static void printcToken();		/* It prints the current token. */

struct keyWd {				/* The structure for reserved words, symbols and names (KeyId) */
	char *word;
	KeyId keyId;
};

static struct keyWd KeyWdT[] = {	/* The table containing reserved words, symbols and names (KeyId) */
	{"begin", Begin},
	{"end", End},
	{"if", If},
	{"then", Then},
	{"while", While},
	{"do", Do},
	{"return", Ret},
	{"function", Func},
	{"var", Var},
	{"const", Const},
	{"odd", Odd},
	{"write", Write},
	{"writeln",WriteLn},
	{"$dummy1",end_of_KeyWd},
							/* The table containing symbols and names (KeyId) */
	{"+", Plus},
	{"-", Minus},
	{"*", Mult},
	{"/", Div},
	{"(", Lparen},
	{")", Rparen},
	{"=", Equal},
	{"<", Lss},
	{">", Gtr},
	{"<>", NotEq},
	{"<=", LssEq},
	{">=", GtrEq},
	{",", Comma},
	{".", Period},
	{";", Semicolon},
	{":=", Assign},
	{"$dummy2",end_of_KeySym}
};

int isKeyWd(KeyId k)			/* Is a key k a reserved word? */
{
	return (k < end_of_KeyWd);
}

int isKeySym(KeyId k)		/* Is a key k a symbol? */
{
	if (k < end_of_KeyWd)
		return 0;
	return (k < end_of_KeySym);
}

static KeyId charClassT[256];		/* The table containing kinds of characters */

static void initCharClassT()		/* It initializes the table containing kinds of characters */
{
	int i;
	for (i=0; i<256; i++)
		charClassT[i] = others;
	for (i='0'; i<='9'; i++)
		charClassT[i] = digit;
	for (i='A'; i<='Z'; i++)
		charClassT[i] = letter;
	for (i='a'; i<='z'; i++)
		charClassT[i] = letter;
	charClassT['+'] = Plus; charClassT['-'] = Minus;
	charClassT['*'] = Mult; charClassT['/'] = Div;
	charClassT['('] = Lparen; charClassT[')'] = Rparen;
	charClassT['='] = Equal; charClassT['<'] = Lss;
	charClassT['>'] = Gtr; charClassT[','] = Comma;
	charClassT['.'] = Period; charClassT[';'] = Semicolon;
	charClassT[':'] = colon;
}

int openSource(char fileName[]) 		/* It opens a source file. */
{
	char fileNameO[30];
	if ( (fpi = fopen(fileName,"r")) == NULL ) {
		printf("can't open %s\n", fileName);
		return 0;
	}
	strcpy(fileNameO, fileName);
#if defined(LATEX)
	strcat(fileNameO,".tex");
#elif defined(TOKEN_HTML)
	strcat(fileNameO,".html");
#else
	strcat(fileNameO,".html");
#endif
	if ( (fptex = fopen(fileNameO,"w")) == NULL ) {	 /* It creates an html (or tex) file. */
		printf("can't open %s\n", fileNameO);
		return 0;
	} 
	return 1;
}

void closeSource()				 /* It closes a source file and the html (or tex) file. */
{
	fclose(fpi);
	fclose(fptex);
}

void initSource()
{
	lineIndex = -1;				 /* Initialization */
	ch = '\n';
	printed = 1;
	initCharClassT();
#if defined(LATEX)
	fprintf(fptex,"\\documentstyle[12pt]{article}\n");   
	fprintf(fptex,"\\begin{document}\n");
	fprintf(fptex,"\\fboxsep=0pt\n");
	fprintf(fptex,"\\def\\insert#1{$\\fbox{#1}$}\n");
	fprintf(fptex,"\\def\\delete#1{$\\fboxrule=.5mm\\fbox{#1}$}\n");
	fprintf(fptex,"\\rm\n");
#elif defined(TOKEN_HTML)
	fprintf(fptex,"<HTML>\n");   /* An HTML tag */
	fprintf(fptex,"<HEAD>\n<TITLE>compiled source program</TITLE>\n</HEAD>\n");
	fprintf(fptex,"<BODY>\n<PRE>\n");
#else
	fprintf(fptex,"<HTML>\n");   /* An HTML tag */
	fprintf(fptex,"<HEAD>\n<TITLE>compiled source program</TITLE>\n</HEAD>\n");
	fprintf(fptex,"<BODY>\n<PRE>\n");
#endif
}

void finalSource()
{
	if (cToken.kind==Period)
		printcToken();
	else
		errorInsert(Period);
#if defined(LATEX)
	fprintf(fptex,"\n\\end{document}\n");
#elif defined(TOKEN_HTML)
	fprintf(fptex,"\n</PRE>\n</BODY>\n</HTML>\n");
#else
	fprintf(fptex,"\n</PRE>\n</BODY>\n</HTML>\n");
#endif
}
	
/* FYI, ordinary error messages */
/*
void error(char *m)	
{
	if (lineIndex > 0)
		printf("%*s\n", lineIndex, "***^");
	else
		printf("^\n");
	printf("*** error *** %s\n", m);
	errorNo++;
	if (errorNo > MAXERROR){
		printf("too many errors\n");
		printf("abort compilation\n");	
		exit (1);
	}
}
*/

void errorNoCheck()			/* It gives up the compilation if the number of errors exceeds the limit. */
{
	if (errorNo++ > MAXERROR){
#if defined(LATEX)
	        fprintf(fptex, "too many errors\n\\end{document}\n");
#elif defined(TOKEN_HTML)
		fprintf(fptex, "too many errors\n</PRE>\n</BODY>\n</HTML>\n");
#else
		fprintf(fptex, "too many errors\n</PRE>\n</BODY>\n</HTML>\n");
#endif
		printf("; abort compilation\n");	
		exit (1);
	}
}

void errorType(char *m)		/* It outputs a type error to the html (or tex) file. */
{
	printSpaces();
#if defined(LATEX)
	fprintf(fptex, "\\(\\stackrel{\\mbox{\\scriptsize %s}}{\\mbox{", m);
	printcToken();
	fprintf(fptex, "}}\\)");
#elif defined(TOKEN_HTML)
	fprintf(fptex, "<FONT COLOR=%s>", TYPE_C);
	fprintf(fptex, "TypeError(%s)-&gt", m);
	printcToken();
	fprintf(fptex, "</FONT>");
#else
	fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", TYPE_C, m);
	printcToken();
#endif
	errorNoCheck();
}

void errorInsert(KeyId k)		/* It outputs an error which means that there must be keyString(k). */
{
#if defined(LATEX)
         if (k < end_of_KeyWd) 	/* A reserved word */
	   fprintf(fptex, "\\ \\insert{{\\bf %s}}", KeyWdT[k].word); 
	 else 					/*　An operator or a symbol */
	   fprintf(fptex, "\\ \\insert{$%s$}", KeyWdT[k].word);
#elif defined(TOKEN_HTML)
	fprintf(fptex, "<FONT COLOR=%s>", INSERT_C);
        fprintf(fptex, "insert ");
         if (k < end_of_KeyWd) 	/*　予約語　*/
	   fprintf(fptex, "(Keyword, '%s')", KeyWdT[k].word);
	 else 					/* An operator or a symbol */
	   fprintf(fptex, "(Symbol, '%s')", KeyWdT[k].word);
	fprintf(fptex, "</FONT>");
#else
	fprintf(fptex, "<FONT COLOR=%s><b>%s</b></FONT>", INSERT_C, KeyWdT[k].word);
#endif
	errorNoCheck();
}

void errorMissingId()			/* It outputs a missing id error to the html (or tex) file. */
{
#if defined(LATEX)
        fprintf(fptex, "\\insert{Id}"); 
#elif defined(TOKEN_HTML)
	fprintf(fptex, "<FONT COLOR=%s>", INSERT_C);
        fprintf(fptex, "insert ");
	fprintf(fptex, "(Id, identifier)");
	fprintf(fptex, "</FONT>");
#else
	fprintf(fptex, "<FONT COLOR=%s>Id</FONT>", INSERT_C);
#endif
	errorNoCheck();
}

void errorMissingOp()		/* It outputs a missing operator error to the html (or tex) file. */
{
#if defined(LATEX)
	fprintf(fptex, "\\insert{$\\otimes$}");
#elif defined(TOKEN_HTML)
	fprintf(fptex, "<FONT COLOR=%s>", INSERT_C);
        fprintf(fptex, "insert ");
	fprintf(fptex, "(Symbol, operator)");
	fprintf(fptex, "</FONT>");
#else
	fprintf(fptex, "<FONT COLOR=%s>@</FONT>", INSERT_C);
#endif
	errorNoCheck();
}

void errorDelete()			/* It outputs an extra token error to the html (or tex) file. */
{
	int i=(int)cToken.kind;
	printSpaces();
	printed = 1;
#if defined(LATEX)
	if (i < end_of_KeyWd) {							/* A reserved word */
	        fprintf(fptex, "\\delete{{\\bf %s}}", KeyWdT[i].word);
	} else if (i < end_of_KeySym) {					/* An operator or a delimiter */
		fprintf(fptex, "\\delete{$%s$}", KeyWdT[i].word);
	} else if (i==(int)Id) {						/*　Identfier　*/
		fprintf(fptex, "\\delete{%s}", cToken.u.id);
	} else if (i==(int)Num) {							/*　Num　*/
		fprintf(fptex, "\\delete{%d}", cToken.u.value);
	}
#elif defined(TOKEN_HTML)
	if (i < end_of_KeyWd) {							/* A reserved word */
	        fprintf(fptex, "<FONT COLOR=%s>", DELETE_C);
		fprintf(fptex, "delete ");
		fprintf(fptex, "(Keyword, '%s')", KeyWdT[i].word);
                fprintf(fptex, "</FONT>");
	} else if (i < end_of_KeySym) {					/* An operator or a delimiter */
	        fprintf(fptex, "<FONT COLOR=%s>", DELETE_C);
		fprintf(fptex, "delete ");
		fprintf(fptex, "(Symbol, '%s')", KeyWdT[i].word);
                fprintf(fptex, "</FONT>");
	} else if (i==(int)Id) {						/*　Identfier　*/
	        fprintf(fptex, "<FONT COLOR=%s>", DELETE_C);
		fprintf(fptex, "delete ");
		fprintf(fptex, "(Id, '%s')", cToken.u.id);
                fprintf(fptex, "</FONT>");
	} else if (i==(int)Num) {							/*　Num　*/
	        fprintf(fptex, "<FONT COLOR=%s>", DELETE_C);
		fprintf(fptex, "delete ");
		fprintf(fptex, "(number, '%d')", cToken.u.value);
                fprintf(fptex, "</FONT>");
	}
#else 
	if (i < end_of_KeyWd) {							/* A reserved word */
		fprintf(fptex, "<FONT COLOR=%s><b>%s</b></FONT>", DELETE_C, KeyWdT[i].word);
	} else if (i < end_of_KeySym) {					/* An operator or a delimiter */
		fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", DELETE_C, KeyWdT[i].word);
	} else if (i==(int)Id) {						/*　Identfier　*/
		fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", DELETE_C, cToken.u.id);
	} else if (i==(int)Num) {							/*　Num　*/
		fprintf(fptex, "<FONT COLOR=%s>%d</FONT>", DELETE_C, cToken.u.value);
	}
#endif
}

void errorMessage(char *m)	/* It outputs an error message to the html (or tex) file. */
{
#if defined(LATEX)
	fprintf(fptex, "$^{%s}$", m);
#elif defined(TOKEN_HTML)
	fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", TYPE_C, m);
#else
	fprintf(fptex, "<FONT COLOR=%s>%s</FONT>", TYPE_C, m);
#endif
	errorNoCheck();
}

void errorF(char *m)			/* It outputs a fatal error and gives up the compilation. */
{
	errorMessage(m);
#if defined(LATEX)
	fprintf(fptex, "fatal errors\n\\end{document}\n");
#elif defined(TOKEN_HTML)
	fprintf(fptex, "fatal errors\n</PRE>\n</BODY>\n</HTML>\n");
#else
	fprintf(fptex, "fatal errors\n</PRE>\n</BODY>\n</HTML>\n");
#endif
	if (errorNo)
		printf("; total %d errors\n", errorNo);
	printf("; abort compilation\n");	
	exit (1);
}

int errorN()				/* It returns the number of errors. */
{
	return errorNo;
}

char nextChar()				/* It returns the next character. */
{
	char ch;
	if (lineIndex == -1){
		if (fgets(line, MAXLINE, fpi) != NULL){ 
/*			puts(line); */	/* FYI, ordinary error messages */
			lineIndex = 0;
		} else {
			errorF("end of file\n");      /* It gives up the compilation if it reaches end of file. */
		}
	}
	if ((ch = line[lineIndex++]) == '\n'){	 /*　ch gets the next character. */
		lineIndex = -1;				/* If ch is a new line character, it prepare for the next line. */
		return '\n';				/* It returns a new line character. */
	}
	return ch;
}

Token nextToken()			/* It returns the next token. */
{
	int i = 0;
	int num;
	KeyId cc;
	Token temp;
	char ident[MAXNAME];
	printcToken(); 			/* It prints the previous token. */
	spaces = 0; CR = 0;
	while (1){				/* It counts the number of white spaces and new lines preceeding the next token. */
		if (ch == ' ')
			spaces++;
		else if	(ch == '\t')
			spaces+=TAB;
		else if (ch == '\n'){
			spaces = 0;  CR++;
		}
		else break;
		ch = nextChar();
	}
	switch (cc = charClassT[ch]) {
	case letter: 				/* identifier */
		do {
			if (i < MAXNAME)
				ident[i] = ch;
			i++; ch = nextChar();
		} while (  charClassT[ch] == letter
				|| charClassT[ch] == digit );
		if (i >= MAXNAME){
			errorMessage("too long");
			i = MAXNAME - 1;
		}	
		ident[i] = '\0'; 
		for (i=0; i<end_of_KeyWd; i++)
			if (strcmp(ident, KeyWdT[i].word) == 0) {
				temp.kind = KeyWdT[i].keyId;  		/* a reserved word */
				cToken = temp; printed = 0;
				return temp;
			}
		temp.kind = Id;		/* A user defined name */
		strcpy(temp.u.id, ident);
		break;
	case digit: 					/* number */
		num = 0;
		do {
			num = 10*num+(ch-'0');
			i++; ch = nextChar();
		} while (charClassT[ch] == digit);
      		if (i>MAXNUM)
      			errorMessage("too large");
      		temp.kind = Num;
		temp.u.value = num;
		break;
	case colon:
		if ((ch = nextChar()) == '=') {
			ch = nextChar();
			temp.kind = Assign;		/*　":="　*/
			break;
		} else {
			temp.kind = nul;
			break;
		}
	case Lss:
		if ((ch = nextChar()) == '=') {
			ch = nextChar();
			temp.kind = LssEq;		/*　"<="　*/
			break;
		} else if (ch == '>') {
			ch = nextChar();
			temp.kind = NotEq;		/*　"<>"　*/
			break;
		} else {
			temp.kind = Lss;
			break;
		}
	case Gtr:
		if ((ch = nextChar()) == '=') {
			ch = nextChar();
			temp.kind = GtrEq;		/*　">="　*/
			break;
		} else {
			temp.kind = Gtr;
			break;
		}
	default:
		temp.kind = cc;
		ch = nextChar(); break;
	}
	cToken = temp; printed = 0;
	return temp;
}

Token checkGet(Token t, KeyId k)			/* t.kind==k ? */
	/* If t.kind==k, it returns the next token. */
	/* If t.kind!=k, it declares an error. In addition, if both t and k are symbols or reserved words, */
	/* it ignores t and returns the next token. This means that it replaces t with k. */
	/* Othewise, it assumes that it inserts k, and returns t. */
{
	if (t.kind==k)
			return nextToken();
	if ((isKeyWd(k) && isKeyWd(t.kind)) ||
		(isKeySym(k) && isKeySym(t.kind))){
			errorDelete();
			errorInsert(k);
			return nextToken();
	}
	errorInsert(k);
	return t;
}

static void printSpaces()			/* It prints white spaces and new lines. */
{
#if defined(LATEX)
        while (CR-- > 0) {
		fprintf(fptex, "\\ \\par\n");
	}
	while (spaces-- > 0) {
		fprintf(fptex, " ");
		fprintf(fptex, "\\ ");
	}
#elif defined(TOKEN_HTML)
        while (CR-- > 0) {
		fprintf(fptex, "\n");
	}
	while (spaces-- > 0) {
		fprintf(fptex, " ");
	}
#else
        while (CR-- > 0) {
		fprintf(fptex, "\n");
	}
	while (spaces-- > 0) {
		fprintf(fptex, " ");
	}
#endif
	CR = 0; spaces = 0;
}

void printcToken()				/* It prints the current token. */
{
	int i=(int)cToken.kind;
	if (printed){
		printed = 0; return;
	}
	printed = 1;
	printSpaces();				/* It prints white spaces and new lines preceeding the token. */
#if defined(LATEX)
	if (i < end_of_KeyWd) {						/* A reserved word */
		fprintf(fptex, "{\\bf %s}", KeyWdT[i].word);
	} else if (i < end_of_KeySym) {					/* An operator or a delimiter */
		fprintf(fptex, "$%s$", KeyWdT[i].word);
	} else if (i==(int)Id){							/*　Identfier　*/
		switch (idKind) {
		case varId: 
			fprintf(fptex, "%s", cToken.u.id); return;
		case parId: 
			fprintf(fptex, "{\\sl %s}", cToken.u.id); return;
		case funcId: 
			fprintf(fptex, "{\\it %s}", cToken.u.id); return;
		case constId: 
			fprintf(fptex, "{\\sf %s}", cToken.u.id); return;
		}
	}else if (i==(int)Num) {		/*　Num　*/
		fprintf(fptex, "%d", cToken.u.value);
	}
#elif defined(TOKEN_HTML)
	if (i < end_of_KeyWd) {						/* A reserved word */
		fprintf(fptex, "(Keyword, '%s') ", KeyWdT[i].word);
	} else if (i < end_of_KeySym) {					/* An operator or a delimiter */
		fprintf(fptex, "(Symbol, '%s') ", KeyWdT[i].word);
	} else if (i==(int)Id){							/*　Identfier　*/
		switch (idKind) {
		case varId: 
			fprintf(fptex, "(varId, '%s') ", cToken.u.id); return;
		case parId: 
			fprintf(fptex, "(parId, '%s') ", cToken.u.id); return;
		case funcId: 
			fprintf(fptex, "(funcId, '%s') ", cToken.u.id); return;
		case constId: 
			fprintf(fptex, "(constId, '%s') ", cToken.u.id); return;
		}
	}else if (i==(int)Num) {		/*　Num　*/
		fprintf(fptex, "(number, '%d') ", cToken.u.value);
	}
#else
	if (i < end_of_KeyWd) {						/* A reserved word */
		fprintf(fptex, "<b>%s</b>", KeyWdT[i].word);
	} else if (i < end_of_KeySym) {					/* An operator or a delimiter */
		fprintf(fptex, "%s", KeyWdT[i].word);
	} else if (i==(int)Id){							/*　Identfier　*/
		switch (idKind) {
		case varId: 
			fprintf(fptex, "%s", cToken.u.id); return;
		case parId: 
			fprintf(fptex, "<i>%s</i>", cToken.u.id); return;
		case funcId: 
			fprintf(fptex, "<i>%s</i>", cToken.u.id); return;
		case constId: 
			fprintf(fptex, "<tt>%s</tt>", cToken.u.id); return;
		}
	}else if (i==(int)Num) {		/*　Num　*/
		fprintf(fptex, "%d", cToken.u.value);
	}
#endif
}

void setIdKind (KindT k)		 /* It sets the kind of the current token (id) for the html (or tex) file. */
{
	idKind = k;
}



