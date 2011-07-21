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
 * Encode a char
 */
uint8_t charToCode(char a);

/*
 * verify that x has only A,T,C, and G
 */
bool isValidDNA(char*x);

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

string convertToString(vector<Kmer>*b,int m_wordSize,bool color);

Kmer kmerAtPosition(const char*string,int pos,int w,char strand,bool color);

int roundNumber(int number,int alignment);

char complementNucleotide(char c);

uint64_t getPathUniqueId(int rank,int id);
int getIdFromPathUniqueId(uint64_t a);
int getRankFromPathUniqueId(uint64_t a);

string reverseComplement(string*a);

void print64(uint64_t a);
void print8(uint8_t a);

#endif


