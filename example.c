#include <stdio.h>
#include <stdlib.h>


int custom_printf(char* input)
{
	__asm__("movq $3405688917, %r15");
	printf("This is custom\n");
	printf("%s",input);
	__asm("movq $3405647957, %r15");
	return 1;
}

int main()
{
	__asm__("movq $3405688917, %r15");
	printf("Test\n");
	custom_printf("Test2\n");
	__asm("movq $3405647957, %r15");
}
