#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// This is testing a infinite buffer overflow.
// This should segmentation fault.

int main(void)
{
	char mem[20];
	// Fill it with a string that's 15 bytes
	strcpy(mem, "Wow, 15 bytes.");
	printf("Normal copy succeeded!\n");
	
	// Do the dangerous copy.
    int i=0;
    mem[0] = 'A';
	while(1)
    {
        i++;
        mem[i] = 'H';
    }
	return 0;
}
