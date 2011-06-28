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
#include <structures/MyHashTable.h>
#include <stdint.h>

/** a KmerCandidate may become a genomic super-star --
 * one of the fews that make it to the graph
 */
class KmerCandidate{
public:
	Kmer m_lowerKey;
	COVERAGE_TYPE m_count;
}ATTRIBUTE_PACKED;

/**
 * The KmerAcademy is the place where KmerCandidate  
 * train to become  Vertex.
 * If they fail to endure their hard training,
 * they remain here. Usually, those failing are observed
 * only once.
 * The KmerAcademy is burned and destroyed once the graph is ready.
 */
class KmerAcademy{
	Parameters*m_parameters;
	uint64_t m_size;
	bool m_inserted;
	MyHashTable<Kmer,KmerCandidate> m_hashTable;

public:
	void constructor(int rank,Parameters*a);
	uint64_t size();
	KmerCandidate*find(Kmer*key);
	KmerCandidate*insert(Kmer*key);
	bool inserted();
	void destructor();
	MyHashTable<Kmer,KmerCandidate>*getHashTable();
	void printStatistics();
};

#endif
