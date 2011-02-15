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

#include<crypto.h>
#include<assert.h>
#include<stdio.h>
#include<sys/time.h>
#include<vector>
#include<fstream>
#include<common_functions.h>
#include<time.h>
#include<stdlib.h>
#include<iostream>
#include<string>
#include<cstring>
#include<sstream>
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

// convert k-mer to uint64_t
uint64_t wordId(const char*a){
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	return wordId_DistantSegments(a);
	#endif
	return wordId_Classic(a);

}

uint64_t wordId_Classic(const char*a){
	uint64_t i=0;
	for(int j=0;j<(int)strlen(a);j++){
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
		i=(i|(k<<(j<<1)));
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

string idToWord(uint64_t i,int wordSize){
	char a[1000];
	for(int p=0;p<wordSize;p++){
		uint64_t j=(i<<(sizeof(uint64_t)*8-2-2*p))>>(sizeof(uint64_t)*8-2); // clear the bits.
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

char getLastSymbol(uint64_t i,int m_wordSize){
	i=(i<<(sizeof(uint64_t)*8-2*m_wordSize))>>(sizeof(uint64_t)*8-2); // clecar bits
        if((int)i==_ENCODING_A)
                return 'A';
        if((int)i==_ENCODING_T)
                return 'T';
        if((int)i==_ENCODING_C)
                return 'C';
        if((int)i==_ENCODING_G)
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

uint64_t complementVertex(uint64_t a,int b,bool colorSpace){
	if(colorSpace){
		return complementVertex_colorSpace(a,b);
	}
	return complementVertex_normal(a,b);
}

uint64_t complementVertex_colorSpace(uint64_t a,int wordSize){
	uint64_t output=0;
	int position2=0;
	for(int positionInMer=wordSize-1;positionInMer>=0;positionInMer--){
		uint64_t complementVertex=(a<<((sizeof(uint64_t)*8-2)-2*positionInMer))>>(sizeof(uint64_t)*8-2);
		output=(output|(complementVertex<<(position2<<1)));
		position2++;
	}

	return output;
}

uint64_t complementVertex_normal(uint64_t a,int m_wordSize){
	uint64_t output=0;
	int position2=0;
	for(int positionInMer=m_wordSize-1;positionInMer>=0;positionInMer--){
		uint64_t j=(a<<((sizeof(uint64_t)*8-2)-2*positionInMer))>>(sizeof(uint64_t)*8-2);
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
		output=(output|(complementVertex<<(position2<<1)));
		position2++;
	}
	return output;
}

string addLineBreaks(string dna){
	ostringstream output;
	int j=0;
	int columns=60;
	while(j<(int)dna.length()){
		output<<dna.substr(j,columns)<<endl;
		j+=columns;
	}
	return output.str();
}

#define MALLOC_DEBUG
void*__Malloc(int c){
	void*a=NULL;
	a=malloc(c);
	if(a==NULL){
		cout<<"Critical exception: The system is out of memory, returned NULL."<<endl;
	}
	assert(a!=NULL);
	assert(c!=0);

	#ifdef MALLOC_DEBUG
	printf("%s %i %s %i bytes, ret %p\n",__FILE__,__LINE__,__func__,c,a);
	fflush(stdout);
	#endif
	return a;
}

void __Free(void*a){
	#ifdef MALLOC_DEBUG
	printf("%s %i %s %p\n",__FILE__,__LINE__,__func__,a);
	fflush(stdout);
	#endif

	free(a);
}

uint8_t getFirstSegmentLastCode(uint64_t v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	v=(v<<(sizeof(uint64_t)*8-(segmentLength*2)));// clear garbage
	v=(v>>(sizeof(uint64_t)*8-2));// restore state
	return v;
}

uint8_t getSecondSegmentLastCode(uint64_t v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//               6 5 4 3 2 1 0
	v=v<<(sizeof(uint64_t)*8-(totalLength*2));
	v=(v>>(sizeof(uint64_t)*8-2));// restore state
	
	return v;
}

uint8_t getFirstSegmentFirstCode(uint64_t v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	v=v<<(sizeof(uint64_t)*8-2);
	v=v>>(sizeof(uint64_t)*8-2);
	return v;
}

uint8_t getSecondSegmentFirstCode(uint64_t v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//               6 5 4 3 2 1 0
	int extra=(2*segmentLength-2);
	v=v<<(sizeof(uint64_t)*8-2*totalLength+extra);
	v=v>>(sizeof(uint64_t)*8-2);
	return v;
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

string convertToString(vector<uint64_t>*b,int m_wordSize){
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
	a<<idToWord((*b)[0],m_wordSize);
	#endif
	for(int j=1;j<(int)(*b).size();j++){
		a<<getLastSymbol((*b)[j],m_wordSize);
	}
	string contig=a.str();
	return contig;
}

int vertexRank(uint64_t a,int _size,int w){
	return hash_function_1(a,w)%(_size);
}

uint64_t kmerAtPosition(char*m_sequence,int pos,int w,char strand,bool color){
	int length=strlen(m_sequence);
	if(pos>length-w){
		cout<<"Fatal: offset is too large."<<endl;
		exit(0);
	}
	if(pos<0){
		cout<<"Fatal: negative offset. "<<pos<<endl;
		exit(0);
	}
	if(strand=='F'){
		char*sequence=m_sequence+pos;
		char tmp=sequence[w];
		sequence[w]='\0';
		uint64_t v=wordId(sequence);
		sequence[w]=tmp;
		return v;
	}else if(strand=='R'){
		char*sequence=m_sequence+length-pos-w;
		char tmp=sequence[w];
		sequence[w]='\0';
		uint64_t v=wordId(sequence);
		sequence[w]=tmp;
		return complementVertex(v,w,color);
	}
	return 0;
}

int roundNumber(int s,int alignment){
	return ((s/alignment)+1)*alignment;
}

uint64_t getMicroSeconds(){
	struct timeval tv;
	struct timezone tz;
	struct tm *tm;
	gettimeofday(&tv,&tz);
	tm=localtime(&tv.tv_sec);
	uint64_t milliSeconds=tm->tm_hour*60*60*1000*1000+tm->tm_min*60*1000*1000+tm->tm_sec*1000*1000+tv.tv_usec;
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

vector<uint64_t> _getOutgoingEdges(uint64_t a,uint8_t edges,int k){
	vector<uint64_t> b;
	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<(sizeof(uint64_t)*8-5-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			uint64_t l=(a>>2)|(((uint64_t)i)<<(2*(k-1)));
			b.push_back(l);
		}
	}

	return b;
}

vector<uint64_t> _getIngoingEdges(uint64_t a,uint8_t edges,int k){
	vector<uint64_t> b;
	for(int i=0;i<4;i++){
		int j=((((uint64_t)edges)<<((sizeof(uint64_t)*8-1)-i))>>(sizeof(uint64_t)*8-1));
		if(j==1){
			uint64_t l=((a<<(sizeof(uint64_t)*8-2*k+2))>>(sizeof(uint64_t)*8-2*k))|((uint64_t)i);
			b.push_back(l);
		}
	}
	return b;
}

uint64_t hash_function_1(uint64_t a,int w){
	uint64_t b=complementVertex_normal(a,w);
	if(b<a){
		a=b;
	}
	return uniform_hashing_function_1_64_64(a);
}

uint64_t hash_function_2(uint64_t a,int w,uint64_t*b){
	*b=complementVertex_normal(a,w);
	if(*b<a){
		a=*b;
	}
	return uniform_hashing_function_2_64_64(a);
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
