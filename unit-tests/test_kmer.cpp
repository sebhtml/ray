
#include <structures/Vertex.h>
#include <tests/unitTest.h>
#include <structures/Kmer.h>
#include <core/constants.h>
#include <set>
#include <core/common_functions.h>
#include <vector>
#include <assert.h>
#include <string>
#include <iostream>
using namespace std;

void test_addInEdge(){
	string a="AGCAAGTTAGCAACATCATATGAGTGCAATCCTGTTGTAGGCTCATCTAAGACATAAATAGTT";
	string b= "GCAAGTTAGCAACATCATATGAGTGCAATCCTGTTGTAGGCTCATCTAAGACATAAATAGTTT";
	int wordSize=a.length();

	Kmer aKmer=wordId(a.c_str());
	Kmer bKmer=wordId(b.c_str());

	Vertex bVertex;
	bVertex.constructor();
	bVertex.m_lowerKey=bKmer;
	bVertex.addIngoingEdge(&bKmer,&aKmer,wordSize);
	
	vector<Kmer>inEdges=bVertex.getIngoingEdges(&bKmer,wordSize);
	bool found=false;
	for(int j=0;j<(int)inEdges.size();j++){
		if(inEdges[j]==aKmer){
			found=true;
			break;
		}
	}
	if(!found){
		cout<<"Expected: "<<a<<endl;
		cout<<"Actual:"<<endl;
		cout<<inEdges.size()<<endl;
		for(int j=0;j<(int)inEdges.size();j++){
			cout<<idToWord(&(inEdges[j]),wordSize,false)<<endl;
		}
	}
	assertEquals(inEdges.size(),1);
	assertEquals(found,true);
}

void test_addOutEdge(){
	string a="CAATAAGTAAAAAAGATTTTGTAACTTTCACAGCCTTATTTTTATCAATAGATACTGATAT";
	string b= "AATAAGTAAAAAAGATTTTGTAACTTTCACAGCCTTATTTTTATCAATAGATACTGATATT";
	int wordSize=a.length();

	Kmer aKmer=wordId(a.c_str());
	Kmer bKmer=wordId(b.c_str());

	Vertex aVertex;
	aVertex.constructor();
	Kmer lower=aKmer;
	Kmer aRC=complementVertex(&aKmer,wordSize,false);

	if(aRC<lower){
		lower=aRC;
	}
	aVertex.m_lowerKey=lower;
	aVertex.addOutgoingEdge(&aKmer,&bKmer,wordSize);
	
	vector<Kmer>Edges=aVertex.getOutgoingEdges(&aKmer,wordSize);
	bool found=false;
	for(int j=0;j<(int)Edges.size();j++){
		if(Edges[j]==bKmer){
			found=true;
			break;
		}
	}
	if(!found){
		cout<<"Expected: "<<endl;
		cout<<b<<endl;
		cout<<"Actual:"<<endl;
		for(int j=0;j<(int)Edges.size();j++){
			cout<<idToWord(&(Edges[j]),wordSize,false)<<endl;
		}
		uint8_t edges=aVertex.getEdges(&aKmer);
		cout<<"Edges"<<endl;
		print8(edges);
	}
	assertEquals(Edges.size(),1);
	assertEquals(found,true);
}

