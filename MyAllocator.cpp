/*
 	OpenAssembler -- a de Bruijn DNA assembler for mixed high-throughput technologies
    Copyright (C) 2009  Sébastien Boisvert

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




#include<MyAllocator.h>
#include<stdlib.h>
#include<iostream>
using namespace std;

MyAllocator::MyAllocator(){
}

void MyAllocator::constructor(int chunkSize){
	m_CHUNK_SIZE=chunkSize; // 10M
	m_currentChunk=(void*)malloc(m_CHUNK_SIZE);
	if(m_currentChunk==NULL){
		cout<<"NULL"<<endl;
	}
	m_chunks.push_back(m_currentChunk);
	m_currentPosition=0;
	m_jobs=0;
}

void*MyAllocator::allocate(int s){
	s+=s%8;
	if(s>m_CHUNK_SIZE){
		return NULL;
	}
	
	m_jobs++;
	int left=m_CHUNK_SIZE-m_currentPosition;
	if(s>left){
		m_currentChunk=malloc(m_CHUNK_SIZE);
		m_chunks.push_back(m_currentChunk);
		m_currentPosition=0;
		return allocate(s);
	}
	void*r=(void*)((char*)m_currentChunk+m_currentPosition);
	m_currentPosition+=s;
	return r;
}


MyAllocator::~MyAllocator(){
	clear();
}

void MyAllocator::clear(){
	for(int i=0;i<(int)m_chunks.size();i++){
		free(m_chunks[i]);
	}
	m_chunks.clear();
}

int MyAllocator::getChunkSize(){
	return m_CHUNK_SIZE;
}

void MyAllocator::print(){
	cout<<"Jobs="<<m_jobs<<endl;
	cout<<"ChunkSize="<<m_CHUNK_SIZE<<endl;
	cout<<"EatenRams="<<m_chunks.size()<<endl;
	cout<<"TotalBytesConsumed="<<m_CHUNK_SIZE*m_chunks.size()<<endl;
}

int MyAllocator::getNumberOfChunks(){
	return m_chunks.size();
}
