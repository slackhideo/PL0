
    /********* main.c *********/

#include <stdio.h>
#include "getSource.h"

int main(int argc, char* argv[])
{
  if (argc == 2) {
    /* pl0d src */
    if (!openSource(argv[1]))
      return 1;
    if (compile()){
      execute();
    }
    closeSource();			/* It closes a source file */
  } else if  (argc == 3 && strcmp(argv[1], "-l") == 0) {
    /* pl0d -l src */
    if (!openSource(argv[2]))
      return 1;
    if (compile()){
      listCode();
    }
    closeSource();			/* It closes a source file */
  } else {
    printf("pl0d [-l] src\n");
  }
  return 0;
}
