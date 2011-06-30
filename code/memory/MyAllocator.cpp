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

#include <core/common_functions.h>
#include <memory/MyAllocator.h>
#include <stdlib.h>
#include <memory/malloc_types.h>
#include <iostream>
#include <stdio.h>
#include <assert.h>
using namespace std;

/**
 * reset the currentPosition and reuse allocated memory.
 */
void MyAllocator::reset(){
	m_currentPosition=0;
	m_currentChunkId=0;
	m_store.reset();
}

/** the constructor does not allocate a chunk */
void MyAllocator::constructor(int chunkSize,int type,bool show){
	m_show=show;
	m_CHUNK_SIZE=chunkSize; 
	m_type=type;
}

/**  add a chunk of memory */
void MyAllocator::addChunk(){
	void*currentChunk=(void*)__Malloc(m_CHUNK_SIZE,m_type,m_show);
	#ifdef ASSERT
	assert(currentChunk!=NULL);
	#endif
	m_chunks.push_back(currentChunk);
	m_currentChunkId=0;
	m_currentPosition=0;
}

void*MyAllocator::allocate(int s){
	#ifdef ASSERT
	assert(s!=0);
	#endif

	/* reuse memory freed elsewhere */
	if(m_store.hasAddressesToReuse(s))
		return m_store.reuseAddress(s);

	/* add a first chunk to the pool */
	if(m_chunks.size()==0)
		addChunk();

	#ifndef FORCE_PACKING
	// hopefully fix alignment issues on Itanium
	int alignment=8;
	if(s%8!=0){
		s=roundNumber(s,alignment);
	}
	#ifdef ASSERT
	assert(s%8==0);
	#endif
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
		if(!(m_currentChunkId+1<(int)m_chunks.size())){
			void*currentChunk=__Malloc(m_CHUNK_SIZE,m_type,m_show);
			m_chunks.push_back(currentChunk);
		}
		m_currentChunkId++;
		m_currentPosition=0;
		return allocate(s);
	}
	
	#ifdef ASSERT
	if(m_currentChunkId>=(int)m_chunks.size()){
		cout<<"ChunkId="<<m_currentChunkId<<" Chunks="<<m_chunks.size()<<endl;
	}
	
	assert(m_currentChunkId<(int)m_chunks.size());
	if(m_currentPosition>=m_CHUNK_SIZE){
		cout<<"Error: ToAllocate="<<s<<" Chunks="<<m_chunks.size()<<" CurrentChunk="<<m_currentChunkId<<" ChunkPosition="<<m_currentPosition<<" ChunkSize="<<m_CHUNK_SIZE<<endl;
		exit(EXIT_NO_MORE_MEMORY);
	}
	assert(m_currentPosition<m_CHUNK_SIZE);
	#endif

	// the address is the head of currentCHunk+ m_currentPosition bytes
	void*r=(void*)(((char*)m_chunks[m_currentChunkId])+m_currentPosition);
	// increase the current position.
	m_currentPosition+=s;
	return r;
}

MyAllocator::~MyAllocator(){}

void MyAllocator::clear(){
	m_store.reset();
	for(int i=0;i<(int)m_chunks.size();i++){
		__Free(m_chunks[i],m_type,m_show);
	}
	m_chunks.clear();
	m_currentPosition=0;
	m_currentChunkId=0;
}

int MyAllocator::getChunkSize(){
	return m_CHUNK_SIZE;
}

void MyAllocator::print(){
	cout<<"ChunkSize= "<<m_CHUNK_SIZE<<endl;
	cout<<"AllocatedChunks= "<<m_chunks.size()<<endl;
	cout<<"TotalBytesConsumed= "<<m_CHUNK_SIZE*m_chunks.size()<<endl;
	m_store.print();
}

int MyAllocator::getNumberOfChunks(){
	return m_chunks.size();
}

void MyAllocator::free(void*a,int b){
	m_store.addAddressToReuse(a,b);
}

MyAllocator::MyAllocator(){
}


