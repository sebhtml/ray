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

#ifndef _GridTable
#define _GridTable

#include <structures/Kmer.h>
#include <memory/MyAllocator.h>
#include <graph/KmerAcademy.h>
#include <core/Parameters.h>
#include <structures/Vertex.h>

/**
 * The GridTable  stores  all the k-mers for the graph.
 * Low-coverage (covered once) are not stored here at all.
 * The underlying data structure is a MyHashTable.
 */
class GridTable{
	MyHashTable<Kmer,Vertex> m_hashTable;
	KmerAcademy m_kmerAcademy;
	Parameters*m_parameters;
	uint64_t m_size;
	bool m_inserted;

public:
	void constructor(int rank,Parameters*a);
	uint64_t size();
	Vertex*find(Kmer*key);
	Vertex*insert(Kmer*key);
	bool inserted();

	void addRead(Kmer*a,ReadAnnotation*e);
	ReadAnnotation*getReads(Kmer*a);
	void addDirection(Kmer*a,Direction*d);
	vector<Direction> getDirections(Kmer*a);
	void clearDirections(Kmer*a);
	void buildData(Parameters*a);
	bool isAssembled(Kmer*a);

	bool insertedInAcademy();
	KmerCandidate*insertInAcademy(Kmer*key);
	KmerAcademy*getKmerAcademy();
	MyHashTable<Kmer,Vertex>*getHashTable();
	void printStatistics();
};

#endif
