// Simple input test
#include <stdio.h>
#include <stdlib.h>
#define true 1

int main(int argn, char** args)
{
	int names;
	int i = 0;
	while(true)
	{
		scanf("%d", &names);
		switch(names)
		{
			case 1:
                i + 4;
                break;
            case 2:
                i + 3;
            case 3:
                i * 2;
        }
        printf("Number: %d", i);
    }
}