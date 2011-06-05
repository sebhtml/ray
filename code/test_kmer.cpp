

#include <Kmer.h>
#include <common_functions.h>
#include <assert.h>
#include <string>
#include <iostream>
using namespace std;


int main(int argc,char**argv){
	//cout<<endl;
	//cout<<"MAXKMERLENGTH="<<MAXKMERLENGTH<<endl;
	string seq=argv[1];
	//cout<<"Seq="<<endl;
	int wordSize=seq.length();

	Kmer id=wordId(seq.c_str());
	//cout<<"Kmer="<<endl;
	//cout<<"idToWord="<<endl;
	//cout<<idToWord(&id,wordSize);
	//cout<<endl;

	Kmer empty;
	//cout<<idToWord(&empty,wordSize);
	string result=idToWord(&id,wordSize);
	//cout<<seq<<endl;
	//if(seq!=result){
		for(int i=0;i<seq.length();i++){
			//cout<<" "<<seq[i]<<" ";
		}
/*
		cout<<endl;
		id.print();
		cout<<result<<endl;
*/
	//}
	assert(seq==result);
	char last=seq[seq.length()-1];
	char observed=getLastSymbol(&id,wordSize);
	assert(observed==last);
	return 0;
}

