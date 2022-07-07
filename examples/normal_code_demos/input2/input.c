// Simple input test
#include <stdio.h>
#include <stdlib.h>

int recursions()
{
	return recursions();
}

int main(int argn, char** args)
{
	int names;
	int i = 0;
	printf("Would you like to select either:\n");
	printf("1 - a warm hello :)\n");
	printf("2 - a cold truth \n");
	//printf("Pointer [0x%x]\n", names);
	scanf("%d", &names);
	printf("%d names!", names);
	if(names == 1)
	{
		printf("Hello!\n");
	}
	else if(names == 2)
	{
		printf("A cold truth.\n");
	}
	else if(names == 3)
	{
		printf("A secret option!\n");
	}
	else if(names == 4)
	{
		printf("It's hidden companion!\n");

	}
	else if(names == 5)
	{
		printf("seppiku\n");
		recursions();
	}
	else
	{
		printf("bad branch");
	}

	return 0;
}
