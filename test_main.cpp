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

#include<fstream>
#include<CoverageDistribution.h>
#include<set>
#include<Vertex.h>
#include<common_functions.h>
#include<MyAllocator.h>
#include<iostream>
#include<assert.h>
using namespace std;

void test_segment(string x,int segment){
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
	int w=x.length()-1;
	MyAllocator allocator;
	allocator.constructor(10000);
	string prefix=x.substr(0,w);
	string suffix=x.substr(1);
	VERTEX_TYPE p=wordId(prefix.c_str());
	VERTEX_TYPE s=wordId(suffix.c_str());
	v.addOutgoingEdge(wordId(suffix.c_str()),w,&allocator);
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
	v2.addIngoingEdge(p,w,&allocator);
	
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
	MyAllocator allocator;
	allocator.constructor(10000);
	string prefix="ATGGAAAAAAATGAGAATGCAC";
	string suffix1="TGGAAAAAAATGAGAATGCACG";
	string suffix2="TGGAAAAAAATGAGAATGCACA";
	VERTEX_TYPE p=wordId(prefix.c_str());
	VERTEX_TYPE s1=wordId(suffix1.c_str());
	VERTEX_TYPE s2=wordId(suffix2.c_str());
	Vertex v0;
	v0.constructor();
	int w=prefix.length();
	v0.addOutgoingEdge(s1,w,&allocator);
	v0.addOutgoingEdge(s2,w,&allocator);
	vector<VERTEX_TYPE> out=v0.getOutgoingEdges(p,w);
	assert(out.size()==2);
	set<VERTEX_TYPE> o;
	o.insert(out[0]);
	o.insert(out[1]);
	assert(o.count(s1)>0);
	assert(o.count(s2)>0);
	v0.addOutgoingEdge(s1,w,&allocator);
	assert(out.size()==2);
}

void test3(){
	MyAllocator allocator;
	allocator.constructor(10000);
	string p="CGCCTTTGAAAAAAAAAAGGCTCGA";
	string s= "GCCTTTGAAAAAAAAAAGGCTCGAT";
	Vertex v;
	v.constructor();
	int w=p.length();
	VERTEX_TYPE p1=wordId(p.c_str());
	VERTEX_TYPE s1=wordId(s.c_str());
	v.addOutgoingEdge(s1,w,&allocator);
	vector<VERTEX_TYPE> o=v.getOutgoingEdges(p1,w);
	assert(o.size()==1);
	assert(o[0]==s1);
	assert(o.size()>0);
}

void test_coverage(string file){
	ifstream f(file.c_str());
	bool t=f;
	f.close();
	if(!t)
		return;
	map<int,VERTEX_TYPE> c;
	ifstream f2(file.c_str());
	int a;
	VERTEX_TYPE b;
	string buffer;
	f2>>buffer>>buffer;
	while(!f2.eof()){
		a=0;
		f2>>a>>b;
		if(a==0)
			break;
		c[a]=b;
	}
	string file2="/dev/null";
	CoverageDistribution d(&c,&file2);
	int m=d.getMinimumCoverage();
	int p=d.getPeakCoverage();
	assert(m<p);
	cout<<(file)<<endl;
	cout<<m<<" "<<p<<endl;
	f2.close();
}

void test_coverageDistribution(){
	test_coverage("./scripts/0mix11-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("/data/users/sra/solidsoftwaretools.com/dh10bfrag/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix12-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix14-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix21-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix22-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix23-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix31-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix32-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix13-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0mix33-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0short1-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0short2-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0sim5-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0sim8-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0sim1-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0sim6-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0sim4-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0short3-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0sim2-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0sim7-ray.sh.dir/Ray-CoverageDistribution.txt");
	test_coverage("./scripts/0sim3-ray.sh.dir/Ray-CoverageDistribution.txt");
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
	test_coverageDistribution();

	return 0;
}