void test_addInEdge2(){
	string a="AGCAAGTTAGCAACATCATATGAGTGCAATCCTGTTGTAGGCTCATCTAAGACATAAATAGTT";
	string b= "GCAAGTTAGCAACATCATATGAGTGCAATCCTGTTGTAGGCTCATCTAAGACATAAATAGTTT";
	int wordSize=a.length();

	Kmer aKmer=wordId(a.c_str());
	Kmer bKmer=wordId(b.c_str());

	Vertex bVertex;
	bVertex.constructor();
	Kmer bRC=complementVertex(&bKmer,wordSize,false);
	Kmer lower=bKmer;
	Kmer aRC=complementVertex(&aKmer,wordSize,false);

	if(bRC<lower){
		lower=bRC;
	}
	bVertex.m_lowerKey=lower;
	bVertex.addIngoingEdge(&bKmer,&aKmer,wordSize);
	
	vector<Kmer>inEdges=bVertex.getIngoingEdges(&bKmer,wordSize);
	bool found=false;
	for(int j=0;j<(int)inEdges.size();j++){
		if(inEdges[j]==aKmer){
			found=true;
			break;
		}
	}
	if(!found){
		cout<<"Expected: "<<a<<endl;
		cout<<"Actual:"<<endl;
		cout<<inEdges.size()<<endl;
		for(int j=0;j<(int)inEdges.size();j++){
			cout<<idToWord(&(inEdges[j]),wordSize,false)<<endl;
		}
	}
	assertEquals(inEdges.size(),1);
	assertEquals(found,true);
}

void test_out_large(){
	string a="TCAAAAATTTCTTTCAAAGTAATCTCATAAGCTGCTGGA";
	string b= "CAAAAATTTCTTTCAAAGTAATCTCATAAGCTGCTGGAT";
	int wordSize=a.length();

	// description of m_edges:
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0
	
	uint8_t edges=(1<<(4+RAY_NUCLEOTIDE_T));

	Kmer aKmer=wordId(a.c_str());
	Kmer bKmer=wordId(b.c_str());
	
	vector<Kmer>oEdges=_getOutgoingEdges(&aKmer,edges,wordSize);
	assertEquals(oEdges.size(),1);

	Kmer actual=oEdges[0];
	string actualStr=idToWord(&actual,wordSize,false);
	if(actualStr!=b){
		cout<<"MAXKMERLENGTH: "<<MAXKMERLENGTH<<endl;
		cout<<"WordSize: "<<wordSize<<endl;
		cout<<"Expected"<<endl;
		cout<<a<<" -> "<<b<<endl;
		cout<<"Actual:"<<endl;
		cout<<a<<" -> "<<actualStr<<"*"<<endl;
		cout<<endl;
	}
	assertEquals(actualStr,b);

	if(actual!=bKmer){
		cout<<"MAXKMERLENGTH: "<<MAXKMERLENGTH<<endl;
		cout<<"WordSize: "<<wordSize<<endl;
		cout<<"Expected: "<<endl;
		bKmer.print();
		cout<<"Actual: "<<endl;
		actual.print();
	}
	assert(actual==bKmer);
}

