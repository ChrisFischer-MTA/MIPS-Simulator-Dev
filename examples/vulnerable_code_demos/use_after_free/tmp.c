#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// UAF 1

int main(void)
{
	// Allocate 100 bytes of memory.
	char* mem = malloc(sizeof(char)*20);
	// Fill it with a string that's 15 bytes
	strcpy(mem, "Wow, 15 bytes.");
	printf("Normal copy succeeded!\n");
	
	free(mem);
	printf("%s\n", mem);
	// Do the dangerous copy.
	printf("Dangerous usage succeeded!\n");
	return 0;
}
