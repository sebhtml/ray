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

#ifndef _BloomFilter_H
#define _BloomFilter_H

#include <stdint.h>
#include <plugin_KmerAcademyBuilder/Kmer.h>

/**
 * Bloom filter implementation
 * This is a drop-in replacement thanks to the KmerAcademy design.
 * \see http://en.wikipedia.org/wiki/Bloom_filter
 * \author Sébastien Boisvert
 */
class BloomFilter{
	/** the bits */
	uint64_t*m_bitmap;

	/** the number of bits */
	uint64_t m_bits;

/**
 * Number of used bits
 */
	uint64_t m_numberOfSetBits;

	uint64_t m_numberOfInsertions;

	/** the number of hash functions */
	int m_hashFunctions;

	/** a random number for each hash function */
	uint64_t m_hashNumbers[8];
public:
	/** initialize the filter */
	void constructor(uint64_t bits);
	/** check for a value */
	bool hasValue(Kmer*kmer);
	/** check is a value was inserted. false positive rate is not 0 */
	void insertValue(Kmer*kmer);
	/** destroy the BloomFilter */
	void destructor();

	uint64_t getNumberOfBits();
	uint64_t getNumberOfSetBits();

	uint64_t getNumberOfInsertions();
};

#endif
