/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#ifndef _ArrayOfReads
#define _ArrayOfReads

#include<plugin_SequencesLoader/Read.h>

/**
 * This class holds reads. These are stored in chunks, which are utterly linked in chains.
 * When a chunk is full, another one is added, and linked to the full one aforementionned.
 * \author Sébastien Boisvert
 */
class ArrayOfReads{
	MyAllocator*m_allocator;
	int m_CHUNK_SIZE;
	Read**m_chunks;
	int m_numberOfChunks;
	int m_maxNumberOfChunks;
	LargeCount m_elements;
	LargeCount m_maxSize;
public:
	/**
 * 	add a read
 */
	void push_back(Read*a);

/** 
 * get a read
 */
	Read*at(LargeIndex i);
	Read*operator[](LargeIndex i);
	LargeCount size();
	void clear();
	void constructor(MyAllocator*allocator);
	void reset();
};

#endif
