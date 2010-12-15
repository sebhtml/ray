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

#define _ENCODING_A 0
#define _ENCODING_T 1
#define _ENCODING_C 2
#define _ENCODING_G 3

string reverseComplement(string a){
	char*rev=(char*)__Malloc(a.length()+1);
	int i=0;
	for(int p=a.length()-1;p>=0;p--){
		char c=a[p];
		switch(c){
			case 'A':
				rev[i]='T';
				break;
			case 'T':
				rev[i]='A';
				break;
			case 'G':
				rev[i]='C';	
				break;
			case 'C':
				rev[i]='G';
				break;
			default:
				rev[i]=c;
				break;
		}
		i++;
	}
	rev[a.length()]='\0';
	string j(rev);
	__Free(rev);
	return j;
}

// convert k-mer to VERTEX_TYPE
VERTEX_TYPE wordId(const char*a){
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	return wordId_DistantSegments(a);
	#endif
	return wordId_Classic(a);

}

VERTEX_TYPE wordId_Classic(const char*a){
	VERTEX_TYPE i=0;
	for(int j=0;j<(int)strlen(a);j++){
		VERTEX_TYPE k=_ENCODING_A; // default is A
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

VERTEX_TYPE wordId_DistantSegments(const char*a){
	VERTEX_TYPE i=0;
	int len=strlen(a);
	for(int j=0;j<(int)strlen(a);j++){
		VERTEX_TYPE k=_ENCODING_A; // default is A
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

string idToWord(VERTEX_TYPE i,int wordSize){
	char a[1000];
	for(int p=0;p<wordSize;p++){
		VERTEX_TYPE j=(i<<(sizeof(VERTEX_TYPE)*8-2-2*p))>>(sizeof(VERTEX_TYPE)*8-2); // clear the bits.
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

//  63 (sizeof(VERTEX_TYPE)*8-2) ... 1 0
//              
char getFirstSymbol(VERTEX_TYPE i,int k){
	i=(i<<((sizeof(VERTEX_TYPE)*8-2)))>>(sizeof(VERTEX_TYPE)*8-2); // clear bits
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

char getLastSymbol(VERTEX_TYPE i,int m_wordSize){
	i=(i<<(sizeof(VERTEX_TYPE)*8-2*m_wordSize))>>(sizeof(VERTEX_TYPE)*8-2); // clecar bits
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
 *   63 (sizeof(VERTEX_TYPE)*8-2) ... 1 0
 *
 */

void coutBIN(VERTEX_TYPE a){
	for(int i=sizeof(VERTEX_TYPE)*8-1;i>=0;i--){
		cout<<(int)((a<<(sizeof(VERTEX_TYPE)*8-1-i))>>(sizeof(VERTEX_TYPE)*8-1));
	}
	cout<<endl;
}
void coutBIN8(uint8_t a){
	for(int i=7;i>=0;i--){
		cout<<(int)((a<<(7-i))>>7);
	}
	cout<<endl;
}


VERTEX_TYPE getKPrefix(VERTEX_TYPE a,int k){
	return (a<<(sizeof(VERTEX_TYPE)*8-2*(k+1)+2))>>(sizeof(VERTEX_TYPE)*8-2*(k+1)+2); // move things around...
}

VERTEX_TYPE getKSuffix(VERTEX_TYPE a,int k){
	return a>>2;
}

VERTEX_TYPE complementVertex(VERTEX_TYPE a,int b,bool colorSpace){
	if(colorSpace)
		return complementVertex_colorSpace(a,b);
	return complementVertex_normal(a,b);
}

VERTEX_TYPE complementVertex_colorSpace(VERTEX_TYPE a,int wordSize){
	VERTEX_TYPE output=0;
	int position2=0;
	for(int positionInMer=wordSize-1;positionInMer>=0;positionInMer--){
		VERTEX_TYPE complementVertex=(a<<((sizeof(VERTEX_TYPE)*8-2)-2*positionInMer))>>(sizeof(VERTEX_TYPE)*8-2);
		output=(output|(complementVertex<<(position2<<1)));
		position2++;
	}

	return output;
}

VERTEX_TYPE complementVertex_normal(VERTEX_TYPE a,int m_wordSize){
	VERTEX_TYPE output=0;
	int position2=0;
	for(int positionInMer=m_wordSize-1;positionInMer>=0;positionInMer--){
		VERTEX_TYPE j=(a<<((sizeof(VERTEX_TYPE)*8-2)-2*positionInMer))>>(sizeof(VERTEX_TYPE)*8-2);
		VERTEX_TYPE complementVertex=0;
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


/**
 * malloc with a memory verification.
 */
void*__Malloc(int c){
	void*a=NULL;
	a=malloc(c);
	if(a==NULL){
		cout<<"Critical exception: The system is out of memory, malloc returned NULL."<<endl;
	}
	assert(a!=NULL);
	return a;
}

void __Free(void*a){
	free(a);
}



uint8_t getFirstSegmentLastCode(VERTEX_TYPE v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	v=(v<<(sizeof(VERTEX_TYPE)*8-(segmentLength*2)));// clear garbage
	v=(v>>(sizeof(VERTEX_TYPE)*8-2));// restore state
	return v;
}

uint8_t getSecondSegmentLastCode(VERTEX_TYPE v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//               6 5 4 3 2 1 0
	v=v<<(sizeof(VERTEX_TYPE)*8-(totalLength*2));
	v=(v>>(sizeof(VERTEX_TYPE)*8-2));// restore state
	
	return v;
}

uint8_t getFirstSegmentFirstCode(VERTEX_TYPE v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	v=v<<(sizeof(VERTEX_TYPE)*8-2);
	v=v>>(sizeof(VERTEX_TYPE)*8-2);
	return v;
}

uint8_t getSecondSegmentFirstCode(VERTEX_TYPE v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//               6 5 4 3 2 1 0
	int extra=(2*segmentLength-2);
	v=v<<(sizeof(VERTEX_TYPE)*8-2*totalLength+extra);
	v=v>>(sizeof(VERTEX_TYPE)*8-2);
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
	if(a==_ENCODING_A)
		return 'A';
	if(a==_ENCODING_T)
		return 'T';
	if(a==_ENCODING_C)
		return 'C';
	if(a==_ENCODING_G)
		return 'G';
	return 'A';
}

string convertToString(vector<VERTEX_TYPE>*b,int m_wordSize){
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

int vertexRank(VERTEX_TYPE a,int _size){
	return uniform_hashing_function_1_64_64(a)%(_size);
}

/*
 *
 * show progress on-screen.
 */
void showProgress(time_t m_lastTime){
	printf("\r");
	int columns=10;
	int nn=m_lastTime%columns;
	
	for(int i=0;i<nn;i++){
		printf(".");
	}
	for(int i=0;i<columns-nn;i++){
		printf(" ");
	}
	fflush(stdout);

}

char*__basename(char*a){
	int i=0;
	int last=-1;
	int len=strlen(a);
	while(i<len){
		if(a[i]=='/')
			last=i;
		i++;
	}
	return a+last+1;
}



VERTEX_TYPE kmerAtPosition(const char*m_sequence,int pos,int w,char strand,bool color){
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
		char sequence[40];
		for(int i=0;i<w;i++){
			sequence[i]=m_sequence[pos+i];
		}
		sequence[w]='\0';
		VERTEX_TYPE v=wordId(sequence);
		return v;
	}else{
		char sequence[40];
		for(int i=0;i<w;i++){
			char a=m_sequence[strlen(m_sequence)-pos-w+i];
			sequence[i]=a;
		}
		sequence[w]='\0';
		VERTEX_TYPE v=wordId(sequence);
		return complementVertex(v,w,color);
	}
	return 0;
}

int roundNumber(int s,int alignment){
	return ((s/alignment)+1)*alignment;
}

u64 getMicroSeconds(){
	struct timeval tv;
	struct timezone tz;
	struct tm *tm;
	gettimeofday(&tv,&tz);
	tm=localtime(&tv.tv_sec);
	u64 milliSeconds=tm->tm_hour*60*60*1000*1000+tm->tm_min*60*1000*1000+tm->tm_sec*1000*1000+tv.tv_usec;
	return milliSeconds;
}



