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
#include <graph/KmerAcademy.h>
#include <core/common_functions.h>
#include <iostream>
#include <cryptography/crypto.h>
#include <stdlib.h>
#include <stdio.h>
#include <core/Parameters.h>
using namespace std;

void KmerAcademy::constructor(int rank,Parameters*parameters){
	m_parameters=parameters;
	m_size=0;
	int chunkSize=16777216; // 16 MiB
	m_allocator.constructor(chunkSize,RAY_MALLOC_TYPE_KMER_ACADEMY,
		m_parameters->showMemoryAllocations());

	m_inserted=false;
	m_gridSize=4194304;
	m_frozen=false;
	int bytes1=m_gridSize*sizeof(KmerCandidate*);
	int bytes2=m_gridSize*sizeof(uint16_t);
	m_gridData=(KmerCandidate**)__Malloc(bytes1,RAY_MALLOC_TYPE_KMER_ACADEMY,m_parameters->showMemoryAllocations());
	m_gridSizes=(uint16_t*)__Malloc(bytes2,RAY_MALLOC_TYPE_KMER_ACADEMY,m_parameters->showMemoryAllocations());

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(rank);
	}

	for(int i=0;i<m_gridSize;i++){
		m_gridSizes[i]=0;
		m_gridData[i]=NULL;
	}
}

uint64_t KmerAcademy::size(){
	return m_size;
}

KmerCandidate*KmerAcademy::find(Kmer*key){
	Kmer lowerKey;
	int bin=hash_function_2(key,m_parameters->getWordSize(),&lowerKey,m_parameters->getColorSpaceMode())%m_gridSize;

	if(key->isLower(&lowerKey)){
		lowerKey=*key;

	}
	for(int i=0;i<m_gridSizes[bin];i++){
		KmerCandidate*gridEntry=m_gridData[bin]+i;
		if(gridEntry->m_lowerKey.isEqual(&lowerKey)){
			return move(bin,i);
		}
	}
	return NULL;
}

KmerCandidate*KmerAcademy::insert(Kmer*key){
	Kmer lowerKey;
	m_inserted=false;
	int bin=hash_function_2(key,m_parameters->getWordSize(),&lowerKey,m_parameters->getColorSpaceMode())%m_gridSize;
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	for(int i=0;i<m_gridSizes[bin];i++){
		KmerCandidate*gridEntry=m_gridData[bin]+i;
		if(gridEntry->m_lowerKey.isEqual(&lowerKey)){
			//cout<<"Found "<<key<<" in bin "<<bin<<endl;
			return move(bin,i);
		}
	}
	KmerCandidate*newEntries=(KmerCandidate*)m_allocator.allocate((m_gridSizes[bin]+1)*sizeof(KmerCandidate));
	for(int i=0;i<m_gridSizes[bin];i++){
		newEntries[i]=m_gridData[bin][i];
	}
	if(m_gridSizes[bin]!=0){
		m_allocator.free(m_gridData[bin],m_gridSizes[bin]*sizeof(KmerCandidate));
	}
	m_gridData[bin]=newEntries;

	m_gridData[bin][m_gridSizes[bin]].m_lowerKey=lowerKey;
	int oldSize=m_gridSizes[bin];
	m_gridSizes[bin]++;
	// check overflow
	assert(m_gridSizes[bin]>oldSize);
	m_inserted=true;
	m_size+=2;
	return move(bin,m_gridSizes[bin]-1);
}

bool KmerAcademy::inserted(){
	return m_inserted;
}

KmerCandidate*KmerAcademy::getElementInBin(int bin,int element){
	#ifdef ASSERT
	assert(bin<getNumberOfBins());
	assert(element<getNumberOfElementsInBin(bin));
	#endif
	return m_gridData[bin]+element;
}

int KmerAcademy::getNumberOfElementsInBin(int bin){
	#ifdef ASSERT
	assert(bin<getNumberOfBins());
	#endif
	return m_gridSizes[bin];
}

int KmerAcademy::getNumberOfBins(){
	return m_gridSize;
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
KmerCandidate*KmerAcademy::move(int bin,int item){
	if(m_frozen){
		return m_gridData[bin]+item;
	}
	KmerCandidate tmp;
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

void KmerAcademy::freeze(){
	m_frozen=true;
}

void KmerAcademy::destructor(){
	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
	}

	int bytes1=m_gridSize*sizeof(KmerCandidate*);
	int bytes2=m_gridSize*sizeof(uint16_t);
	__Free(m_gridData,RAY_MALLOC_TYPE_KMER_ACADEMY,m_parameters->showMemoryAllocations());
	__Free(m_gridSizes,RAY_MALLOC_TYPE_KMER_ACADEMY,m_parameters->showMemoryAllocations());
	uint64_t freed=bytes1+bytes2;
	freed+=m_allocator.getNumberOfChunks()*m_allocator.getChunkSize();
	m_allocator.clear();
	
	if(m_parameters->showMemoryUsage())
		cout<<"Rank "<<m_parameters->getRank()<<": Freeing unused assembler memory: "<<freed/1024<<" KiB freed"<<endl;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
	}
}
