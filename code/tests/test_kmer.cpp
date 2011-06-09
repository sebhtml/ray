
#include <tests/unitTest.h>
#include <structures/Kmer.h>
#include <set>
#include <core/common_functions.h>
#include <assert.h>
#include <string>
#include <iostream>
using namespace std;

void test_Ingoing(){
	string a="GACTTGATTAGACAAGAAGTT";
	string b= "ACTTGATTAGACAAGAAGTTG";
	int wordSize=a.length();

	// description of m_edges:
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0
	
	uint8_t edges=(1<<3);

	Kmer aKmer=wordId(a.c_str());
	Kmer bKmer=wordId(b.c_str());
	
	vector<Kmer>inEdges=_getIngoingEdges(&bKmer,edges,wordSize);
	Kmer actual=inEdges[0];
	string actualStr=idToWord(&actual,wordSize);
	if(actualStr!=a){
		cout<<"MAXKMERLENGTH: "<<MAXKMERLENGTH<<endl;
		cout<<"WordSize: "<<wordSize<<endl;
		cout<<"Expected"<<endl;
		cout<<a<<" -> "<<b<<endl;
		cout<<"Actual:"<<endl;
		cout<<actualStr<<" -> "<<b<<endl;
		cout<<endl;
	}
	assertEquals(actualStr,a);

	if(actual!=aKmer){
		cout<<"MAXKMERLENGTH: "<<MAXKMERLENGTH<<endl;
		cout<<"WordSize: "<<wordSize<<endl;
		cout<<"Expected: "<<endl;
		aKmer.print();
		cout<<"Actual: "<<endl;
		actual.print();
	}
	assert(actual==aKmer);
}


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
		Kmer theKmer=inEdges[i];
		string a=idToWord(&theKmer,wordSize);
		Kmer id=wordId(a.c_str());

		if(theKmer!=id){
			cout<<"MAXKMERLENGTH: "<<MAXKMERLENGTH<<endl;
			cout<<"WordSize: "<<wordSize<<endl;
			cout<<"Expected: "<<endl;
			id.print();
			cout<<"Actual: "<<endl;
			theKmer.print();
		}

		assert(id==theKmer);

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
		Kmer theKmer=outEdges[i];
		string a=idToWord(&theKmer,wordSize);

		Kmer id=wordId(a.c_str());// make sure that all bit are set to 0 except those relevant 
		if(theKmer!=id){
			cout<<"Expected: "<<endl;
			id.print();
			cout<<"Actual: "<<endl;
			theKmer.print();
		}
		assert(theKmer==id);
		assertEquals(tmp.count(a),0);
		tmp.insert(a);
		assertEquals(seq.substr(1,wordSize-1),a.substr(0,wordSize-1));
		//cout<<seq<<" -> "<<a<<endl;
	}

	assertEquals(tmp.size(),4);

	test_Ingoing();
	return 0;
}

