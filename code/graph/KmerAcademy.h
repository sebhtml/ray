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

#ifndef _KmerAcademy_H
#define _KmerAcademy_H

#include <memory/MyAllocator.h>
#include <structures/Kmer.h>
#include <core/Parameters.h>
#include <stdint.h>

class KmerCandidate{
public:
	Kmer m_lowerKey;
	uint8_t m_count;
};

class KmerAcademy{
	Parameters*m_parameters;
	uint64_t m_size;
	bool m_inserted;
	MyAllocator m_allocator;
	KmerCandidate**m_gridData;
	uint16_t*m_gridSizes;
	int m_gridSize;
	bool m_frozen;

	/**
 *   move the item in front of the others
 */
	KmerCandidate*move(int bin,int item);
public:
	void constructor(int rank,Parameters*a);
	uint64_t size();
	KmerCandidate*find(Kmer*key);
	KmerCandidate*insert(Kmer*key);
	bool inserted();
	KmerCandidate*getElementInBin(int bin,int element);
	int getNumberOfElementsInBin(int bin);
	int getNumberOfBins();
	void freeze();
	void destructor();
};

#endif
