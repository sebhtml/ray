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

#include <memory/DefragmentationGroup.h>
#include <memory/allocator.h>
#include <stdlib.h>
#include <memory/malloc_types.h>
#include <iostream>
using namespace std;

#ifdef ASSERT
#include <assert.h>
#endif

/** bit encoding  in the bit array */
#define AVAILABLE 0
#define OCCUPIED 1

/**
 * m_availableElements must be >= n 
 */
bool DefragmentationGroup::canAllocate(int n){
	int available=ELEMENTS_PER_GROUP-m_lastFreePosition;
	return n<=available;
}

/**
 * This is easy, just save the current head,
 * forward the head n times
 * and return the head 
 * Also, the bitmap must be updated.
 */
SmallSmartPointer DefragmentationGroup::allocate(int n){
	#ifdef ASSERT
	assert(n>0);
	assert(canAllocate(n));
	#endif

	/** get the HEAD */
	SmallSmartPointer returnValue=m_lastFreePosition;

	#ifdef ASSERT
	assert(returnValue<ELEMENTS_PER_GROUP);
	assert(m_allocatedOffsets!=NULL);
	assert(m_allocatedSizes!=NULL);
	#endif

	/** save meta-data for the SmallSmartPointer */
	m_allocatedSizes[returnValue]=n;
	m_allocatedOffsets[returnValue]=m_lastFreePosition;

	#ifdef ASSERT
	assert(returnValue+n-1<ELEMENTS_PER_GROUP);
	#endif

	/** mark it as used in the bitmap */
	for(int i=0;i<n;i++)
		setBit(returnValue+i,OCCUPIED);

	/** forward the head  */
	m_lastFreePosition+=n;

	/** lower the number of available elements */
	m_availableElements-=n;
	
	#ifdef ASSERT
	assert(m_allocatedSizes[returnValue]!=0);
	#endif
	
	return returnValue;
}

/**
 * free the bits in the bitmap.
 * defragment
 * done.
 */
void DefragmentationGroup::deallocate(SmallSmartPointer a){
	#ifdef ASSERT
	if(m_allocatedSizes[a]==0)
		cout<<"SmallSmartPointer has size 0."<<endl;
	assert(m_allocatedSizes[a]>0);
	#endif
	int allocatedSize=m_allocatedSizes[a];
	int offset=m_allocatedOffsets[a];
	int max=offset+allocatedSize;

	/** update the bit map */
	for(int i=offset;i<max;i++){
		#ifdef ASSERT
		assert(getBit(i)==OCCUPIED);
		#endif
		setBit(i,AVAILABLE);
	}

	/** set the size to 0 for this SmallSmartPointer */
	m_allocatedSizes[a]=0;
	m_allocatedOffsets[a]=0;
	m_availableElements+=allocatedSize;
	
	/** run the defragmenter at position offset with width allocatedSize */
	defragment(offset,allocatedSize);
}

/**
 * The defragment() call has mainly 2 routines:
 *
 *  1) Try to swap something into the gap
 *
 *  2) Move thing from right to left to close the gap
 *
 *  Finally, m_lastFreePosition is updated.
 *
 *  Also, all the SmallSmartPointer  must be updated  in
 *
 *   - m_allocatedOffsets
 *
 *  m_allocatedSizes does not change at all.
 */
void DefragmentationGroup::defragment(uint16_t offset,uint8_t allocationLength){

}

/**
 * Kick-start a DefragmentationGroup.
 */
