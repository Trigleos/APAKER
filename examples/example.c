#include <stdio.h>
#include <stdlib.h>
//nanomite section start marker: __asm__("movq $3405688917, %r15");
//nanomite section end marker: __asm__("movq $3405647957, %r15");



int custom_printf(char* input)
{
	__asm__("movq $3405688917, %r15");
	printf("This is custom\n");
	printf("%s",input);
	__asm__("movq $3405647957, %r15");
	return 1;
}

int main()
{
	__asm__("movq $3405688917, %r15");
	printf("Test\n");
	int l = 3;
	l *= 5;
	printf("%d\n",l);
	__asm__("movq $3405647957, %r15");
	custom_printf("Test2\n");
}
