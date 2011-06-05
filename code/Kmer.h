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

#include <constants.h>
#include <stdint.h>
#include <vector>
using namespace std;

#define KMER_REQUIRED_BITS 2*MAXKMERLENGTH
#define KMER_REQUIRED_BYTES KMER_REQUIRED_BITS/8
#define KMER_REQUIRED_BYTES_MODULO KMER_REQUIRED_BITS%8
#if KMER_REQUIRED_BYTES_MODULO
	#define KMER_BYTES KMER_REQUIRED_BYTES+1
#else
	#define KMER_BYTES KMER_REQUIRED_BYTES
#endif

#define KMER_UINT64_T KMER_BYTES/8
#define KMER_UINT64_T_MODULO KMER_BYTES%8
#if KMER_UINT64_T_MODULO
	#define KMER_U64_ARRAY_SIZE KMER_UINT64_T+1
#else
	#define KMER_U64_ARRAY_SIZE KMER_UINT64_T
#endif



/**
 * Class for storing k-mers.
 * For now only an array of uint64_t is present, but later,
 * when the code is stable, this could be a mix of u64, u32 and u16 and u8
 * while maintening the same interface, that are the two functions.
 *
 */
class Kmer{
	uint64_t m_u64[KMER_U64_ARRAY_SIZE];
public:
	Kmer();
	~Kmer();
	bool isEqual(const Kmer*a)const ;
	bool isLower(const Kmer*a)const ;
	void print()const;
	void pack(uint64_t*messageBuffer,int*messagePosition)const ;
	void unpack(uint64_t*messageBuffer,int*messagePosition);
	void unpack(vector<uint64_t>*messageBuffer,int*messagePosition);
	bool operator<(const Kmer&b) const;
	void operator=(const Kmer&b);
	bool operator!=(const Kmer&b)const;
	bool operator==(const Kmer&b) const;

	void setU64(int i,uint64_t b);
	uint64_t getU64(int i)const;
	int getNumberOfU64()const;

}ATTRIBUTE_PACKED;


#endif
