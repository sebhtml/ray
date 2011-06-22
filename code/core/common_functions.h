/*
Ray
Copyright (C)  2010, 2011  Sébastien Boisvert

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

#ifndef _common_functions
#define _common_functions

#include <memory/allocator.h>
#include <structures/Kmer.h>
#include <string>
#include <core/constants.h>
#include <core/slave_modes.h>
#include <iostream>
#include <core/master_modes.h>
#include <vector>
#include <communication/mpi_tags.h>
#include <string.h>
#ifdef ASSERT
#include <assert.h>
#endif
using namespace std;

/*
 *  complement the sequence of a biological thing
 */
string reverseComplement(string a,char*rev);

/*
 * transform a Kmer in a string
 */
string idToWord(Kmer*i,int wordSize,bool color);

/*
 * transform a encoded nucleotide in a char
 */
char codeToChar(uint8_t a,bool color);

/*
 * Encode a char
 */
uint8_t charToCode(char a);

/*
 * verify that x has only A,T,C, and G
 */
bool isValidDNA(char*x);

/*
 * get the last letter of a uint64_t
 */
char getLastSymbol(Kmer*i,int w,bool color);

/*
 * complement a vertex, and return another one
 */
INLINE
Kmer complementVertex(Kmer*a,int wordSize,bool colorSpace){
	Kmer output;
	int bitPositionInOutput=0;
	uint64_t mask=3;
	for(int positionInMer=wordSize-1;positionInMer>=0;positionInMer--){
		int u64_id=positionInMer/32;
		int bitPositionInChunk=(2*positionInMer)%64;
		uint64_t chunk=a->getU64(u64_id);
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

/*
 * transform a string in a Kmer
 */
INLINE
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

/*
 * add line breaks to a string
 */
string addLineBreaks(string sequence,int a);

/*
 * use mini distant segments here.
 */
uint8_t getFirstSegmentFirstCode(Kmer*v,int w);
uint8_t getSecondSegmentLastCode(Kmer*v,int w);

string convertToString(vector<Kmer>*b,int m_wordSize,bool color);

int vertexRank(Kmer*a,int _size,int w,bool color);

Kmer kmerAtPosition(char*string,int pos,int w,char strand,bool color);

int roundNumber(int number,int alignment);

uint64_t getMilliSeconds();

void showMemoryUsage(int rank);

vector<Kmer> _getOutgoingEdges(Kmer*a,uint8_t edges,int k);
vector<Kmer> _getIngoingEdges(Kmer*a,uint8_t edges,int k);

char complementNucleotide(char c);

uint64_t hash_function_1(Kmer*a,int w,bool color);
uint64_t hash_function_2(Kmer*a,int w,Kmer*b,bool color);

uint8_t invertEdges(uint8_t a);

uint64_t getPathUniqueId(int rank,int id);
int getIdFromPathUniqueId(uint64_t a);
int getRankFromPathUniqueId(uint64_t a);

void now();

string reverseComplement(string*a);

void print64(uint64_t a);
void print8(uint8_t a);

int portableProcessId();

#endif


