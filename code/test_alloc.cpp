#include <MyAllocator.h>
#include <iostream>
using namespace std;

int main(){
	MyAllocator a;
	a.constructor(40);
	for(int i=0;i<10;i++){
		a.allocate(13);
	}
	a.reset();
	a.allocate(4);
	a.allocate(4);
	cout<<a.getNumberOfChunks()<<endl;
	return 0;
}
