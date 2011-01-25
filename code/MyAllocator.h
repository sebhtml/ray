/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

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


#ifndef _MyAllocator
#define _MyAllocator

#include<vector>
using namespace std;

/*
 * all memory allocations that are pervasive use this allocator.
 * MyAllocator is a simple CHUNK_ALLOCATOR.
 * it allocates m_CHUNK_SIZE bytes internally.
 * when one requests memory, it uses this *chunk*.
 * it the chunk gets depleted, MyAllocator renews a new one and store the depleted one in its internals.
 *
 */
class MyAllocator{
	vector<void*>m_chunks;
	void*m_currentChunk;
	int m_currentPosition;
	int m_CHUNK_SIZE;

	void**m_addressesToReuse;
	int m_numberOfAddressesToReuse;
	int m_growthRate;
	int m_maxSize;

	bool hasAddressesToReuse();
	void*reuseAddress();
public:
	/**
 * allocator.
 */
	MyAllocator();
	void print();
	/**
 	* reset the chunk to reuse it properly.
 	*/
	void reset();
	void clear();
	/**
 * assign a size to the allocator.
 */
	void constructor(int a);
	void*allocate(int s);
	~MyAllocator();
	int getChunkSize();
	int getNumberOfChunks();
	void addAddressToReuse(void*ptr);
	void freeAddressesToReuse();
	void resetMemory();
};


#endif
