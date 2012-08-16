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

#ifndef _Kmer
#define _Kmer

#include <application_core/constants.h>
#include <stdint.h>
#include <core/types.h>

/* this header was missing, but the code compiled with clang++, gcc, intel, pgi, but not pathscale. pathscale was right */
#include <fstream> 

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
 * \author Sébastien Boisvert
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
	void pack(MessageUnit *messageBuffer,int*messagePosition);
	void unpack(MessageUnit*messageBuffer,int*messagePosition);
	void unpack(vector<MessageUnit>*messageBuffer,int*messagePosition);
	void operator=(const Kmer&b);
	bool operator<(const Kmer&b)const;
	bool operator!=(const Kmer&b)const;
	bool operator==(const Kmer&b)const;

	void setU64(int i,uint64_t b);

	uint64_t getU64(int i);

	int getNumberOfU64();
/*
 * get the last letter of a uint64_t
 */
	char getLastSymbol(int w,bool color);

/*
 * complement a vertex, and return another one
 */
	Kmer complementVertex(int wordSize,bool colorSpace);
	
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

	void convertToString(int kmerLength,bool coloredMode,char*buffer);
	double getGuanineCytosineProportion(int kmerLength,bool coloredMode);

}ATTRIBUTE_PACKED;

/*
 * transform a encoded nucleotide in a char
 */
char codeToChar(uint8_t a,bool color);

inline int getNumberOfNucleotides(int numberOfKmers,int kmerLength){
	return ( numberOfKmers==0 ) ?  0 :  (numberOfKmers + kmerLength -1 );
}

#endif
