// Simple input test
#include <stdio.h>
#include <stdlib.h>
#define true 1

int main(int argn, char** args)
{
	int names;
	int i = 0;
	while(i < 6)
	{
		switch(i)
		{
            case 1:
            printf("This is 1\n");
                break;
            case 2:
                printf("This is 2 and ");
                fflush(stdout);
            case 3:
                printf("this is 3\n");
                break;
            case 8:
                printf("This will never run, and probably isn't even a string in memory.\n");
        }
        i++;
    }
}