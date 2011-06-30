#include <core/common_functions.h>
#include <structures/Kmer.h>
#include <map>
#include <stdlib.h>
#include <time.h>
#include <iostream>
using namespace std;

int computeAverage(vector<int>*a){
	uint64_t sum;
	for(int i=0;i<(int)a->size();i++)
		sum+=a->at(i);
	return sum/a->size();
}

void f2(){
	srand(time(NULL));
	int size=1536;
	map<int,int> counts;
	
	uint64_t samples=100000000;
	uint64_t base=rand();
	int wordSize=63;
	Kmer kmer;
	kmer.setU64(0,base);

	int average=samples/size;
	while(samples--){
		uint64_t second=rand();
		kmer.setU64(1,second);
		int rank=vertexRank(&kmer,size,wordSize,false);
		counts[rank]++;
	}
	vector<int> data;
	for(int i=0;i<size;i++){
		data.push_back(counts[i]);
		//cout<<i<<" "<<counts[i]<<endl;
	}
	int deviation=average/10;
	int min=average-deviation;
	int max=average+deviation;
	for(int i=0;i<size;i++){
		if(counts[i]>=max){
			cout<<counts[i]<<" and Max="<<max<<endl;
		}
		assert(counts[i]<max);
		assert(counts[i]>min);
	}
}

void f1(){
	srand(time(NULL));
	int size=1536;
	map<int,int> counts;
	
	uint64_t samples=100000000;
	uint64_t base=rand();
	int wordSize=63;
	Kmer kmer;
	kmer.setU64(0,base);

	int average=samples/size;
	while(samples--){
		uint64_t first=rand();
		uint64_t second=rand();
		kmer.setU64(0,first);
		kmer.setU64(1,second);
		int rank=vertexRank(&kmer,size,wordSize,false);
		counts[rank]++;
	}
	vector<int> data;
	for(int i=0;i<size;i++){
		data.push_back(counts[i]);
		//cout<<i<<" "<<counts[i]<<endl;
	}
	int deviation=average/10;
	int min=average-deviation;
	int max=average+deviation;
	for(int i=0;i<size;i++){
		if(counts[i]>=max){
			cout<<counts[i]<<" and Max="<<max<<endl;
		}
		assert(counts[i]<max);
		assert(counts[i]>min);
	}
}

int main(){
	f1();
	f2();
	return EXIT_SUCCESS;
}
