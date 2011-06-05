
#include <common_functions.h>
#include <stdint.h>
#include <iostream>
using namespace std;

void print(uint8_t a){
	cout<<"OUT     IN"<<endl;
	cout<<"G C T A G C T A"<<endl;
	for(int i=7;i>=0;i--){
		cout<<i<<" ";
	}
	cout<<endl;
	for(int i=7;i>=0;i--){
		int b=((((uint64_t)a)<<((sizeof(uint64_t)*8-1)-i))>>(sizeof(uint64_t)*8-1));
		cout<<b<<" ";
	}
	cout<<endl;
}

int main(){
	uint8_t edges=5;
	uint8_t res=invertEdges(edges);
	cout<<"In="<<endl;
	print(edges);
	cout<<"Out="<<endl;
	print(res);
	return 0;
}
