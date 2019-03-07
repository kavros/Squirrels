#include <stdio.h>
#include "squirrel-functions.h"
int main()
{
	long seed = -1;
	initialiseRNG(&seed);
	printf("Hello World !!\n");
	return 0;
}