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

#include <assert.h>
#include <GridTable.h>
#include <common_functions.h>
#include <crypto.h>
#include <stdlib.h>
#include <stdio.h>

void GridTable::constructor(int rank){
	m_size=0;
	m_inserted=false;
	m_gridSize=4194304;
	int bytes1=m_gridSize*sizeof(GridData*);
	m_gridData=(GridData**)__Malloc(bytes1);
	int bytes2=m_gridSize*sizeof(uint16_t);
	m_gridSizes=(uint16_t*)__Malloc(bytes2);
	int bytes3=m_gridSize*sizeof(uint16_t);
	m_gridReservedSizes=(uint16_t*)__Malloc(bytes3);
	printf("Rank %i: allocating %i bytes for grid table\n",rank,bytes1+bytes2+bytes3);
	fflush(stdout);
	for(int i=0;i<m_gridSize;i++){
		m_gridSizes[i]=0;
		m_gridData[i]=NULL;
		m_gridReservedSizes[i]=0;
	}
	m_gridAllocator.constructor();
}

uint64_t GridTable::size(){
	return m_size;
}

Vertex*GridTable::find(uint64_t key){
	int bin=uniform_hashing_function_2_64_64(key)%m_gridSize;
	for(int i=0;i<m_gridSizes[bin];i++){
		GridData*gridEntry=m_gridData[bin]+i;
		if(gridEntry->m_key==key){
			return move(bin,i);
		}
	}
	return NULL;
}

Vertex*GridTable::insert(uint64_t key){
	m_inserted=false;
	int bin=uniform_hashing_function_2_64_64(key)%m_gridSize;
	for(int i=0;i<m_gridSizes[bin];i++){
		GridData*gridEntry=m_gridData[bin]+i;
		if(gridEntry->m_key==key){
			return move(bin,i);
		}
	}
	if(m_gridReservedSizes[bin]==m_gridSizes[bin]){
		GridData*newEntries=m_gridAllocator.allocate(m_gridSizes[bin]+1,m_gridReservedSizes+bin);
		for(int i=0;i<m_gridSizes[bin];i++){
			newEntries[i].m_key=m_gridData[bin][i].m_key;
			newEntries[i].m_value=m_gridData[bin][i].m_value;
		}
		if(m_gridSizes[bin]!=0){
			m_gridAllocator.free(m_gridData[bin],m_gridSizes[bin]);
		}
		m_gridData[bin]=newEntries;
	}

	m_gridData[bin][m_gridSizes[bin]].m_key=key;
	int oldSize=m_gridSizes[bin];
	m_gridSizes[bin]++;
	// check overflow
	assert(m_gridSizes[bin]>oldSize);
	m_inserted=true;
	m_size++;
	return move(bin,m_gridSizes[bin]-1);
}

bool GridTable::inserted(){
	return m_inserted;
}

void GridTable::remove(uint64_t a){

}

GridData*GridTable::getElementInBin(int bin,int element){
	#ifdef ASSERT
	assert(bin<getNumberOfBins());
	assert(element<getNumberOfElementsInBin(bin));
	#endif
	return m_gridData[bin]+element;
}

int GridTable::getNumberOfElementsInBin(int bin){
	#ifdef ASSERT
	assert(bin<getNumberOfBins());
	#endif
	return m_gridSizes[bin];
}

int GridTable::getNumberOfBins(){
	return m_gridSize;
}

MyAllocator*GridTable::getAllocator(){
	return m_gridAllocator.getAllocator();
}

void GridTable::freeze(){
	m_frozen=true;
}

void GridTable::unfreeze(){
	m_frozen=false;
}

bool GridTable::frozen(){
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
Vertex*GridTable::move(int bin,int item){
	if(m_frozen){
		return &(m_gridData[bin][item].m_value);
	}
	GridData tmp;
	#ifdef ASSERT
	assert(item<getNumberOfElementsInBin(bin));
	#endif
	tmp=m_gridData[bin][item];
	for(int i=item-1;i>=0;i--){
		m_gridData[bin][i+1]=m_gridData[bin][i];
	}
	m_gridData[bin][0]=tmp;
	return &(m_gridData[bin][0].m_value);
}

