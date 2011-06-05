/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/


#include <crypto.h>
#include <assert.h>
#include <stdio.h>

#ifdef HAVE_CLOCK_TIME
#include <sys/time.h>
#endif

#include <vector>
#include <malloc_types.h>
#include <fstream>
#include <common_functions.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
using namespace std;

char complementNucleotide(char c){
	switch(c){
		case 'A':
			return 'T';
		case 'T':
			return 'A';
		case 'G':
			return 'C';
		case 'C':
			return 'G';
		default:
			return c;
	}
}

string reverseComplement(string*a){
	ostringstream b;
	for(int i=a->length()-1;i>=0;i--){
		b<<complementNucleotide((*a)[i]);
	}
	return b.str();
}

// convert k-mer to uint64_t
Kmer wordId(const char*a){
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	return wordId_DistantSegments(a);
	#endif
	return wordId_Classic(a);

}

Kmer wordId_Classic(const char*a){
	Kmer i;
	//i.print();
	int theLen=strlen(a);
	for(int j=0;j<(int)theLen;j++){
		uint64_t k=_ENCODING_A; // default is A
		char h=a[j];
		switch(h){
			case 'T':
				k=_ENCODING_T;
				break;
			case 'C':
				k=_ENCODING_C;
				break;
			case 'G':
				k=_ENCODING_G;
				break;
		}
		int bitPosition=2*j;
		int chunk=bitPosition/64;
		int bitPositionInChunk=bitPosition%64;
		//cout<<"bitPositionInChunk="<<bitPositionInChunk<<" code="<<k<<endl;
		#ifdef ASSERT
		if(!(chunk<i.getNumberOfU64())){
			cout<<"Chunk="<<chunk<<" positionInKmer="<<j<<" KmerLength="<<strlen(a)<<" bitPosition=" <<bitPosition<<" Chunks="<<i.getNumberOfU64()<<endl;
		}
		assert(chunk<i.getNumberOfU64());
		#endif
		uint64_t filter=(k<<bitPositionInChunk);
		//i.print();
		i.setU64(chunk,i.getU64(chunk)|filter);
	}
	return i;
}

uint64_t wordId_DistantSegments(const char*a){
	uint64_t i=0;
	int len=strlen(a);
	for(int j=0;j<(int)strlen(a);j++){
		uint64_t k=_ENCODING_A; // default is A
		char h='A';
		if(j<_SEGMENT_LENGTH or j>len-_SEGMENT_LENGTH-1){
			h=a[j];
		}
		switch(h){
			case 'T':
				k=_ENCODING_T;
				break;
			case 'C':
				k=_ENCODING_C;
				break;
			case 'G':
				k=_ENCODING_G;
				break;
		}
		i=(i|(k<<(j<<1)));
	}
	return i;

}

void printU64(uint64_t a){
	for(int i=0;i<64;i++){
		int bit=(a<<i)>>63;
		printf("%i",bit);
	}
	printf("\n");
}

/*
 *	7 6 5 4 3 2 1 0 
 *			7 6 5 4 3 2 1 0
 *
 * 	k=6
 */
string idToWord(const Kmer*i,int wordSize){
	char a[1000];
	for(int p=0;p<wordSize;p++){
		int bitPosition=2*p;
		int chunkId=bitPosition/64;
		int bitPositionInChunk=(bitPosition%64);
		uint64_t chunk=i->getU64(chunkId);
		//printU64(chunk);
		uint64_t j=(chunk<<(62-bitPositionInChunk))>>62; // clear the bits.
		//cout<<"Position="<<p<<" Chunk "<<chunkId<<" BitInChunk="<<bitPositionInChunk<<" code="<<j<<endl;
		switch(j){
			case _ENCODING_A:
				a[p]='A';
				break;
			case _ENCODING_T:
				a[p]='T';
				break;
			case _ENCODING_C:
				a[p]='C';
				break;
			case _ENCODING_G:
				a[p]='G';
				break;
			default:
				break;
		}
	}
	a[wordSize]='\0';
	string b(a);
	return b;
}

//  63 (sizeof(uint64_t)*8-2) ... 1 0
//              
char getFirstSymbol(uint64_t i,int k){
	i=(i<<((sizeof(uint64_t)*8-2)))>>(sizeof(uint64_t)*8-2); // clear bits
        if((int)i==_ENCODING_A)
                return 'A';
        if((int)i==_ENCODING_T)
                return 'T';
        if((int)i==_ENCODING_C)
                return 'C';
        if((int)i==_ENCODING_G)
                return 'G';
	return '0';
}

