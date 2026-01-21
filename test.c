#include <stdio.h>
#include <stdlib.h>

int foo()
{
	fprintf(stdout,"%s:%d HELLO []\n",__FILE__,__LINE__);
	
	return 5+3;
}

int main()
{
	fprintf(stdout,"%s:%d HELLO [WORLD]\n",__FILE__,__LINE__);
	
	fprintf(stdout,"%s:%d NEXT [%d]\n",__FILE__,__LINE__,foo());
	
	return 0;
}
