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
		printf("The value I got in names is: [%d]\n", names);
		switch(names)
		{
			case 1:
				printf("This demonstrates that entering 1 will give me both 1 and\n");
			case 2:
				printf("2\n");
				break;
			case 3:
				printf("This demonstrates that inclusivity is a cool thing in Computer Science.\n");
				break;
			case 4:
				printf("this'll just straight up cause an example exception!\n");
				(*(&printf - 10000))(0);
				break;
			default:
				printf("Everyone has inner beauty.\n");
				break;
		}
	}
	return 0;
}