char getLastSymbol(Kmer*i,int m_wordSize){
	int bitPosition=2*m_wordSize;
	int chunkId=bitPosition/64;
	int bitPositionInChunk=bitPosition%64;
	uint64_t chunk=i->getU64(chunkId);
	chunk=(chunk<<(sizeof(uint64_t)*8-bitPositionInChunk))>>(sizeof(uint64_t)*8-2); // clecar bits

	// TODO: replace by a switch
        if((int)chunk==_ENCODING_A)
                return 'A';
        if((int)chunk==_ENCODING_T)
                return 'T';
        if((int)chunk==_ENCODING_C)
                return 'C';
        if((int)chunk==_ENCODING_G)
                return 'G';
	cout<<"Fatal exception, getLastSymbol."<<endl;
	exit(0);
}

bool isValidDNA(const char*x){
	int len=strlen(x);
	for(int i=0;i<len;i++){
		char a=x[i];
		if(!(a=='A'||a=='T'||a=='C'||a=='G'))
			return false;
	}
	return true;
}

/*
 * 
 *   63 (sizeof(uint64_t)*8-2) ... 1 0
 *
 */

void coutBIN(uint64_t a){
	for(int i=sizeof(uint64_t)*8-1;i>=0;i--){
		cout<<(int)((a<<(sizeof(uint64_t)*8-1-i))>>(sizeof(uint64_t)*8-1));
	}
	cout<<endl;
}
void coutBIN8(uint8_t a){
	for(int i=7;i>=0;i--){
		cout<<(int)((a<<(7-i))>>7);
	}
	cout<<endl;
}

uint64_t getKPrefix(uint64_t a,int k){
	return (a<<(sizeof(uint64_t)*8-2*(k+1)+2))>>(sizeof(uint64_t)*8-2*(k+1)+2); // move things around...
}

uint64_t getKSuffix(uint64_t a,int k){
	return a>>2;
}

Kmer complementVertex(Kmer*a,int b,bool colorSpace){
	if(colorSpace){
		return complementVertex_colorSpace(a,b);
	}
	return complementVertex_normal(a,b);
}

Kmer complementVertex_colorSpace(Kmer*a,int wordSize){
	Kmer output;
	int position2=0;
	for(int positionInMer=wordSize-1;positionInMer>=0;positionInMer--){
		int bitPosition=positionInMer*2;
		int u64_id=bitPosition/64;
		int bitPositionInChunk=bitPosition%64;
		uint64_t chunk=a->getU64(u64_id);
		uint64_t complementVertex=(chunk<<((sizeof(uint64_t)*8-2)-bitPositionInChunk))>>(sizeof(uint64_t)*8-2);

		uint64_t oldValue=output.getU64(u64_id);
		oldValue=(oldValue|(complementVertex<<(position2)));
		position2+=2;
		if(position2>=64){
			position2=0;
		}
	}

	return output;
}

/*
 *	127..64
 * 			63..0
 *
 *
 */
Kmer complementVertex_normal(Kmer*a,int m_wordSize){
	Kmer output;
	int position2=0;
	for(int positionInMer=m_wordSize-1;positionInMer>=0;positionInMer--){
		int bitPosition=positionInMer*2;
		int u64_id=bitPosition/64;
		int bitPositionInChunk=bitPosition%64;
		uint64_t chunk=a->getU64(u64_id);
		uint64_t j=(chunk<<((sizeof(uint64_t)*8-2)-bitPositionInChunk))>>(sizeof(uint64_t)*8-2);
		uint64_t complementVertex=0;
		switch(j){
			case _ENCODING_A:
				complementVertex=_ENCODING_T;
				break;
			case _ENCODING_T:
				complementVertex=_ENCODING_A;
				break;
			case _ENCODING_C:
				complementVertex=_ENCODING_G;
				break;
			case _ENCODING_G:
				complementVertex=_ENCODING_C;
				break;
			default:
				assert(false);
				break;
		}
		#ifdef USE_DISTANT_SEGMENTS_GRAPH
		
		if(positionInMer<_SEGMENT_LENGTH or positionInMer>m_wordSize-_SEGMENT_LENGTH-1){
		}else{
			complementVertex=_ENCODING_A;
		}
		#endif
		uint64_t oldValue=output.getU64(u64_id);
		oldValue=(oldValue|(complementVertex<<(position2)));
		position2+=2;
		if(position2>=64){
			position2=0;
		}
	}
	return output;
}