void DefragmentationGroup::constructor(int period,bool show){
	m_lastFreePosition=0;
	m_availableElements=ELEMENTS_PER_GROUP;
	/** allocate the memory */
	m_block=(uint8_t*)__Malloc(ELEMENTS_PER_GROUP*period,RAY_MALLOC_TYPE_DEFRAG_GROUP,show);

	/** initialise sizes */
	m_allocatedSizes=(uint8_t*)__Malloc(ELEMENTS_PER_GROUP*sizeof(uint8_t),RAY_MALLOC_TYPE_DEFRAG_GROUP,show);
	m_allocatedOffsets=(uint16_t*)__Malloc(ELEMENTS_PER_GROUP*sizeof(uint16_t),RAY_MALLOC_TYPE_DEFRAG_GROUP,show);
	for(int i=0;i<ELEMENTS_PER_GROUP;i++){
		m_allocatedSizes[i]=0;
		m_allocatedOffsets[i]=0;
	}

	/** initialise the bitmap */
	m_bitmap=(uint64_t*)__Malloc(ELEMENTS_PER_GROUP/64*sizeof(uint64_t),RAY_MALLOC_TYPE_DEFRAG_GROUP,show);
	for(int i=0;i<ELEMENTS_PER_GROUP;i++)
		setBit(i,AVAILABLE);
}

/**
 * Free memory
 */
void DefragmentationGroup::destructor(bool show){
	__Free(m_block,RAY_MALLOC_TYPE_DEFRAG_GROUP,show);
	m_block=NULL;
	__Free(m_allocatedSizes,RAY_MALLOC_TYPE_DEFRAG_GROUP,show);
	m_allocatedSizes=NULL;
	__Free(m_bitmap,RAY_MALLOC_TYPE_DEFRAG_GROUP,show);
	m_bitmap=NULL;
	__Free(m_allocatedOffsets,RAY_MALLOC_TYPE_DEFRAG_GROUP,show);
	m_allocatedOffsets=NULL;
}

/*
 * Check if element bit is available or not 
 */
int DefragmentationGroup::getBit(int bit){
	#ifdef ASSERT
	assert(m_bitmap!=NULL);
	#endif

	int bitChunk=bit/64;
	int bitPosition=bit%64;
	uint64_t filter=m_bitmap[bitChunk];
	filter<<=(63-bitPosition);
	filter>>=63;
	int value=filter;
	
	#ifdef ASSERT
	assert(value==AVAILABLE||value==OCCUPIED);
	#endif

	return value;
}

/**
 * Set bit bit as value
 */
void DefragmentationGroup::setBit(int bit,int value){
	#ifdef ASSERT
	assert(m_bitmap!=NULL);
	assert(value==AVAILABLE||value==OCCUPIED);
	assert(getBit(bit)==AVAILABLE||getBit(bit)==OCCUPIED);
	#endif

	int bitChunk=bit/64;
	int bitPosition=bit%64;
	uint64_t filter=1;
	filter<<=bitPosition;
	if(value==1){
		m_bitmap[bitChunk]|=filter;
	}else if(value==0){
		filter=~filter;
		m_bitmap[bitChunk]&=filter;
	}

	#ifdef ASSERT
	assert(getBit(bit)==AVAILABLE||getBit(bit)==OCCUPIED);
	if(getBit(bit)!=value)
		cout<<"Expected: "<<value<<" Actual: "<<getBit(bit)<<endl;

	assert(getBit(bit)==value);
	#endif
}

/**
 * Resolvee a SmallSmartPointer
 */
void*DefragmentationGroup::getPointer(SmallSmartPointer a,int period){
	return m_block+m_allocatedOffsets[a]*period;
}

/**
 * Set pointers to NULL
 */
void DefragmentationGroup::setPointers(){
	m_block=NULL;
	m_allocatedOffsets=NULL;
	m_allocatedSizes=NULL;
	m_bitmap=NULL;
}

/**
 * Check if the DefragmentationGroup is active.
 */
bool DefragmentationGroup::isOnline(){
	return m_block!=NULL;
}

/**
 * Get the number of available elements
 */
int DefragmentationGroup::getAvailableElements(){
	return m_availableElements;
}

/**
 * Print the bitmap
 */
void DefragmentationGroup::print(){
	cout<<"bitmap, 0-1023:"<<endl;
	for(int i=0;i<ELEMENTS_PER_GROUP;i++)
		cout<<getBit(i);
	cout<<endl;
}
