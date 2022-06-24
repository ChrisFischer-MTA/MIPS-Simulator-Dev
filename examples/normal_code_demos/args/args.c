#include <stdio.h>

// Print out the arguments.

int main(int argn, char** args)
{
	int i = 0;
	printf("This program was called with %d arguments\n", argn);
	for(i = 0; i < argn; i++)
	{
		printf("Argument [%d] is : [%s]\n", argn, args[i]); 
	}
	return 1;
}
