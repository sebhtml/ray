/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#include <memory/RingAllocator.h>
#include <memory/allocator.h>

#include <string.h>
#include <assert.h>
#include <iostream>
using namespace std;

void RingAllocator::constructor(int chunks,int size,const char*type,bool show){
	resetCount();
	m_chunks=chunks;
	m_max=size;
	strcpy(m_type,type);
	m_numberOfBytes=m_chunks*m_max;
	m_memory=(uint8_t*)__Malloc(sizeof(uint8_t)*m_chunks*m_max,m_type,show);
	m_current=0;
	m_show=show;
}

RingAllocator::RingAllocator(){}

/*
 * allocate a chunk of m_max bytes in constant time
 */
void*RingAllocator::allocate(int a){
	#ifdef ASSERT
	m_count++;
	if(a>m_max){
		cout<<"Request "<<a<<" but maximum is "<<m_max<<endl;
	}
	assert(a<=m_max);
	#endif

	void*address=(void*)(m_memory+m_current*m_max);

	m_current++;

	// depending on the architecture
	// branching (if) can be faster than integer division/modulo

	m_current%=m_chunks;

	return address;
}

int RingAllocator::getSize(){
	return m_max;
}

void RingAllocator::clear(){
	__Free(m_memory,m_type,m_show);
	m_memory=NULL;
}

void RingAllocator::resetCount(){
	m_count=0;
}

int RingAllocator::getCount(){
	return m_count;
}