string addLineBreaks(string dna,int columns){
	ostringstream output;
	int j=0;
	while(j<(int)dna.length()){
		output<<dna.substr(j,columns)<<endl;
		j+=columns;
	}
	return output.str();
}

//#define MALLOC_DEBUG
void*__Malloc(int c,int mallocType){
	assert(c!=0);
	void*a=NULL;
	a=malloc(c);
	if(a==NULL){
		cout<<"Critical exception: The system is out of memory, returned NULL."<<endl;
	}
	assert(a!=NULL);

	#ifdef MALLOC_DEBUG
	printf("%s %i\t%s\t%i bytes, ret\t%p\t%s\n",__FILE__,__LINE__,__func__,c,a,MALLOC_TYPES[mallocType]);
	fflush(stdout);
	#endif
	return a;
}

void __Free(void*a,int mallocType){
	#ifdef MALLOC_DEBUG
	printf("%s %i\t%s\t%p\t%s\n",__FILE__,__LINE__,__func__,a,MALLOC_TYPES[mallocType]);
	fflush(stdout);
	#endif

	free(a);
}

uint8_t getSecondSegmentLastCode(Kmer* v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//               6 5 4 3 2 1 0
	uint64_t a=v->getU64(0);
	a=a<<(sizeof(uint64_t)*8-(totalLength*2));
	a=(a>>(sizeof(uint64_t)*8-2));// restore state
	
	return a;
}

uint8_t getFirstSegmentFirstCode(Kmer*v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	uint64_t a=v->getU64(0);
	a=a<<(sizeof(uint64_t)*8-2);
	a=a>>(sizeof(uint64_t)*8-2);
	return a;
}

uint8_t charToCode(char a){
	if(a=='A')
		return _ENCODING_A;
	if(a=='T')
		return _ENCODING_T;
	if(a=='C')
		return _ENCODING_C;
	if(a=='G')
		return _ENCODING_G;
	return _ENCODING_A;
}

char codeToChar(uint8_t a){
	switch(a){
		case _ENCODING_A:
			return 'A';
		case _ENCODING_T:
			return 'T';
		case _ENCODING_C:
			return 'C';
		case _ENCODING_G:
			return 'G';
	}
	return 'A';
}

string convertToString(vector<Kmer>*b,int m_wordSize){
	ostringstream a;
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	//
	//TTAATT
	// TTAATT
	//  TTAATT
	//  the first vertex can not fill in the first delta letter alone, it needs help.
	for(int p=0;p<m_wordSize;p++){
		a<<codeToChar(getFirstSegmentFirstCode((*b)[p],_SEGMENT_LENGTH,m_wordSize));
	}
	#else
	a<<idToWord(&(*b)[0],m_wordSize);
	#endif
	for(int j=1;j<(int)(*b).size();j++){
		a<<getLastSymbol(&(*b)[j],m_wordSize);
	}
	string contig=a.str();
	return contig;
}

int vertexRank(Kmer*a,int _size,int w){
	return hash_function_1(*a,w)%(_size);
}

Kmer kmerAtPosition(const char*m_sequence,int pos,int w,char strand,bool color){
	int length=strlen(m_sequence);
	if(pos>length-w){
		cout<<"Fatal: offset is too large: position= "<<pos<<" Length= "<<length<<" WordSize=" <<w<<endl;
		exit(0);
	}
	if(pos<0){
		cout<<"Fatal: negative offset. "<<pos<<endl;
		exit(0);
	}
	if(strand=='F'){
		char sequence[100];
		memcpy(sequence,m_sequence+pos,w);
		sequence[w]='\0';
		Kmer v=wordId(sequence);
		return v;
	}else if(strand=='R'){
		char sequence[100];
		memcpy(sequence,m_sequence+length-pos-w,w);
		sequence[w]='\0';
		Kmer v=wordId(sequence);
		return complementVertex(&v,w,color);
	}
	Kmer error;
	return error;
}

int roundNumber(int s,int alignment){
	return ((s/alignment)+1)*alignment;
}

uint64_t getMilliSeconds(){
	uint64_t milliSeconds=0;
	#ifdef HAVE_CLOCK_GETTIME
	timespec temp;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&temp);
	uint64_t seconds=temp.tv_sec;
	uint64_t nanoseconds=temp.tv_nsec;
	milliSeconds=seconds*1000+nanoseconds/1000/1000;
	#endif
	return milliSeconds;
}

void showMemoryUsage(int rank){
	#ifdef linux
	ifstream f("/proc/self/status");
	while(!f.eof()){
		string key;
		f>>key;
		if(key=="VmData:"){
			uint64_t count;
			f>>count;
			printf("Rank %i: VmData= %lu KiB\n",rank,count);
			fflush(stdout);
			break;
		}
	}
	f.close();
	#endif
}

