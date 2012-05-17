/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <stdio.h>
#include <assert.h>
#include <string>
#include <fstream>
#include <cryptography/crypto.h>
using namespace std;

bool Kmer::operator<(const Kmer&b)const{
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		if(m_u64[i]<b.m_u64[i]){
			return true;
		}else if(m_u64[i]>b.m_u64[i]){
			return false;
		}
	}
	return false;
}

Kmer::Kmer(){
	for(int i=0;i<getNumberOfU64();i++){
		setU64(i,0);
	}
}

Kmer::~Kmer(){
}

int Kmer::getNumberOfU64(){
	return KMER_U64_ARRAY_SIZE;
}

bool Kmer::isLower(Kmer*a){
	for(int i=0;i<getNumberOfU64();i++){
		if(getU64(i)<a->getU64(i)){
			return true;
		}else if(getU64(i)>a->getU64(i)){
			return false;
		}
	}
	return false;
}

bool Kmer::isEqual(Kmer*a){
	for(int i=0;i<getNumberOfU64();i++){
		if(getU64(i)!=a->getU64(i)){
			return false;
		}
	}
	return true;
}

void Kmer::print(){
	for(int j=0;j<getNumberOfU64();j++){
		uint64_t a=getU64(j);
		for(int k=63;k>=0;k-=2){
			int bit=a<<(k-1)>>63;
			printf("%i",bit);
			bit=a<<(k)>>63;
			printf("%i ",bit);
		}
	}
	printf("\n");
}

void Kmer::pack(MessageUnit*messageBuffer,int*messagePosition){
	for(int i=0;i<getNumberOfU64();i++){
		messageBuffer[*messagePosition]=getU64(i);
		(*messagePosition)++;
	}
}

void Kmer::unpack(MessageUnit*messageBuffer,int*messagePosition){
	for(int i=0;i<getNumberOfU64();i++){
		setU64(i,messageBuffer[*messagePosition]);
		(*messagePosition)++;
	}
}

void Kmer::unpack(vector<MessageUnit>*messageBuffer,int*messagePosition){
	for(int i=0;i<getNumberOfU64();i++){
		setU64(i,(*messageBuffer)[*messagePosition]);
		(*messagePosition)++;
	}
}

void Kmer::operator=(const Kmer&b){
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		m_u64[i]=b.m_u64[i];
	}
}

bool Kmer::operator==(const Kmer&b) const{
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		if(m_u64[i]!=b.m_u64[i]){
			return false;
		}
	}
	return true;
}

bool Kmer::operator!=(const Kmer&b) const{
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		if(m_u64[i]!=b.m_u64[i]){
			return true;
		}
	}
	return false;
}

char Kmer::getLastSymbol(int m_wordSize,bool color){
	return codeToChar(getSecondSegmentLastCode(m_wordSize),color);
}

uint8_t Kmer::getSecondSegmentLastCode(int w){
	int bitPosition=2*w;
	int chunkId=bitPosition/64;
	int bitPositionInChunk=bitPosition%64;
	uint64_t chunk=getU64(chunkId);
	chunk=(chunk<<(sizeof(uint64_t)*8-bitPositionInChunk))>>(sizeof(uint64_t)*8-2); // clecar bits
	
	return (uint8_t)chunk;
}

uint8_t Kmer::getFirstSegmentFirstCode(int w){
	// ATCAGTTGCAGTACTGCAATCTACG
	// 0000000000000011100001100100000000000000000000000001011100100100
	//                                                   6 5 4 3 2 1 0
	uint64_t a=getU64(0);
	a=a<<(sizeof(uint64_t)*8-2);
	a=a>>(sizeof(uint64_t)*8-2);
	return a;
}

int Kmer::vertexRank(int _size,int w,bool color){
	Kmer b=complementVertex(w,color);
	if(isLower(&b))
		b=*this;
	return b.hash_function_1()%_size;
}

/**
 * Get the outgoing edges
 * one bit (1=yes, 0=no) per possible edge
 */
