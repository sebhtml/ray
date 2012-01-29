/*
 	Ray
    Copyright (C)  2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _MyAllocator
#define _MyAllocator

#include <vector>
#include <memory/ReusableMemoryStore.h>
using namespace std;

/**
 * all memory allocations that are pervasive use this allocator.
 * MyAllocator is a simple CHUNK_ALLOCATOR.
 * it allocates m_CHUNK_SIZE bytes internally.
 * when one requests memory, it uses this *chunk*.
 * it the chunk gets depleted, MyAllocator renews a new one and store the depleted one in its internals.
 * Also, MyAllocator reuses automatically memory freed with MyAllocator::free()
 * \author Sébastien Boisvert
 */
class MyAllocator{
	char m_type[101];
	bool m_show;
	vector<void*>m_chunks;
	int m_currentPosition;
	int m_currentChunkId;
	int m_CHUNK_SIZE;

/** garbage store, memory will be reused */
	ReusableMemoryStore m_store;

	void addChunk();

public:
	MyAllocator();
	/**
 * print allocator information
 */
	void print();
	void clear();

	/**
 * assign a size to the allocator.
 */
	void constructor(int a,const char*type,bool show);

/**
 * allocate memory
 */
	void*allocate(int s);

	~MyAllocator();
	int getChunkSize();
	int getNumberOfChunks();

/**
 * return the container for reusable memory
 */
	/**
 	* reset the chunk to reuse it properly.
 	*/
	void reset();

/** free memory */
	void free(void*a,int b);
};

int roundNumber(int number,int alignment);


#endif
