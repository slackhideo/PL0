
   /*****************codegen.c******************/

#include <stdio.h>
#include "codegen.h"
#ifndef TBL
#define TBL
#include "table.h"
#endif
#include "getSource.h"

#define MAXCODE 200			/* The maximum length of codes */
#define MAXMEM 2000			/* The maximum length of the stack */
#define MAXREG 20			/* The maximum number of registers used at a time*/
#define MAXLEVEL 5			/* The maximum block nesting level */

typedef struct inst{				/* An instruction code */
	OpCode  opCode;
	union{
		RelAddr addr;
		int value;
		Operator optr;
	}u;
}Inst;

static char ref[MAXCODE];               /* If ref[i] is not zero, code[i] is referenced by jmp, jpc or cal. */
static Inst code[MAXCODE];		/* Generated codes are stored in this array.*/
static int cIndex = -1;				/* code[cIndex] is the last genereted code. */
static void checkMax();	     		 /* It checks if the length of generated codes will exceed the limit or not. */
static void printCode(int i);		/* It prints a code in the address i. */
static void updateRef(int i);

int nextCode()					/* It returns an address where a next code will be generated. */
{
	return cIndex+1;
}

int genCodeV(OpCode op, int v)		/* It generates a code with an address v. e.g. jmp, jpc and so on. */
{
	checkMax();
	code[cIndex].opCode = op;
	code[cIndex].u.value = v;
	return cIndex;
}

int genCodeT(OpCode op, int ti)		/* It generates a code with an operand pointed by an index ti in the name table. e.g. lod, sto, and cal.*/
{
	checkMax();
	code[cIndex].opCode = op;
	code[cIndex].u.addr = relAddr(ti);
	return cIndex;
}

int genCodeO(Operator p)			/* It generates a code of an operator p*/
{
	checkMax();
	code[cIndex].opCode = opr;
	code[cIndex].u.optr = p;
	return cIndex;
}

int genCodeR()					/*　It generates a return code */
{
	if (code[cIndex].opCode == ret)
		return cIndex;			/*　It avoids succsessive return codes. */
	checkMax();
	code[cIndex].opCode = ret;
	code[cIndex].u.addr.level = bLevel();
	code[cIndex].u.addr.addr = fPars();	/*　The number of parameters, which will be used to release a stack frame. */
	return cIndex;
}

void checkMax()		/* It checks if the length of genereted codes will exceed the limit or not. */
{
	if (++cIndex < MAXCODE)
		return;
	errorF("too many code");
}
	
void backPatch(int i)		/* It puts an address where a next code will be genereted into a code at an address i. */
{
	code[i].u.value = cIndex+1;
}

void listCode()			/* It lists generated codes. */
{
	int i;
	printf("\n; code\n");

	for(i=0; i<=cIndex; i++){
	  ref[i] = 0;
	}
	for(i=0; i<=cIndex; i++){
	  updateRef(i);
	}
	for(i=0; i<=cIndex; i++){
		if (ref[i]) {
		  printf("L%3.3d: ", i);
		} else {
		  printf("      ");
		}
		printCode(i);
	}
}

void updateRef(int i)		/* It updates the array ref. */
{
	int flag;
	switch(code[i].opCode){
	case lit: flag=1; break;
	case opr: flag=3; break;
	case lod: flag=2; break;
	case sto: flag=2; break;
	case cal: flag=5; break;
	case ret: flag=2; break;
	case ict: flag=1; break;
	case jmp: flag=4; break;
	case jpc: flag=4; break;
	}
	switch(flag){
	case 4:
		ref[code[i].u.value] = 1;
		return;
	case 5:
		ref[code[i].u.addr.addr] = 1;
		return;
	}
}	

