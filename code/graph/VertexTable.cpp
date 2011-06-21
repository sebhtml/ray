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

#include <memory/malloc_types.h>
#include <assert.h>
#include <graph/VertexTable.h>
#include <core/common_functions.h>
#include <iostream>
#include <cryptography/crypto.h>
#include <stdlib.h>
#include <stdio.h>
#include <core/Parameters.h>
using namespace std;

void VertexTable::constructor(int rank,MyAllocator*allocator,Parameters*parameters){
	m_parameters=parameters;
	m_gridAllocator=allocator;
	m_size=0;
	m_inserted=false;
	m_gridSize=4194304;
	int bytes1=m_gridSize*sizeof(VertexData*);
	m_gridData=(VertexData**)__Malloc(bytes1,RAY_MALLOC_TYPE_VERTEX_TABLE_DATA,m_parameters->showMemoryAllocations());
	int bytes2=m_gridSize*sizeof(uint16_t);
	m_gridSizes=(uint16_t*)__Malloc(bytes2,RAY_MALLOC_TYPE_VERTEX_TABLE_SIZES,m_parameters->showMemoryAllocations());

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(rank);
	}

	for(int i=0;i<m_gridSize;i++){
		m_gridSizes[i]=0;
		m_gridData[i]=NULL;
	}
}

uint64_t VertexTable::size(){
	return m_size;
}

VertexData*VertexTable::find(Kmer*key){
	Kmer lowerKey;
	int bin=hash_function_2(key,m_wordSize,&lowerKey,m_parameters->getColorSpaceMode())%m_gridSize;

	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	for(int i=0;i<m_gridSizes[bin];i++){
		VertexData*gridEntry=m_gridData[bin]+i;
		if(gridEntry->m_lowerKey.isEqual(&lowerKey)){
			return move(bin,i);
		}
	}
	return NULL;
}

VertexData*VertexTable::insert(Kmer*key){
	Kmer lowerKey;
	m_inserted=false;
	int bin=hash_function_2(key,m_wordSize,&lowerKey,m_parameters->getColorSpaceMode())%m_gridSize;
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	for(int i=0;i<m_gridSizes[bin];i++){
		VertexData*gridEntry=m_gridData[bin]+i;
		if(gridEntry->m_lowerKey.isEqual(&lowerKey)){
			//cout<<"Found "<<key<<" in bin "<<bin<<endl;
			return move(bin,i);
		}
	}
	if(true){
		VertexData*newEntries=(VertexData*)m_gridAllocator->allocate((m_gridSizes[bin]+1)*sizeof(VertexData));
		for(int i=0;i<m_gridSizes[bin];i++){
			newEntries[i]=m_gridData[bin][i];
		}
		if(m_gridSizes[bin]!=0){
			m_gridAllocator->free(m_gridData[bin],m_gridSizes[bin]*sizeof(VertexData));
		}
		m_gridData[bin]=newEntries;
	}

	m_gridData[bin][m_gridSizes[bin]].m_lowerKey=lowerKey;
	m_gridData[bin][m_gridSizes[bin]].constructor();
	int oldSize=m_gridSizes[bin];
	m_gridSizes[bin]++;
	// check overflow
	assert(m_gridSizes[bin]>oldSize);
	m_inserted=true;
	m_size+=2;
	return move(bin,m_gridSizes[bin]-1);
}

bool VertexTable::inserted(){
	return m_inserted;
}

void VertexTable::remove(Kmer*a){
}

VertexData*VertexTable::getElementInBin(int bin,int element){
	#ifdef ASSERT
	assert(bin<getNumberOfBins());
	assert(element<getNumberOfElementsInBin(bin));
	#endif
	return m_gridData[bin]+element;
}

int VertexTable::getNumberOfElementsInBin(int bin){
	#ifdef ASSERT
	assert(bin<getNumberOfBins());
	#endif
	return m_gridSizes[bin];
}

int VertexTable::getNumberOfBins(){
	return m_gridSize;
}

MyAllocator*VertexTable::getAllocator(){
	return m_gridAllocator;
}

void VertexTable::freeze(){
	m_frozen=true;
}

void VertexTable::unfreeze(){
	m_frozen=false;
}

bool VertexTable::frozen(){
	return m_frozen;
}

/*
 *         0 1 2 3 4 5 6 7 
 *  input: a b c d e f g h
 *
 *  move(4)  // e
 *
 * 	    0 1 2 3 4 5 6 7
 *  output: e a b c d f g h
 *
 *
 */
VertexData*VertexTable::move(int bin,int item){
	if(m_frozen){
		return m_gridData[bin]+item;
	}
	VertexData tmp;
	#ifdef ASSERT
	assert(item<getNumberOfElementsInBin(bin));
	#endif
	tmp=m_gridData[bin][item];
	for(int i=item-1;i>=0;i--){
		m_gridData[bin][i+1]=m_gridData[bin][i];
	}
	m_gridData[bin][0]=tmp;
	return m_gridData[bin];
}

void VertexTable::setWordSize(int w){
	m_wordSize=w;
}

void VertexTable::addRead(Kmer*a,ReadAnnotation*e){
	VertexData*i=insert(a);
	i->addRead(a,e);
	#ifdef ASSERT
	ReadAnnotation*reads=i->getReads(a);
	assert(reads!=NULL);
	#endif
}

ReadAnnotation*VertexTable::getReads(Kmer*a){
	VertexData*i=find(a);
	if(i==NULL){
		return NULL;
	}
	ReadAnnotation*reads=i->getReads(a);
	return reads;
}

void VertexTable::addDirection(Kmer*a,Direction*d){
	VertexData*i=insert(a);
	i->addDirection(a,d);
}

vector<Direction> VertexTable::getDirections(Kmer*a){
	VertexData*i=find(a);
	if(i==NULL){
		vector<Direction> p;
		return p;
	}
	return i->getDirections(a);
}

void VertexTable::clearDirections(Kmer*a){
	VertexData*i=find(a);
	if(i!=NULL){
		i->clearDirections(a);
	}
}

