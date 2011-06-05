
#include <tests/unitTest.h>
#include <structures/Kmer.h>
#include <set>
#include <core/common_functions.h>
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


	// reverse complement
	string rc=reverseComplement(&seq);

	Kmer comp=complementVertex(&id,wordSize,false);
	string result2=idToWord(&comp,wordSize);
	assertEquals(rc,result2);
	
/*
	for(int i=0;i<seq.length();i++){
		cout<<" "<<seq[i]<<" ";
	}
	cout<<endl;
*/

	// ingoing edges.
	//
	uint8_t edges=0xff;
	vector<Kmer>inEdges=_getIngoingEdges(&id,edges,wordSize);
	assertEquals(4,inEdges.size());
	set<string> tmp;
	for(int i=0;i<(int)inEdges.size();i++){
		//cout<<"Entry "<<i<<" "<<endl;
		//inEdges[i].print();
		string a=idToWord(&(inEdges[i]),wordSize);
		assertEquals(tmp.count(a),0);
		tmp.insert(a);
		assertEquals(a.substr(1,wordSize-1),seq.substr(0,wordSize-1));
	}
	assertEquals(tmp.size(),4);

	// test outgoing edges
	vector<Kmer> outEdges=_getOutgoingEdges(&id,edges,wordSize);
	assertEquals(4,outEdges.size());
	tmp.clear();
	for(int i=0;i<(int)outEdges.size();i++){
		string a=idToWord(&(outEdges[i]),wordSize);

		assertEquals(tmp.count(a),0);
		tmp.insert(a);
		assertEquals(seq.substr(1,wordSize-1),a.substr(0,wordSize-1));
		//cout<<seq<<" -> "<<a<<endl;
	}

	assertEquals(tmp.size(),4);
	
	return 0;
}