void printCode(int i)		/* It prints an instruction code in the address i. */
{
	int flag;
	switch(code[i].opCode){
	case lit: printf("lit"); flag=1; break;
	case opr: printf("opr"); flag=3; break;
	case lod: printf("lod"); flag=2; break;
	case sto: printf("sto"); flag=2; break;
	case cal: printf("cal"); flag=5; break;
	case ret: printf("ret"); flag=2; break;
	case ict: printf("ict"); flag=1; break;
	case jmp: printf("jmp"); flag=4; break;
	case jpc: printf("jpc"); flag=4; break;
	}
	switch(flag){
	case 1:
		printf(",%d\n", code[i].u.value);
		return;
	case 2:
		printf(",%d", code[i].u.addr.level);
		printf(",%d\n", code[i].u.addr.addr);
		return;
	case 3:
		switch(code[i].u.optr){
		case neg: printf(",neg\n"); return;
		case add: printf(",add\n"); return;
		case sub: printf(",sub\n"); return;
		case mul: printf(",mul\n"); return;
		case div: printf(",div\n"); return;
		case odd: printf(",odd\n"); return;
		case eq: printf(",eq\n"); return;
		case ls: printf(",ls\n"); return;
		case gr: printf(",gr\n"); return;
		case neq: printf(",neq\n"); return;
		case lseq: printf(",lseq\n"); return;
		case greq: printf(",greq\n"); return;
		case wrt: printf(",wrt\n"); return;
		case wrl: printf(",wrl\n"); return;
		}
	case 4:
		printf(",L%3.3d\n", code[i].u.value);
		return;
	case 5:
		printf(",%d", code[i].u.addr.level);
		printf(",L%3.3d\n", code[i].u.addr.addr);
		return;
	}
}	

void execute()			/* It executes generated codes */
{
	int stack[MAXMEM];		/* a stack */
	int display[MAXLEVEL];	/* Starting addresses of stack frames of lexically visible blocks at the moment */
	int pc, top, lev, temp;
	Inst i;					/* An instruction code to be executed */
	printf("; start execution\n");
	top = 0;  pc = 0;			/* top: a stack top where the next data will be pushed, pc: a program counter */
	stack[0] = 0;  stack[1] = 0; 	/* stack[top] is a place to preserve a disply which will be overwritten by a callee. */
	/* stack[top+1] is a place to record a return address to a caller. */
	display[0] = 0;			/* The starting address of the main block is 0. */
	do {
		i = code[pc++];			/* It fetches an instruction code to be executed. */
		switch(i.opCode){
		case lit: stack[top++] = i.u.value; 
				break;
		case lod: stack[top++] = stack[display[i.u.addr.level] + i.u.addr.addr]; 
				 break;
		case sto: stack[display[i.u.addr.level] + i.u.addr.addr] = stack[--top]; 
				 break;
		case cal: lev = i.u.addr.level +1;	/* The level of the name of a callee is i.u.addr.level */
		  /* The level of the body of the callee is i.u.addr.level+1. */
				stack[top] = display[lev]; 	/*　It preserves display[lev] in stack[top]　*/
				stack[top+1] = pc; display[lev] = top; /* The stack frame of a callee starts at top. */
				pc = i.u.addr.addr;
				 break;
		case ret: temp = stack[--top];		/* It preserves a return value into a variable temp. */
				top = display[i.u.addr.level];  	/* It restores top. */
				display[i.u.addr.level] = stack[top];		/* It resotres a display. */
				pc = stack[top+1];
				top -= i.u.addr.addr;		/* It moves top by the number of arguments. */
				stack[top++] = temp;		/* It pushes the return value preserved in temp. */
				break;
		case ict: top += i.u.value; 
				if (top >= MAXMEM-MAXREG)
					errorF("stack overflow");
				break;
		case jmp: pc = i.u.value; break;
		case jpc: if (stack[--top] == 0)
					pc = i.u.value;
				break;
		case opr: 
			switch(i.u.optr){
			case neg: stack[top-1] = -stack[top-1]; continue;
			case add: --top;  stack[top-1] += stack[top]; continue;
			case sub: --top; stack[top-1] -= stack[top]; continue;
			case mul: --top;  stack[top-1] *= stack[top];  continue;
			case div: --top;  stack[top-1] /= stack[top]; continue;
			case odd: stack[top-1] = stack[top-1] & 1; continue;
			case eq: --top;  stack[top-1] = (stack[top-1] == stack[top]); continue;
			case ls: --top;  stack[top-1] = (stack[top-1] < stack[top]); continue;
			case gr: --top;  stack[top-1] = (stack[top-1] > stack[top]); continue;
			case neq: --top;  stack[top-1] = (stack[top-1] != stack[top]); continue;
			case lseq: --top;  stack[top-1] = (stack[top-1] <= stack[top]); continue;
			case greq: --top;  stack[top-1] = (stack[top-1] >= stack[top]); continue;
			case wrt: printf("%d ", stack[--top]); continue;
			case wrl: printf("\n"); continue;
			}
		}
	} while (pc != 0);
}

