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

#include <plugin_KmerAcademyBuilder/BloomFilter.h>
#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <memory/allocator.h>
#include <iostream>
#ifdef ASSERT
#include <assert.h>
#endif
using namespace std;

void BloomFilter::constructor(uint64_t numberOfBits){
	m_numberOfSetBits=0;
	m_numberOfInsertions=0;

/*
http://pages.cs.wisc.edu/~cao/papers/summary-cache/node8.html

n = 50 000 000
m/n = 10
m = 500 000 000 bits
k = 8

false positive rate = 0.00846 = 0.846%
*/

	m_bits=numberOfBits; /* 500 000 000 */

	#ifdef ASSERT
	assert(numberOfBits>0);
	#endif

	m_hashFunctions=0;

/**
 * these "magic" values are just random bits
they were computed with:

date|md5sum

(and taking the first 16 symbols,
1 hexadecimal symbol = 4 bits
2*4 bits = 1 byte
8 bytes = 1 64-bit integer)

Basically, these are used in XOR hash functions below...

ULL means unsigned long long, it is necessary on some architectures
*/

	m_hashNumbers[m_hashFunctions++]=0xe70b369c4c19f0f9ULL;
	m_hashNumbers[m_hashFunctions++]=0xbeb7f38b993441a2ULL;
	m_hashNumbers[m_hashFunctions++]=0x3149cd9246ca7995ULL;
	m_hashNumbers[m_hashFunctions++]=0x2ef5b2c17d479ee8ULL;
	m_hashNumbers[m_hashFunctions++]=0xfb8daceab90fe233ULL;
	m_hashNumbers[m_hashFunctions++]=0x20fa74e37d497859ULL;
	m_hashNumbers[m_hashFunctions++]=0x9007d0caef749698ULL;
	m_hashNumbers[m_hashFunctions++]=0x4e4100a5605ef967ULL;

	#ifdef ASSERT
	assert(m_hashFunctions == 8);
	assert(m_bits % 64 == 0);
	#endif

	uint64_t requiredBytes=m_bits/8;
	uint64_t required8Bytes=requiredBytes/8;

/*
 * We need more 64-bit integers for storing
 * these extra bits.
 */

	if(m_bits%64!=0)
		required8Bytes++;

	m_bitmap=(uint64_t*)__Malloc(required8Bytes*sizeof(uint64_t), "RAY_MALLOC_TYPE_BLOOM_FILTER", false); /* about 62 MB of memory */

	cout<<"[BloomFilter] allocated "<<required8Bytes*sizeof(uint64_t)<<" bytes for table with "<<m_bits<<" bits"<<endl;
	cout<<"[BloomFilter] hash numbers:";

	for(int i=0;i<m_hashFunctions;i++){
		cout<<hex<<" "<<m_hashNumbers[i];
	}

	cout<<dec<<endl;

	#ifdef ASSERT
	assert(required8Bytes > 0);
	assert(m_bitmap != NULL);
	#endif

/*
 * Set all the bits to 0.
 */
	for(uint64_t i=0;i<required8Bytes;i++){
		m_bitmap[i]=0;
	}
}

bool BloomFilter::hasValue(Kmer*kmer){

	uint64_t origin=kmer->hash_function_2();

	for(int i=0;i<m_hashFunctions;i++){
		uint64_t hashValue = origin ^ m_hashNumbers[i];
		uint64_t bit=hashValue % m_bits;
		uint64_t chunk=bit/64;
		int bitInChunk=bit%64;
		int bitValue=(m_bitmap[chunk] << (63-bitInChunk)) >> 63;

		#ifdef ASSERT
		assert(bitValue == 0 || bitValue == 1);
		#endif

		/* if one bit is 0, the object is not in the BloomFilter */
		if(bitValue == 0)
			return false;

		#ifdef ASSERT
		assert(bitValue == 1);
		#endif
	}

	/* the object is in the BloomFilter or this is a false positive */
	return true;
}

void BloomFilter::insertValue(Kmer*kmer){

	uint64_t origin = kmer->hash_function_2();

	#ifdef ASSERT
	assert(m_hashFunctions == 8);
	#endif

	for(int i=0;i<m_hashFunctions;i++){

		uint64_t hashValue = origin ^ m_hashNumbers[i];
		uint64_t bit=hashValue % m_bits;
		uint64_t chunk=bit/64;
		uint64_t bitInChunk=bit%64;

		int oldBitValue=(m_bitmap[chunk] << (63-bitInChunk)) >> 63;

/*
 * The bit is already set to 1. We don't need to do anything else.
 */
		if(oldBitValue==1)
			continue;


		uint64_t filter=1;

		filter <<= bitInChunk;

		m_bitmap[chunk] |= filter;

		#ifdef ASSERT
		int bitValue=(m_bitmap[chunk] << (63-bitInChunk)) >> 63;
		if(bitValue != 1)
			cout<<"Fatal: bit is "<<bitValue<<" but should be 1 bit="<<bit<<" chunk="<<chunk<<" bitInChunk="<<bitInChunk<<" chunkContent="<<m_bitmap[chunk]<<" filter="<<filter<<endl;
		assert(bitValue == 1);
		#endif

/*
 * We increased the number of set bits by 1.
 */
		m_numberOfSetBits++;
	}

	#ifdef ASSERT
	assert(hasValue(kmer));
	#endif

	m_numberOfInsertions++;
}

void BloomFilter::destructor(){

	#ifdef ASSERT
	assert(m_bitmap != NULL);
	assert(m_hashFunctions > 0);
	assert(m_bits > 0);
	#endif

	__Free(m_bitmap,"RAY_MALLOC_TYPE_BLOOM_FILTER",false);
	m_bitmap=NULL;
	m_bits=0;
	m_hashFunctions=0;

	#ifdef ASSERT
	assert(m_bitmap == NULL);
	#endif
}

uint64_t BloomFilter::getNumberOfBits(){
	return m_bits;
}

uint64_t BloomFilter::getNumberOfSetBits(){
	return m_numberOfSetBits;
}

uint64_t BloomFilter::getNumberOfInsertions(){
	return m_numberOfInsertions;
}
