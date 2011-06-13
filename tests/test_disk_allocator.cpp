#include <OnDiskAllocator.h>

int main(){
	OnDiskAllocator allocator;
	allocator.constructor();
	int p=10;
	while(p--){
		int*array=(int*)allocator.allocate(sizeof(int)*10000000);

		for(int i=0;i<10000000;i++){
			array[i]=i*2+1;
		}
	}

	sleep(1000);

	allocator.clear();

	return 0;
}
