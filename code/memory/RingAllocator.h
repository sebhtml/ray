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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _RingAllocator
#define _RingAllocator

#include<set>
#include<stdint.h>
using namespace std;

/**
 * This class is a ring buffer. No !, it is an allocator. Thus, referred to as a ring allocator.
 *
 * This is an allocator that can allocate up to <m_chunks> allocations of exactly <m_max> bytes.
 * allocation and free are done both in constant time (yeah!)
 */
class RingAllocator{
	int m_type;
	int m_count;
	int m_chunks;
	int m_numberOfBytes;
	int m_max;
	uint8_t*m_memory;
	int m_current;
public:
	RingAllocator();
	void constructor(int chunks,int size,int type);
	void*allocate(int a);
	int getSize();
	void clear();
	void resetCount();
	int getCount();
};


#endif

