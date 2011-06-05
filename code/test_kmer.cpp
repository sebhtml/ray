#include <Kmer.h>
#include <common_functions.h>
#include <string>
#include <iostream>
using namespace std;

#define MAXKMERLENGTH 128

int main(){
	string seq="CATGTATTTATCTAGAGAACTAACAGATAATAGTCTTCCAAAAATTGGGAAGGAATTTGGGGGAAAAGATCATGTATTTATCTAGAGAACTAACAGATAATAGTCTTCCAAA";
	cout<<"Seq="<<endl;
	cout<<seq<<endl;

	Kmer id=wordId(seq.c_str());
	cout<<"Kmer="<<endl;
	id.print();
	return 0;
}

