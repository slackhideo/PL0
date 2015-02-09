
 /***********table.h***********/

typedef enum kindT {		/* Kinds of identifiers */
	varId, funcId, parId, constId
}KindT;
typedef struct relAddr{		/* The structure for variables, parameters and function addresses */
	int level;
	int addr;
}RelAddr; 

void blockBegin(int firstAddr);	/* It is called when a block starts. */
void blockEnd();			/* It is called when a block ends. */
int bLevel();				/* It returns the level of the current block. */
int fPars();				/* It returns the number of parameters of the current block. */
int enterTfunc(char *id, int v);	/* It records the name and the starting address of a function in the name table. */
int enterTvar(char *id);		/* It records a variable name in the name table. */
int enterTpar(char *id);		/* It records parameters in the name table. */
int enterTconst(char *id, int v);	/* It records a constant and its value in the name table. */
void endpar();				/* It is called when parameters end. */
void changeV(int ti, int newVal);	/* It changes the value of the ti-th element in the name table (the starting address of a function) */

int searchT(char *id, KindT k);	/* It returns the index of an element whose name is id. */
						/* It declares an error if it is undefined. */
KindT kindT(int i);			/* It returns the kind of the i-th element in the name table. */

RelAddr relAddr(int ti);		/* It returns the address of the i-th element in the name table. */
int val(int ti);				/* It returns the value of the i-th element in the name table. */
int pars(int ti);				/* It returns the number of parameters of a function (the i-th element in the name table). */
int frameL();				/* The maximum relative address of variables in a block */