vector<Kmer> _getOutgoingEdges(Kmer*a,uint8_t edges,int k){
	vector<Kmer> b;
	Kmer aTemplate;
	aTemplate=*a;

	for(int i=0;i<aTemplate.getNumberOfU64();i++){
		uint64_t word=aTemplate.getU64(i)>>2;
		if(i!=aTemplate.getNumberOfU64()-1){
			uint64_t next=aTemplate.getU64(i+1);
/*
 *		abcd	efgh
 *		00ab	00ef
 *		00ab	cdef
 */
			next=(next<<62);
			word=word|next;
		}
	}

	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<(sizeof(uint64_t)*8-5-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			b.push_back(aTemplate);
		}
	}

	return b;
}

vector<Kmer> _getIngoingEdges(Kmer*a,uint8_t edges,int k){
	cout<<"Input"<<endl;
	a->print();
	vector<Kmer> b;
	Kmer aTemplate;
	aTemplate=*a;
	

	for(int i=0;i<aTemplate.getNumberOfU64();i++){
		uint8_t element=aTemplate.getU64(i);
		element=element<<2;

//	1		0
//
//	127..64		63...0
//
//	00abcdefgh  ijklmnopqr		// initial state
//	abcdefgh00  klmnopqr00		// shift left
//	abcdefghij  klmnopqr00		// copy the last to the first
//	00cdefghij  klmnopqr00		// reset the 2 last

		if(i!=0){
			// the 2 last of the previous will be the 2 first of this one
			uint64_t last=aTemplate.getU64(i-1);
			last=(last>>62);
			element=element|last;
		}

		if(i==aTemplate.getNumberOfU64()-1 && 2*k<64){
			uint64_t filter=1;
			filter=filter<<(2*k);
			element=element|filter;
			filter=1;
			filter=filter<<(2*k+1);
			element=element|filter;
		}
		aTemplate.setU64(i,element);
	}
	cout<<"Template, IN"<<endl;
	aTemplate.print();

	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<((sizeof(uint64_t)*8-1)-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			b.push_back(aTemplate);
		}
	}
	return b;
}

uint64_t hash_function_1(Kmer a,int w){
	Kmer b=complementVertex_normal(&a,w);
	if(b.isLower(&a)){
		a=b;
	}
	return uniform_hashing_function_1_64_64(a.getU64(0));
}

uint64_t hash_function_2(Kmer a,int w,Kmer*b){
	*b=complementVertex_normal(&a,w);
	if(b->isLower(&a)){
		a=*b;
	}
	return uniform_hashing_function_2_64_64(a.getU64(0));
}

	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0

uint8_t invertEdges(uint8_t edges){
	uint8_t out=0;

	// outgoing edges
	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<(sizeof(uint64_t)*8-5-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			switch(i){
				case _ENCODING_A:
					j=_ENCODING_T;
					break;
				case _ENCODING_T:
					j=_ENCODING_A;
					break;
				case _ENCODING_G:
					j=_ENCODING_C;
					break;
				case _ENCODING_C:
					j=_ENCODING_G;
					break;
				default:
					break;
			}
			
			uint8_t newBits=(1<<j);
			out=out|newBits;
		}
	}

	// Ingoing edges
	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<((sizeof(uint64_t)*8-1)-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			switch(i){
				case _ENCODING_A:
					j=_ENCODING_T;
					break;
				case _ENCODING_T:
					j=_ENCODING_A;
					break;
				case _ENCODING_G:
					j=_ENCODING_C;
					break;
				case _ENCODING_C:
					j=_ENCODING_G;
					break;
				default:
					break;
			}
			
			uint8_t newBits=(1<<(4+j));
			out=out|newBits;
		}
	}
	return out;
}


uint64_t getPathUniqueId(int rank,int id){
	uint64_t a=id;
	a=a*MAX_NUMBER_OF_MPI_PROCESSES+rank;
	return a;
}

int getIdFromPathUniqueId(uint64_t a){
	return a/MAX_NUMBER_OF_MPI_PROCESSES;
}

int getRankFromPathUniqueId(uint64_t a){
	int rank=a%MAX_NUMBER_OF_MPI_PROCESSES;
	return rank;
}

void now(){
	time_t m_endingTime=time(NULL);
	struct tm * timeinfo;
	timeinfo=localtime(&m_endingTime);
	cout<<"Date: "<<asctime(timeinfo);
}

