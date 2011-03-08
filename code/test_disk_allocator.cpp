#include <OnDiskAllocator.h>

int main(){
	OnDiskAllocator allocator;
	allocator.constructor(1024*1024*1024);
	int*array=(int*)allocator.allocate(sizeof(int)*1000000);

	for(int i=0;i<1000000;i++){
		array[i]=i*2+1;
	}

	sleep(10);

	allocator.clear();

	return 0;
}
