/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#include<OutboxAllocator.h>
#include<common_functions.h>
#include<assert.h>


void OutboxAllocator::constructor(int chunks,int size){
	m_chunks=chunks;
	m_max=size;
	m_numberOfBytes=m_chunks*m_max;
	m_memory=(uint8_t*)__Malloc(sizeof(uint8_t)*m_chunks*m_max);
	m_availableChunks=(uint16_t*)__Malloc(sizeof(uint16_t)*m_chunks);
	m_numberOfAvailableChunks=0;
	while(m_numberOfAvailableChunks<m_chunks){
		m_availableChunks[m_numberOfAvailableChunks]=m_numberOfAvailableChunks;
		m_numberOfAvailableChunks++;
	}
}

OutboxAllocator::OutboxAllocator(){
}

void*OutboxAllocator::allocate(int a){
	assert(m_numberOfAvailableChunks!=0);
	assert(a<=m_max);
	m_numberOfAvailableChunks--;
	int i=m_availableChunks[m_numberOfAvailableChunks];
	void*address=(void*)(m_memory+i*m_max);
	return address;
}

void OutboxAllocator::free(void*a){
	if(a==NULL){
		return;
	}

	uint64_t start=(uint64_t)m_memory;
	uint64_t toBeFreed=(uint64_t)a;
	int differenceInBytes=toBeFreed-start;
	int i=differenceInBytes/m_max;

	if(i>=0 && i<m_chunks){// else this chunk is not from this allocator.
		m_availableChunks[m_numberOfAvailableChunks]=i;
		m_numberOfAvailableChunks++;
	}
}
