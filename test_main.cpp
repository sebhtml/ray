/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include<set>
#include<Vertex.h>
#include<common_functions.h>
#include<iostream>
#include<assert.h>
using namespace std;

void test_segment(string x,int segment){
	bool color=true;
	string word=x;
	int w=x.length();
	VERTEX_TYPE word64=wordId(word.c_str());
	char s1_first=word[0];
	char s1_last=word[segment-1];
	char s2_first=word[w-segment];
	char s2_last=word[w-1];
	
	assert(getFirstSegmentLastCode(word64,w,segment)==charToCode(s1_last));
	assert(getFirstSegmentFirstCode(word64,w,segment)==charToCode(s1_first));

	assert(getSecondSegmentFirstCode(word64,w,segment)==charToCode(s2_first));
	assert(getSecondSegmentLastCode(word64,w,segment)==charToCode(s2_last));

}

void test_vertex(string x){
	Vertex v;
	v.constructor();
	bool color=true;
	int w=x.length()-1;
	string prefix=x.substr(0,w);
	string suffix=x.substr(1);
	VERTEX_TYPE p=wordId(prefix.c_str());
	VERTEX_TYPE s=wordId(suffix.c_str());
	v.addOutgoingEdge(wordId(suffix.c_str()),w);
	vector<VERTEX_TYPE> out=v.getOutgoingEdges(p,w);
	assert(out.size()==1);
	if(out[0]!=s){
		cout<<"Expected"<<endl;
		coutBIN(s);
		cout<<"Observed"<<endl;
		coutBIN(out[0]);
	}
	assert(out[0]==s);
	Vertex v2;
	v2.constructor();
	v2.addIngoingEdge(p,w);
	
	vector<VERTEX_TYPE> in=v2.getIngoingEdges(s,w);
	assert(in.size()==1);
	if(in[0]!=p){
		cout<<"Expected"<<endl;
		coutBIN(p);
		cout<<"Observed"<<endl;
		coutBIN(in[0]);
	}
	assert(in[0]==p);

	
}

void test_vertex2(){
	bool color=true;
	string prefix="ATGGAAAAAAATGAGAATGCAC";
	string suffix1="TGGAAAAAAATGAGAATGCACG";
	string suffix2="TGGAAAAAAATGAGAATGCACA";
	VERTEX_TYPE p=wordId(prefix.c_str());
	VERTEX_TYPE s1=wordId(suffix1.c_str());
	VERTEX_TYPE s2=wordId(suffix2.c_str());
	Vertex v0;
	v0.constructor();
	int w=prefix.length();
	v0.addOutgoingEdge(s1,w);
	v0.addOutgoingEdge(s2,w);
	vector<VERTEX_TYPE> out=v0.getOutgoingEdges(p,w);
	assert(out.size()==2);
	set<VERTEX_TYPE> o;
	o.insert(out[0]);
	o.insert(out[1]);
	assert(o.count(s1)>0);
	assert(o.count(s2)>0);
	v0.addOutgoingEdge(s1,w);
	assert(out.size()==2);
}

void test3(){
	string p="CGCCTTTAAAAAAAAAAAGGCTCGA";
	string s= "GCCTTTGAAAAAAAAAAAGCTCGAT";
	Vertex v;
	v.constructor();
	int w=p.length();
	VERTEX_TYPE p1=wordId(p.c_str(),w);
	VERTEX_TYPE s1=wordId(s.c_str(),w);
	bool color=true;
	v.addOutgoingEdge(s1,w);
	vector<VERTEX_TYPE> o=v.getOutgoingEdges(p1,w);
	assert(o.size()==1);
	assert(o[0]==s1);
}

int main(){
	test_segment("ATCAGTTGCAGTACTGCAATCTACG",7);
	test_segment("GTCAGTTGCAGTACTGCAATTTACG",5);
	test_segment("TCTCTCTCTCTCTCTCTCTCTCT",7);
	test_segment("TATATATATATATATATATATAT",7);
	test_segment("GGGGGGGGGGGGGGGGGGG",7);
	test_segment("CCCCCCCCCCCCCCCCCCCC",7);
	test_vertex("ATCAGTTGCAGTACTGCAATCTACG");
	test_vertex("TTCAGTTGCAGTACTGCATTCTACG");
	test_vertex("TCTCTCTCTCTCTCTCTCTCTCT");
	test_vertex("ATCCGATCTACGCAATCACACA");
	
	test_vertex2();
	test3();
	return 0;
}
