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

#include<assert.h>
#include<vector>
#include<fstream>
#include<common_functions.h>
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

// convert k-mer to uint64_t
uint64_t wordId(const char*a,bool colorSpace){
	if(colorSpace)
		return wordId_DistantSegments(a);
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
	int numberOfSymbols=strlen(a);
	int segmentLength=_SEGMENT_LENGTH;
	
	/*
 *                    <------------ 5 ---------->
 *                    0   1       2       3     4
 *                    segmentLength=2
 *
 */
	for(int j=0;j<(int)numberOfSymbols;j++){
		uint64_t k=_ENCODING_A; // default is A
		char h='A';
		if(j<segmentLength // left part 
		or j>=numberOfSymbols-segmentLength){ // right part.
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
	char*a=new char[wordSize+1];
	for(int p=0;p<wordSize;p++){
		uint64_t j=(i<<(62-2*p))>>62; // clear the bits.
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
	delete[]a;
	return b;
}

//  63 62 ... 1 0
//              
char getFirstSymbol(uint64_t i,int k){
	i=(i<<(62))>>62; // clear bits
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
	i=(i<<(64-2*m_wordSize))>>62; // clecar bits
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
 *   63 62 ... 1 0
 *
 */

void coutBIN(uint64_t a){
	for(int i=63;i>=0;i--){
		cout<<(int)((a<<(63-i))>>63);
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
	return (a<<(64-2*(k+1)+2))>>(64-2*(k+1)+2); // move things around...
}

uint64_t getKSuffix(uint64_t a,int k){
	return a>>2;
}

uint64_t complementVertex(uint64_t a,int b,bool colorSpace){
	if(colorSpace)
		return complementVertex_colorSpace(a,b);
	return complementVertex_normal(a,b);
}

uint64_t complementVertex_colorSpace(uint64_t a,int wordSize){
	uint64_t output=0;
	int position2=0;
	for(int positionInMer=wordSize-1;positionInMer>=0;positionInMer--){
		uint64_t complementVertex=(a<<(62-2*positionInMer))>>62;
		output=(output|(complementVertex<<(position2<<1)));
		position2++;
	}

	return output;
}

uint64_t complementVertex_normal(uint64_t a,int m_wordSize){
	uint64_t output=0;
	int position2=0;
	for(int positionInMer=m_wordSize-1;positionInMer>=0;positionInMer--){
		uint64_t j=(a<<(62-2*positionInMer))>>62;
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


/*
 * Basically, we take a 64-bit unsigned integer (the sign does not matter..)
 * and we compute the image corresponding to a uniform distribution.
 *
 * This uses some magic, of course!
 */
// see http://www.concentric.net/~Ttwang/tech/inthash.htm 64 bit Mix Functions
uint64_t hash_uint64_t(uint64_t key){
	// some magic here and there.
	key = (~key) + (key << 21); 
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); 
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); 
	key = key ^ (key >> 28);
	key = key + (key << 31);
	return key;
}


void*__Malloc(int c){
	void*a=NULL;
	a=malloc(c);
	assert(a!=NULL);
	return a;
}

void __Free(void*a){
}



uint8_t getFirstSegmentLastCode(uint64_t v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	v=(v<<(64-(segmentLength*2)));// clear garbage
	v=(v>>62);// restore state
	return v;
}

uint8_t getSecondSegmentLastCode(uint64_t v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//               6 5 4 3 2 1 0
	v=v<<(64-(totalLength*2));
	v=(v>>62);// restore state
	
	return v;
}

uint8_t getFirstSegmentFirstCode(uint64_t v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	v=v<<62;
	v=v>>62;
	return v;
}

uint8_t getSecondSegmentFirstCode(uint64_t v,int totalLength,int segmentLength){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//               6 5 4 3 2 1 0
	int extra=(2*segmentLength-2);
	v=v<<(64-2*totalLength+extra);
	v=v>>62;
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
