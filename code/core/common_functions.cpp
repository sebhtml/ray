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

#include <cryptography/crypto.h>
#include <assert.h>
#include <stdio.h>
#include <core/constants.h>
#include <time.h>
#include <vector>
#include <fstream>
#include <core/common_functions.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>

#ifdef OS_POSIX
#include <unistd.h>
#endif
#ifdef OS_WIN
#include <windows.h>
#endif

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

char getLastSymbol(Kmer*i,int m_wordSize,bool color){
	return codeToChar(getSecondSegmentLastCode(i,m_wordSize),color);
}

bool isValidDNA(char*x){
	int len=strlen(x);
	for(int i=0;i<len;i++){
		char a=x[i];
		if(!(a=='A'||a=='T'||a=='C'||a=='G'))
			return false;
	}
	return true;
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

uint8_t getSecondSegmentLastCode(Kmer* v,int w){
	int bitPosition=2*w;
	int chunkId=bitPosition/64;
	int bitPositionInChunk=bitPosition%64;
	uint64_t chunk=v->getU64(chunkId);
	chunk=(chunk<<(sizeof(uint64_t)*8-bitPositionInChunk))>>(sizeof(uint64_t)*8-2); // clecar bits
	
	return (uint8_t)chunk;
}

uint8_t getFirstSegmentFirstCode(Kmer*v,int w){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	uint64_t a=v->getU64(0);
	a=a<<(sizeof(uint64_t)*8-2);
	a=a>>(sizeof(uint64_t)*8-2);
	return a;
}

string convertToString(vector<Kmer>*b,int m_wordSize,bool color){
	ostringstream a;
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	//
	//TTAATT
	// TTAATT
	//  TTAATT
	//  the first vertex can not fill in the first delta letter alone, it needs help.
	for(int p=0;p<m_wordSize;p++){
		a<<codeToChar(getFirstSegmentFirstCode((*b)[p],m_wordSize));
	}
	#else
	a<<idToWord(&(*b)[0],m_wordSize,color);
	#endif
	for(int j=1;j<(int)(*b).size();j++){
		a<<getLastSymbol(&(*b)[j],m_wordSize,color);
	}
	string contig=a.str();
	return contig;
}

int vertexRank(Kmer*a,int _size,int w,bool color){
	return hash_function_1(a,w,color)%(_size);
}

Kmer kmerAtPosition(char*m_sequence,int pos,int w,char strand,bool color){
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
	#ifdef __linux__
	ifstream f("/proc/self/status");
	while(!f.eof()){
		string key;
		f>>key;
		if(key=="VmData:"){
			uint64_t count;
			f>>count;
			cout<<"Rank "<<rank<<": VmData= "<<count<<" KiB"<<endl;
			cout.flush();
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
		aTemplate.setU64(i,word);
	}

	int positionToUpdate=2*k;
	int chunkIdToUpdate=positionToUpdate/64;
	positionToUpdate=positionToUpdate%64;

	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<(sizeof(uint64_t)*8-5-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			Kmer newKmer=aTemplate;
			uint64_t last=newKmer.getU64(chunkIdToUpdate);
			uint64_t filter=i;
			filter=filter<<(positionToUpdate-2);
			last=last|filter;
			newKmer.setU64(chunkIdToUpdate,last);
			b.push_back(newKmer);
		}
	}

	return b;
}

vector<Kmer> _getIngoingEdges(Kmer*a,uint8_t edges,int k){
	vector<Kmer> b;
	Kmer aTemplate;
	aTemplate=*a;
	
	int posToClear=2*k;

	for(int i=0;i<aTemplate.getNumberOfU64();i++){
		uint64_t element=aTemplate.getU64(i);
		element=element<<2;

//	1		0
//
//	127..64		63...0
//
//	00abcdefgh  ijklmnopqr		// initial state
//	abcdefgh00  klmnopqr00		// shift left
//	abcdefghij  klmnopqr00		// copy the last to the first
//	00cdefghij  klmnopqr00		// reset the 2 last

/**
 * Now, we need to copy 2 bits from 
 */
		if(i!=0){
			// the 2 last of the previous will be the 2 first of this one
			uint64_t last=a->getU64(i-1);
			last=(last>>62);
			element=element|last;
		}

		/**
 *	The two last bits that shifted must be cleared
 *	Otherwise, it will change the hash value of the Kmer...
 *	The chunk number i contains bits from i to i*64-1
 *	Therefore, if posToClear is inside these boundaries,
 *	then it is obvious that these awful bits must be changed 
 *	to 0
 */
		if(i*64<=posToClear&&posToClear<i*64+64){
			int position=posToClear%64;

			uint64_t filter=3;// 11 or 1*2^1+1*2^0
			filter=filter<<(position);
			filter=~filter;
			element=element&filter;
		}
		aTemplate.setU64(i,element);
	}

	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<((sizeof(uint64_t)*8-1)-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			Kmer newKmer=aTemplate;
			int id=0;
			uint64_t last=newKmer.getU64(id);
			uint64_t filter=i;
			last=last|filter;
			newKmer.setU64(id,last);
			b.push_back(newKmer);
		}
	}
	return b;
}

