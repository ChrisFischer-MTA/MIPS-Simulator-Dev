// Simple input test
#include <stdio.h>
#include <stdlib.h>

int main(int argn, char** args)
{
	int names = 0;
	int i = 0;
	printf("Hello, how many names should I ask for?\n");
	printf("Pointer [0x%x]\n", names);
	scanf("%d", &names);
	for(i = 0; i < names; i++)
	{
		char* tmp = malloc(sizeof(char)*64);
		printf("Hi, what's your name? :)\n");
		scanf("%s", tmp);
		printf("Hi %s, my name is Lamda. I'm a sentient AI enginner\n", tmp);
	}
	return 0;
}
