#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Stack overflow 1.
// No detections for this yet. Shouldn't segmentation fault.

int main(void)
{
	// Allocate 100 bytes of memory.
	char mem[20];
	// Fill it with a string that's 15 bytes
	strcpy(mem, "Wow, 15 bytes.");
	printf("Normal copy succeeded!\n");
	
	// Do the dangerous copy.
	strcpy(mem, "Wow, more then 15 bytes?");
	printf("Dangerous copy succeeded!\n");
	return 0;
}