void test_Ingoing_large2(){
	string a="AGCAAGTTAGCAACATCATATGAGTGCAATCCTGTTGTAGGCTCATCTAAGACATAAATAGTT";
	string b= "GCAAGTTAGCAACATCATATGAGTGCAATCCTGTTGTAGGCTCATCTAAGACATAAATAGTTT";
	int wordSize=a.length();

	// description of m_edges:
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0
	
	uint8_t edges=(1<<0);

	Kmer aKmer=wordId(a.c_str());
	assertEquals(getFirstSegmentFirstCode(&aKmer,wordSize),RAY_NUCLEOTIDE_A);

	Kmer bKmer=wordId(b.c_str());
	
	vector<Kmer>inEdges=_getIngoingEdges(&bKmer,edges,wordSize);
	Kmer actual=inEdges[0];
	string actualStr=idToWord(&actual,wordSize,false);
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




void test_Ingoing_large(){
	string a="TCAAAAATTTCTTTCAAAGTAATCTCATAAGCTGCTGGA";
	string b= "CAAAAATTTCTTTCAAAGTAATCTCATAAGCTGCTGGAT";
	int wordSize=a.length();

	// description of m_edges:
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0
	
	uint8_t edges=(1<<RAY_NUCLEOTIDE_T);

	Kmer aKmer=wordId(a.c_str());
	Kmer bKmer=wordId(b.c_str());
	
	vector<Kmer>inEdges=_getIngoingEdges(&bKmer,edges,wordSize);
	Kmer actual=inEdges[0];
	string actualStr=idToWord(&actual,wordSize,false);
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



void test_out(){
	string a="GACTTGATTAGACAAGAAGTT";
	string b= "ACTTGATTAGACAAGAAGTTG";
	int wordSize=a.length();

	// description of m_edges:
	// outgoing  ingoing
	//
	
	uint8_t edges=(1<<(4+RAY_NUCLEOTIDE_G));

	Kmer aKmer=wordId(a.c_str());
	Kmer bKmer=wordId(b.c_str());
	
	vector<Kmer>oEdges=_getOutgoingEdges(&aKmer,edges,wordSize);
	assertEquals(oEdges.size(),1);

	Kmer actual=oEdges[0];
	string actualStr=idToWord(&actual,wordSize,false);
	if(actualStr!=b){
		cout<<"MAXKMERLENGTH: "<<MAXKMERLENGTH<<endl;
		cout<<"WordSize: "<<wordSize<<endl;
		cout<<"Expected"<<endl;
		cout<<a<<" -> "<<b<<endl;
		cout<<"Actual:"<<endl;
		cout<<a<<" -> "<<actualStr<<"*"<<endl;
		cout<<endl;
	}
	assertEquals(actualStr,b);

	if(actual!=bKmer){
		cout<<"MAXKMERLENGTH: "<<MAXKMERLENGTH<<endl;
		cout<<"WordSize: "<<wordSize<<endl;
		cout<<"Expected: "<<endl;
		bKmer.print();
		cout<<"Actual: "<<endl;
		actual.print();
	}
	assert(actual==bKmer);
}

void test_Ingoing(){
	string a="GACTTGATTAGACAAGAAGTT";
	string b= "ACTTGATTAGACAAGAAGTTG";
	int wordSize=a.length();

	// description of m_edges:
	// outgoing  ingoing
	//
	// T G C A T G C A
	//
	// 7 6 5 4 3 2 1 0
	
	uint8_t edges=(1<<RAY_NUCLEOTIDE_G);

	Kmer aKmer=wordId(a.c_str());
	Kmer bKmer=wordId(b.c_str());
	
	vector<Kmer>inEdges=_getIngoingEdges(&bKmer,edges,wordSize);
	Kmer actual=inEdges[0];
	string actualStr=idToWord(&actual,wordSize,false);
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
	string seq=argv[1];
	int wordSize=seq.length();

	Kmer id=wordId(seq.c_str());

	Kmer empty;
	string result=idToWord(&id,wordSize,false);
	assert(seq==result);
	char last=seq[seq.length()-1];
	char observed=getLastSymbol(&id,wordSize,false);
	assert(observed==last);


	// reverse complement
	string rc=reverseComplement(&seq);

	Kmer comp=complementVertex(&id,wordSize,false);
	string result2=idToWord(&comp,wordSize,false);
	assertEquals(rc,result2);

	Kmer rcId=wordId(rc.c_str());
	assert(rcId==comp);
	
	// ingoing edges.
	//
	uint8_t edges=0xff;
	vector<Kmer>inEdges=_getIngoingEdges(&id,edges,wordSize);
	assertEquals(4,inEdges.size());
	set<string> tmp;
	for(int i=0;i<(int)inEdges.size();i++){
		Kmer theKmer=inEdges[i];
		string a=idToWord(&theKmer,wordSize,false);
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
		string a=idToWord(&theKmer,wordSize,false);

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
	}

	assertEquals(tmp.size(),4);

	test_Ingoing();
	test_out();

	if(MAXKMERLENGTH>32){
		test_Ingoing_large();
		test_Ingoing_large2();
		test_out_large();
		test_addInEdge();
		test_addInEdge2();
		test_addOutEdge();
	}
	return 0;
}

