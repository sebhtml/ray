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

#include <MyAllocator.h>
#include <VertexData.h>

class VertexTable{
	uint64_t m_size;
	bool m_inserted;
	MyAllocator m_gridAllocator;
	VertexData**m_gridData;
	uint16_t*m_gridSizes;
	//uint16_t*m_gridReservedSizes;
	int m_gridSize;
	bool m_frozen;
	int m_wordSize;

	/**
 *   move the item in front of the others
 */
	VertexData*move(int bin,int item);
public:
	void constructor(int rank);
	void setWordSize(int w);
	uint64_t size();
	VertexData*find(uint64_t key);
	VertexData*insert(uint64_t key);
	bool inserted();
	void remove(uint64_t a);
	VertexData*getElementInBin(int bin,int element);
	int getNumberOfElementsInBin(int bin);
	int getNumberOfBins();
	MyAllocator*getAllocator();
	void freeze();
	void unfreeze();
	bool frozen();

	void addRead(uint64_t a,ReadAnnotation*e);
	ReadAnnotation*getReads(uint64_t a);
	void addDirection(uint64_t a,Direction*d);
	vector<Direction> getDirections(uint64_t a);
	void clearDirections(uint64_t a);

};

#endif
