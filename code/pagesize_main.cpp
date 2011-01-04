#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(){
	long sz = sysconf(_SC_PAGESIZE);
	printf("_SC_PAGESIZE= %lu\n",sz);
	return EXIT_SUCCESS;
}