uint64_t hash_function_1(Kmer*a,int w,bool color){
	Kmer b=complementVertex(a,w,color);
	if(a->isLower(&b)){
		b=*a;
	}
	#if KMER_U64_ARRAY_SIZE == 1
	return uniform_hashing_function_1_64_64(b.getU64(0));
	#else
	uint64_t key=b.getU64(0);
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		uint64_t hash=uniform_hashing_function_1_64_64(b.getU64(i));
		key^=hash;
	}
	return key;

	#endif
}

uint64_t hash_function_2(Kmer*a,int w,Kmer*c,bool color){
	Kmer b=complementVertex(a,w,color);
	*c=b;
	if(a->isLower(&b)){
		b=*a;
	}
	#if KMER_U64_ARRAY_SIZE == 1
	return uniform_hashing_function_2_64_64(b.getU64(0));
	#else
	uint64_t key=b.getU64(0);
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		uint64_t hash=uniform_hashing_function_2_64_64(b.getU64(i));
		key^=hash;
	}
	return key;

	#endif
}

	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0

uint8_t invertEdges(uint8_t edges){
	uint8_t out=0;

	uint64_t mask=3;

	// outgoing edges
	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<(sizeof(uint64_t)*8-5-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			j=~i&mask;
			
			uint8_t newBits=(1<<j);
			out=out|newBits;
		}
	}

	// Ingoing edges
	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<((sizeof(uint64_t)*8-1)-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			j=~i&mask;
			
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
	#ifdef OS_POSIX
	time_t m_endingTime=time(NULL);
	struct tm * timeinfo;
	timeinfo=localtime(&m_endingTime);
	cout<<"Date: "<<asctime(timeinfo);
	#endif
}

void print64(uint64_t a){
	for(int k=63;k>=0;k-=2){
		int bit=a<<(k-1)>>63;
		printf("%i",bit);
		bit=a<<(k)>>63;
		printf("%i ",bit);
	}
	printf("\n");
}

void print8(uint8_t a){
	for(int i=7;i>=0;i--){
		int bit=((((uint64_t)a)<<((sizeof(uint64_t)*8-1)-i))>>(sizeof(uint64_t)*8-1));
		printf("%i ",bit);
	}
	printf("\n");
}

uint8_t charToCode(char a){
	switch (a){
		case 'A':
			return RAY_NUCLEOTIDE_A;
		case 'T':
			return RAY_NUCLEOTIDE_T;
		case 'C':
			return RAY_NUCLEOTIDE_C;
		case 'G':
			return RAY_NUCLEOTIDE_G;
		default:
			return RAY_NUCLEOTIDE_A;
	}
}

char codeToChar(uint8_t a,bool color){
	if(color){
		switch(a){
			case RAY_NUCLEOTIDE_A:
				return DOUBLE_ENCODING_A_COLOR;
			case RAY_NUCLEOTIDE_T:
				return DOUBLE_ENCODING_T_COLOR;
			case RAY_NUCLEOTIDE_C:
				return DOUBLE_ENCODING_C_COLOR;
			case RAY_NUCLEOTIDE_G:
				return DOUBLE_ENCODING_G_COLOR;
		}
		return DOUBLE_ENCODING_A_COLOR;
	}

	switch(a){
		case RAY_NUCLEOTIDE_A:
			return 'A';
		case RAY_NUCLEOTIDE_T:
			return 'T';
		case RAY_NUCLEOTIDE_C:
			return 'C';
		case RAY_NUCLEOTIDE_G:
			return 'G';
	}
	return 'A';
}

string idToWord(Kmer*i,int wordSize,bool color){
	char a[1000];
	for(int p=0;p<wordSize;p++){
		int bitPosition=2*p;
		int chunkId=p/32;
		int bitPositionInChunk=(bitPosition%64);
		uint64_t chunk=i->getU64(chunkId);
		uint64_t j=(chunk<<(62-bitPositionInChunk))>>62; // clear the bits.
		a[p]=codeToChar(j,color);
	}
	a[wordSize]='\0';
	string b=a;
	return b;
}

int portableProcessId(){
	#ifdef OS_POSIX
	return getpid();
	#elif defined(OS_WIN)
	return GetCurrentProcessId();
	#else
	return -1;
	#endif
}
