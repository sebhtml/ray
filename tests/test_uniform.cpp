#include <core/common_functions.h>
#include <structures/Kmer.h>
#include <time.h>
#include <iostream>
using namespace std;

int main(){
	srand(time(NULL));
	int size=1536;
	map<int,int> counts;
	
	uint64_t samples=1000000;
	uint64_t base=rand();
	int wordSize=63;
	Kmer kmer;
	kmer.setU64(0,base);

	while(samples--){
		uint64_t first=rand();
		uint64_t second=rand();
		kmer.setU64(0,first);
		kmer.setU64(1,second);
		int rank=vertexRank(&kmer,size,wordSize);
		counts[rank]++;
	}
	for(int i=0;i<size;i++){
		cout<<i<<" "<<counts[i]<<endl;
	}
	return EXIT_SUCCESS;
}
