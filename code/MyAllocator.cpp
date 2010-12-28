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


#include<common_functions.h>
#include<MyAllocator.h>
#include<stdlib.h>
#include<iostream>
#include<assert.h>
using namespace std;

MyAllocator::MyAllocator(){
	m_addressesToReuse=NULL;
	m_numberOfAddressesToReuse=0;
	m_growthRate=1000;
	m_maxSize=0;
}

/**
 * reset the currentPosition and reuse allocated memory.
 */
void MyAllocator::reset(){
	m_currentPosition=0;
}

void MyAllocator::constructor(int chunkSize){
	m_CHUNK_SIZE=chunkSize; 
	m_currentChunk=(void*)__Malloc(m_CHUNK_SIZE);
	#ifdef ASSERT
	assert(m_currentChunk!=NULL);
	#endif
	m_chunks.push_back(m_currentChunk);
	m_currentPosition=0;
}

void*MyAllocator::allocate(int s){
	if(hasAddressesToReuse()){
		cout<<"reuse"<<endl;
		return reuseAddress();
	}

	// hopefully fix alignment issues on Itanium
	int alignment=8;
	if(s%8!=0){
		s=roundNumber(s,alignment);
	}
	#ifdef ASSERT
	assert(s%8==0);
	#endif


	#ifdef ASSERT
	assert(m_currentChunk!=NULL);
	#endif

	#ifdef ASSERT
	if(s>m_CHUNK_SIZE){
		cout<<"Requested "<<s<<" -- only have "<<m_CHUNK_SIZE<<endl;
	}
	assert(s<=m_CHUNK_SIZE);
	#endif
	if(s>m_CHUNK_SIZE){
		cout<<"Critical exception: The length of the requested memory exceeds the CHUNK_SIZE: "<<s<<" > "<<m_CHUNK_SIZE<<endl;
		assert(false);
		return NULL;// you asked too much.., this is critical..
	}
	
	int left=m_CHUNK_SIZE-m_currentPosition;
	if(s>left){
		m_currentChunk=__Malloc(m_CHUNK_SIZE);
		#ifdef ASSERT
		assert(m_currentChunk!=NULL);
		#endif
		m_chunks.push_back(m_currentChunk);
		m_currentPosition=0;
		return allocate(s);
	}
	
	// the address is the head of currentCHunk+ m_currentPosition bytes
	void*r=(void*)(((char*)m_currentChunk)+m_currentPosition);
	// increase the current position.
	m_currentPosition+=s;
	return r;
}


MyAllocator::~MyAllocator(){
	clear();
}

void MyAllocator::clear(){
	for(int i=0;i<(int)m_chunks.size();i++){
		__Free(m_chunks[i]);
	}
	m_chunks.clear();
}

int MyAllocator::getChunkSize(){
	return m_CHUNK_SIZE;
}

void MyAllocator::print(){
	cout<<"ChunkSize="<<m_CHUNK_SIZE<<endl;
	cout<<"EatenRams="<<m_chunks.size()<<endl;
	cout<<"TotalBytesConsumed="<<m_CHUNK_SIZE*m_chunks.size()<<endl;
}

int MyAllocator::getNumberOfChunks(){
	return m_chunks.size();
}

bool MyAllocator::hasAddressesToReuse(){
	return m_numberOfAddressesToReuse>0;
}

void*MyAllocator::reuseAddress(){
	m_numberOfAddressesToReuse--;
	void*ptr=m_addressesToReuse[m_numberOfAddressesToReuse];
	if(m_numberOfAddressesToReuse==0){
		free(m_addressesToReuse);
		m_addressesToReuse=NULL;
	}
	return ptr;
}

void MyAllocator::addAddressToReuse(void*ptr){
	if(m_numberOfAddressesToReuse==m_maxSize || m_addressesToReuse==NULL){
		m_maxSize+=m_growthRate;
		m_addressesToReuse=(void**)__Realloc(m_addressesToReuse,m_maxSize*sizeof(void*));
	}
	m_addressesToReuse[m_numberOfAddressesToReuse]=ptr;
	m_numberOfAddressesToReuse++;
}

void MyAllocator::printMemoryUsage(int rank,const char*description){
	uint64_t bytes=m_CHUNK_SIZE*m_chunks.size();
	int megaCount=bytes/1000/1000;

	cout<<"Rank "<<rank<<": memory usage for "<<description<<" is "<<megaCount<<" MB (10^6)"<<endl;
}
