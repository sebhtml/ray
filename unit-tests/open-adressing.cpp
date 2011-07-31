#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <iostream>
using namespace std;

/**
 * make sure that double hashing can probe all the M buckets in M probing events
 */
int main(){
	uint64_t h1=392176;
	uint64_t h2=937741;
	uint64_t buckets=2097152;
	
	int*hits=(int*)malloc(sizeof(int)*buckets);
	for(int bucket=0;bucket<buckets;bucket++)
		hits[bucket]=0;

	for(uint64_t probe=0;probe<buckets;probe++){
		uint64_t bucket=(h1+probe*h2)%buckets;
		cout<<"ProbedBucket "<<bucket<<endl;
		hits[bucket]++;
	}

	int at1=0;
	for(int bucket=0;bucket<buckets;bucket++){
		if(hits[bucket]==1)
			at1++;
	}
	
	assert(at1==buckets);

	return 0;
}
