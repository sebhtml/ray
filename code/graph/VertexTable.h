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

#ifndef _VertexTable
#define _VertexTable

#include <memory/MyAllocator.h>
#include <structures/VertexData.h>
#include <memory/OnDiskAllocator.h>
#include <core/Parameters.h>

class VertexTable{
	Parameters*m_parameters;
	uint64_t m_size;
	bool m_inserted;
	MyAllocator*m_gridAllocator;
	VertexData**m_gridData;
	uint16_t*m_gridSizes;
	int m_gridSize;
	bool m_frozen;
	int m_wordSize;

	/**
 *   move the item in front of the others
 */
	VertexData*move(int bin,int item);
public:
	void constructor(int rank,MyAllocator*allocator,Parameters*a);
	void setWordSize(int w);
	uint64_t size();
	VertexData*find(Kmer*key);
	VertexData*insert(Kmer*key);
	bool inserted();
	void remove(Kmer*a);
	VertexData*getElementInBin(int bin,int element);
	int getNumberOfElementsInBin(int bin);
	int getNumberOfBins();
	MyAllocator*getAllocator();
	void freeze();
	void unfreeze();
	bool frozen();

	void addRead(Kmer*a,ReadAnnotation*e);
	ReadAnnotation*getReads(Kmer*a);
	void addDirection(Kmer*a,Direction*d);
	vector<Direction> getDirections(Kmer*a);
	void clearDirections(Kmer*a);

};

#endif