vector<Kmer> Kmer::_getOutgoingEdges(uint8_t edges,int k){
	vector<Kmer> b;
	Kmer aTemplate;
	aTemplate=*this;

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

/**
 * Get the ingoing edges
 * one bit (1=yes, 0=no) per possible edge
 */
vector<Kmer> Kmer::_getIngoingEdges(uint8_t edges,int k){
	vector<Kmer> b;
	Kmer aTemplate;
	aTemplate=*this;
	
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
			uint64_t last=getU64(i-1);
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

uint64_t Kmer::hash_function_1(){
	#if KMER_U64_ARRAY_SIZE == 1
	return uniform_hashing_function_1_64_64(getU64(0));
	#else
	uint64_t key=getU64(0);
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		uint64_t hash=uniform_hashing_function_1_64_64(getU64(i));
		key^=hash;
	}
	return key;

	#endif
}

uint64_t Kmer::hash_function_2(){
	#if KMER_U64_ARRAY_SIZE == 1
	return uniform_hashing_function_2_64_64(getU64(0));
	#else
	uint64_t key=getU64(0);
	for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
		uint64_t hash=uniform_hashing_function_2_64_64(getU64(i));
		key^=hash;
	}
	return key;

	#endif
}

void Kmer::convertToString(int kmerLength,bool color, char*buffer){
	for(int p=0;p<kmerLength;p++){
		int bitPosition=2*p;
		int chunkId=p/32;
		int bitPositionInChunk=(bitPosition%64);
		uint64_t chunk=getU64(chunkId);
		uint64_t j=(chunk<<(62-bitPositionInChunk))>>62; // clear the bits.
		buffer[p]=codeToChar(j,color);
	}
	buffer[kmerLength]='\0';
}

string Kmer::idToWord(int wordSize,bool color){
	char a[300];

	convertToString(wordSize,color,a);

	string b=a;

	return b;
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

void Kmer::write(ofstream*f){
	for(int i=0;i<getNumberOfU64();i++){
		uint64_t a=getU64(i);
		f->write((char*)&a,sizeof(uint64_t));
	}
}

void Kmer::read(ifstream*f){
	for(int i=0;i<getNumberOfU64();i++){
		uint64_t a=0;
		f->read((char*)&a,sizeof(uint64_t));
		setU64(i,a);
	}
}

void Kmer::setU64(int i,uint64_t b){
	#ifdef ASSERT
	assert(i<KMER_U64_ARRAY_SIZE);
	#endif
	m_u64[i]=b;
}

uint64_t Kmer::getU64(int i){
	#ifdef ASSERT
	assert(i<KMER_U64_ARRAY_SIZE);
	#endif
	return m_u64[i];
}

Kmer Kmer::complementVertex(int wordSize,bool colorSpace){
	Kmer output;
	int bitPositionInOutput=0;
	uint64_t mask=3;
	/* the order is inverted and nucleotides are complemented */
	/* this is costly  */
	for(int positionInMer=wordSize-1;positionInMer>=0;positionInMer--){
		int u64_id=positionInMer/32;
		int bitPositionInChunk=(2*positionInMer)%64;
		uint64_t chunk=getU64(u64_id);
		uint64_t j=(chunk<<(62-bitPositionInChunk))>>62;
		
		if(!colorSpace) /* in color space, reverse complement is just reverse */
			j=~j&mask;

		int outputChunk=bitPositionInOutput/64;
		uint64_t oldValue=output.getU64(outputChunk);
		oldValue=(oldValue|(j<<(bitPositionInOutput%64)));
		output.setU64(outputChunk,oldValue);
		bitPositionInOutput+=2;
	}
	return output;
}

double Kmer::getGuanineCytosineProportion(int kmerLength,bool coloredMode){
	char buffer[300];

	convertToString(kmerLength,coloredMode,buffer);

	int count=0;

	for(int i=0;i<kmerLength;i++){
		if(buffer[i]=='G' || buffer[i]=='C'){
			count++;
		}
	}

	#ifdef ASSERT
	assert(kmerLength!=0);
	#endif

	double proportion=(0.0+count)/kmerLength;

	return proportion;
}
