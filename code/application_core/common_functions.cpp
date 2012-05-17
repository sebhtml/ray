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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <cryptography/crypto.h>
#include <assert.h>
#include <stdio.h>
#include <application_core/constants.h>
#include <time.h>
#include <vector>
#include <fstream>
#include <application_core/common_functions.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <cstring>
#include <core/OperatingSystem.h>
#include <sstream>

using namespace std;

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

string convertToString(vector<Kmer>*b,int m_wordSize,bool color){
	ostringstream a;
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	//
	//TTAATT
	// TTAATT
	//  TTAATT
	//  the first vertex can not fill in the first delta letter alone, it needs help.
	for(int p=0;p<m_wordSize;p++){
		a<<codeToChar(b->at(p).getFirstSegmentFirstCode(m_wordSize));
	}
	#else
	a<<b->at(0).idToWord(m_wordSize,color);
	#endif
	for(int j=1;j<(int)(*b).size();j++){
		a<<b->at(j).getLastSymbol(m_wordSize,color);
	}
	string contig=a.str();
	return contig;
}

Kmer kmerAtPosition(const char*m_sequence,int pos,int w,char strand,bool color){
	#ifdef ASSERT
	assert(w<=MAXKMERLENGTH);
	#endif
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
		char sequence[MAXKMERLENGTH];
		memcpy(sequence,m_sequence+pos,w);
		sequence[w]='\0';
		Kmer v=wordId(sequence);
		return v;
	}else if(strand=='R'){
		char sequence[MAXKMERLENGTH];
		memcpy(sequence,m_sequence+length-pos-w,w);
		sequence[w]='\0';
		Kmer v=wordId(sequence);
		return v.complementVertex(w,color);
	}
	Kmer error;
	return error;
}

uint64_t getPathUniqueId(Rank rank,int id){
	uint64_t a=id;
	a=a*MAX_NUMBER_OF_MPI_PROCESSES+rank;
	return a;
}

int getIdFromPathUniqueId(uint64_t a){
	return a/MAX_NUMBER_OF_MPI_PROCESSES;
}

Rank getRankFromPathUniqueId(uint64_t a){
	int rank=a%MAX_NUMBER_OF_MPI_PROCESSES;
	return rank;
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

Kmer wordId(const char*a){
	Kmer i;
	int theLen=strlen(a);
	for(int j=0;j<(int)theLen;j++){
		uint64_t k=charToCode(a[j]);
		int bitPosition=2*j;
		int chunk=bitPosition/64;
		int bitPositionInChunk=bitPosition%64;
		#ifdef ASSERT
		if(!(chunk<i.getNumberOfU64())){
			cout<<"Chunk="<<chunk<<" positionInKmer="<<j<<" KmerLength="<<strlen(a)<<" bitPosition=" <<bitPosition<<" Chunks="<<i.getNumberOfU64()<<endl;
		}
		assert(chunk<i.getNumberOfU64());
		#endif
		uint64_t filter=(k<<bitPositionInChunk);
		i.setU64(chunk,i.getU64(chunk)|filter);
	}
	return i;
}

/** pack the pointer in a uint64_t */
uint64_t pack_pointer(void**pointer){
	if( NUMBER_OF_BITS == 64){

		uint64_t*placeHolder=(uint64_t*)pointer;
		uint64_t integerValue=*placeHolder;

		return integerValue;
	
	}else if(NUMBER_OF_BITS == 32){

		uint32_t*placeHolder=(uint32_t*)pointer;
		uint32_t integerValue=*placeHolder;

		return integerValue;
	}

	return 0; // not supported
}

void unpack_pointer(void**pointer,uint64_t integerValue){

	if( NUMBER_OF_BITS == 64){

		uint64_t*placeHolder=(uint64_t*)pointer;

		*placeHolder=integerValue;

	}else if(NUMBER_OF_BITS == 32){

		uint32_t*placeHolder=(uint32_t*)pointer;

		*placeHolder=integerValue;
	}

}

bool flushFileOperationBuffer(bool force,ostringstream*buffer,ostream*file,int bufferSize){

	int available=buffer->tellp();

	if(available==0)
		return false;

	if(force || available>=bufferSize){

		(*file)<<buffer->str();
		buffer->str("");

		return true;
	}

	return false;
}

bool flushFileOperationBuffer_FILE(bool force,ostringstream*buffer,FILE*file,int bufferSize){

	int available=buffer->tellp();

	if(available==0)
		return false;

	if(force || available>=bufferSize){

		string copy=buffer->str();

		fprintf(file,"%s",copy.c_str());
		buffer->str("");

		return true;
	}

	return false;
}

