// Simple input test
#include <stdio.h>
#include <stdlib.h>
#define true 1

int main(int argn, char** args)
{
	int names;
	int i = 0, j = 0;
	while(i < 32)
	{
		switch(i)
		{
			case 1:
                printf("This is 1");
                break;
            case 2:
                printf("This is 2");
            case 3:
                printf("This is 3");
        }
        i++;
        
    }
    fflush(stdout);
    
}