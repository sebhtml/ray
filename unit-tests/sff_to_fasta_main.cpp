#include <assembler/Loader.h>
#include <structures/Read.h>
#include <stdint.h>
#include <iostream>
using namespace std;

int main(int argc,char**argv){
	if(argc!=2){
		cout<<"Provide an SFF file."<<endl;
		return 0;
	}
	string file=argv[1];
	Loader loader;
	loader.constructor("",false);
	loader.load(file,false);
	char read[4096];
	for(uint64_t i=0;i<loader.size();i++){
		loader.at(i)->getSeq(read,false,false);
		cout<<">"<<i<<endl<<read<<endl;
	}
	return 0;
}
