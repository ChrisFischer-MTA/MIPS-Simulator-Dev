// Simple input test
#include <stdio.h>
#include <stdlib.h>

int main(int argn, char** args)
{
	int names;
	
	int name0;
	int name1;
	int name2;
	int name3;
	int name4;
	int name5;
	int name6;
	int name7;
	int name8;
	int name9;
	int name10;
	
	
	int i = 0;
	printf("Hello, how many names should I ask for?\n");
	printf("Pointer [0x%x]\n", names);
	printf("name0 [0x%x]\n",  &name0);
	printf("name1 [0x%x]\n",  &name1);
	printf("name2 [0x%x]\n",  &name2);
	printf("name3 [0x%x]\n",  &name3);
	printf("name4 [0x%x]\n",  &name4);
	printf("name5 [0x%x]\n",  &name5);
	printf("name6 [0x%x]\n",  &name6);
	printf("name7 [0x%x]\n",  &name7);
	printf("name8 [0x%x]\n",  &name8);
	printf("name9 [0x%x]\n",  &name9);
	printf("name10 [0x%x]\n", &name10);
	scanf("%d %d %d %d %d %d %d %d %d %d %d", &name0, &name1, &name2, &name3, &name4, &name5, &name6, &name7, &name8, &name9, &name10);
	scanf("%d", &names);
	for(i = 0; i < names; i++)
	{
		char* tmp = malloc(sizeof(char)*1024);
		printf("Hi, what's your name? :)\n");
		scanf("%s", tmp);
		printf("Hi %s, my name is Lamda. I'm a sentient AI enginner\n", tmp);
	}
	return 0;
}
