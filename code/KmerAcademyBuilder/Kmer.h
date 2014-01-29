/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2011, 2012, 2013 Sébastien Boisvert

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

#include <code/Mock/constants.h>

#include <RayPlatform/store/CarriageableItem.h>

#include <RayPlatform/core/types.h>

#include <stdint.h>

/* this header was missing, but the code compiled with clang++, gcc, intel, pgi, but not pathscale. pathscale was right */
#include <fstream> 
#include <vector>
#ifdef CONFIG_ASSERT
#include <assert.h>
#endif
#include <string>
using namespace std;

/*
 * Determine the number of uint64_t necessary to host 
 * k-mers of maximum length CONFIG_MAXKMERLENGTH
 */
#define KMER_REQUIRED_BITS (2*CONFIG_MAXKMERLENGTH)
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
class Kmer : public CarriageableItem {

private:
	/** the actual array of uint64_t */
	uint64_t m_u64[KMER_U64_ARRAY_SIZE];

public:
	Kmer();
	~Kmer();
	bool isEqual(Kmer*a)const;
	bool isLower(Kmer*a)const;
	void print()const;
	void pack(MessageUnit *messageBuffer,int*messagePosition)const;
	void unpack(const MessageUnit*messageBuffer,int*messagePosition);
	void unpack(const vector<MessageUnit>*messageBuffer,int*messagePosition);

	int load(const char * buffer);
	int dump(char * buffer) const;
	int getRequiredNumberOfBytes() const;

	void operator=(const Kmer&b);
	bool operator<(const Kmer&b)const;
	bool operator!=(const Kmer&b)const;
	bool operator==(const Kmer&b)const;

	void setU64(int i,uint64_t b);

	uint64_t getU64(int i)const;

	int getNumberOfU64()const;

/*
 * get the last letter of a uint64_t
 */
	char getLastSymbol(int w,bool color)const;

	char getSymbolAtPosition(int kmerLength,bool colored, int position)const;

/*
 * complement a vertex, and return another one
 */
	Kmer complementVertex(int wordSize,bool colorSpace)const;

/*
 * use mini distant segments here.
 */
	uint8_t getFirstSegmentFirstCode(int w)const;
	uint8_t getSecondSegmentLastCode(int w)const;
	Rank vertexRank(int _size,int w,bool color)const;
/**
 * get the outgoing Kmer objects for a Kmer a having edges and
 * a k-mer length k
 *
 * TODO: vector<Kmer>* should be a output parameter
 */
	vector<Kmer> getOutgoingEdges(uint8_t edges,int k)const;

/**
 * get the ingoing Kmer objects for a Kmer a having edges and
 * a k-mer length k
 *
 * TODO: vector<Kmer>* should be a output parameter
 */
	vector<Kmer> getIngoingEdges(uint8_t edges,int k)const;

	/** hash 1 is used to distribute k-mers on MPI ranks */
	uint64_t hash_function_1()const;
	uint64_t getHashValue1() const;

	/** hash 2 is used for double hashing in the hash tables */
	uint64_t hash_function_2()const;
	uint64_t getHashValue2() const;
/*
 * transform a Kmer in a string
 */
	string idToWord(int wordSize,bool color)const;

	void write(ostream*f)const;
	void read(istream*f);

	void convertToString(int kmerLength,bool coloredMode,char*buffer)const;
	double getGuanineCytosineProportion(int kmerLength,bool coloredMode)const;

	bool canHaveChild(const Kmer*otherKmer,int kmerLength)const;
	bool canHaveParent(const Kmer*otherKmer,int kmerLength)const;

	void loadFromTextRepresentation(const char * text);
	uint64_t getTwinHash1(int kmerLength, bool colorSpaceMode) const;
	void getLowerKey(Kmer * kmer, int kmerLength, bool colorSpaceMode) const;

}ATTRIBUTE_PACKED;

/*
 * transform a encoded nucleotide in a char
 */
char codeToChar(uint8_t a,bool color);

int getNumberOfNucleotides(int numberOfKmers,int kmerLength);


#endif
