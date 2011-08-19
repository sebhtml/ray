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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _Kmer
#define _Kmer

#include <core/constants.h>
#include <stdint.h>
#include <vector>
#ifdef ASSERT
#include <assert.h>
#endif
#include <string>
using namespace std;

/*
 * Determine the number of uint64_t necessary to host 
 * k-mers of maximum length MAXKMERLENGTH
 */
#define KMER_REQUIRED_BITS (2*MAXKMERLENGTH)
#define KMER_REQUIRED_BYTES (KMER_REQUIRED_BITS/8)
#define KMER_REQUIRED_BYTES_MODULO (KMER_REQUIRED_BITS%8)
#if KMER_REQUIRED_BYTES_MODULO
	#define KMER_BYTES (KMER_REQUIRED_BYTES+1)
#else
	#define KMER_BYTES KMER_REQUIRED_BYTES
#endif

#define KMER_UINT64_T (KMER_BYTES/8)
#define KMER_UINT64_T_MODULO (KMER_BYTES%8)
#if KMER_UINT64_T_MODULO
	#define KMER_U64_ARRAY_SIZE (KMER_UINT64_T+1)
#else
	#define KMER_U64_ARRAY_SIZE (KMER_UINT64_T)
#endif

/**
 * Class for storing k-mers.
 * For now only an array of uint64_t is present, but later,
 * when the code is stable, this could be a mix of u64, u32 and u16 and u8
 * while maintening the same interface, that are the two functions.
 *
 */
class Kmer{
	/** the actual array of uint64_t */
	uint64_t m_u64[KMER_U64_ARRAY_SIZE];
public:
	Kmer();
	~Kmer();
	bool isEqual(Kmer*a);
	bool isLower(Kmer*a);
	void print();
	void pack(uint64_t*messageBuffer,int*messagePosition);
	void unpack(uint64_t*messageBuffer,int*messagePosition);
	void unpack(vector<uint64_t>*messageBuffer,int*messagePosition);
	void operator=(const Kmer&b);
	bool operator<(const Kmer&b)const;
	bool operator!=(const Kmer&b)const;
	bool operator==(const Kmer&b)const;

	INLINE
	void setU64(int i,uint64_t b){
		#ifdef ASSERT
		assert(i<KMER_U64_ARRAY_SIZE);
		#endif
		m_u64[i]=b;
	}

	INLINE
	uint64_t getU64(int i){
		#ifdef ASSERT
		assert(i<KMER_U64_ARRAY_SIZE);
		#endif
		return m_u64[i];
	}

	int getNumberOfU64();
/*
 * get the last letter of a uint64_t
 */
	char getLastSymbol(int w,bool color);

/*
 * complement a vertex, and return another one
 */
	INLINE
	Kmer complementVertex(int wordSize,bool colorSpace){
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
	
/*
 * use mini distant segments here.
 */
	uint8_t getFirstSegmentFirstCode(int w);
	uint8_t getSecondSegmentLastCode(int w);
	int vertexRank(int _size,int w,bool color);
/**
 * get the outgoing Kmer objects for a Kmer a having edges and
 * a k-mer length k
 */
	vector<Kmer> _getOutgoingEdges(uint8_t edges,int k);

/**
 * get the ingoing Kmer objects for a Kmer a having edges and
 * a k-mer length k
 */
	vector<Kmer> _getIngoingEdges(uint8_t edges,int k);

	/** hash 1 is used to distribute k-mers on MPI ranks */
	uint64_t hash_function_1();

	/** hash 2 is used for double hashing in the hash tables */
	uint64_t hash_function_2();
/*
 * transform a Kmer in a string
 */
	string idToWord(int wordSize,bool color);

	void write(ofstream*f);
	void read(ifstream*f);

}ATTRIBUTE_PACKED;

/*
 * transform a encoded nucleotide in a char
 */
char codeToChar(uint8_t a,bool color);

#endif
