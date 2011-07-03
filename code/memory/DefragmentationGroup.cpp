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
#include <string.h>
#include <memory/malloc_types.h>
#include <iostream>
using namespace std;

#ifdef ASSERT
#include <assert.h>
#endif

/** bit encoding  in the bit array */
#define AVAILABLE 0
#define UTILISED 1

/** indicate that a chunk is full*/
#define CHUNK_IS_FULL 18446744073709551615 /* maximum value for uint64_t */

/**
 *  Return true if the DefragmentationGroup can allocate n elements 
 */
bool DefragmentationGroup::canAllocate(int n){
	return (ELEMENTS_PER_GROUP-m_lastFreePosition)>=n;
}

/**
 * This is easy, just save the current head,
 * forward the head n times
 * and return the head 
 * Also, the bitmap must be updated.
 */
SmallSmartPointer DefragmentationGroup::allocate(int n,int period){
	#ifdef ASSERT
	assert(n>0);
	assert(canAllocate(n));
	assert(n<=ELEMENTS_PER_GROUP);
	assert(m_lastFreePosition+n-1<ELEMENTS_PER_GROUP);
	assert(m_allocatedOffsets!=NULL);
	assert(m_allocatedSizes!=NULL);
	#endif

	/** get an handle */
	SmallSmartPointer returnValue=getAvailableSmallSmartPointer();

	#ifdef ASSERT
	assert(returnValue<ELEMENTS_PER_GROUP);
	assert(m_allocatedSizes[returnValue]==0);
	#endif

	/** save meta-data for the SmallSmartPointer */
	m_allocatedSizes[returnValue]=n;
	m_allocatedOffsets[returnValue]=m_lastFreePosition;

	#ifdef ASSERT
	assert(m_allocatedSizes[returnValue]==n);
	assert(m_allocatedOffsets[returnValue]==m_lastFreePosition);
	#endif
	
	/** if we use the FreeSlice, advance the HEAD */
	/** forward the head  */
	m_lastFreePosition+=n;

	return returnValue;
}

/**
 * Usually very fast, getAvailableSmallSmartPointer returns a SmallSmartPointer
 * that is available. To do so, the list m_availablePointers is searched.
 * This list contains FAST_POINTERS entries (default is 256).
 * If there is a hit, it is returned to the caller.
 * 
 * Otherwise, all the possible SmallSmartPointer handles are probed
 * (there are ELEMENTS_PER_GROUP (default is 65536) of them)
 * and those available are appended to m_availablePointers.
 * 
 * Finally, the SmallSmartPointer that was the last to be recorded to m_availablePointers
 * is returned.
 */
SmallSmartPointer DefragmentationGroup::getAvailableSmallSmartPointer(){
	for(int i=0;i<FAST_POINTERS;i++)
		if(m_allocatedSizes[m_availablePointers[i]]==0)
			return m_availablePointers[i];

	/**
 * 	Otherwise, populate m_availablePointers
 *  find an alternative available handle */
	/** O(65536) */

	SmallSmartPointer returnValue=0;

	int fast=0;
	while(m_allocatedSizes[m_availablePointers[fast]]==0 && fast<FAST_POINTERS)
		fast++;

	int i=0;

	for(i=0;i<ELEMENTS_PER_GROUP;i++){
		if(m_allocatedSizes[i]==0){
			m_availablePointers[fast++]=i;
			/* always update returnValue to return the last one observed */
			returnValue=i;
		}
		while(m_allocatedSizes[m_availablePointers[fast]]==0 && fast<FAST_POINTERS)
			fast++;
		/** we populated m_availablePointers */
		if(fast==FAST_POINTERS)
			break;
	}

	return returnValue;
}

/**
 * deallocate a SmallSmartPointer
 * defragment
 * done.
 */
void DefragmentationGroup::deallocate(SmallSmartPointer a,int period){
	#ifdef ASSERT
	assert(a<ELEMENTS_PER_GROUP);
	if(m_allocatedSizes[a]==0)
		cout<<__func__<<" SmallSmartPointer "<<(int)a<<" has size 0."<<endl;
	assert(m_allocatedSizes[a]>0);
	#endif

	int allocatedSize=m_allocatedSizes[a];
	int offset=m_allocatedOffsets[a];

	/** set the size to 0 for this SmallSmartPointer */
	m_allocatedSizes[a]=0;

	/** defragment it now */
	int indicator=offset+allocatedSize;
	int lastUtilised=m_lastFreePosition-1;
	if(indicator<lastUtilised)
		moveElementsToCloseGap(offset,allocatedSize,period);

	//** add it as a fast pointer if necessary */
	int fast=0;
	while(fast<FAST_POINTERS){
		#ifdef ASSERT
		assert(fast<FAST_POINTERS);
		assert(m_availablePointers[fast]<ELEMENTS_PER_GROUP);
		#endif
	
		/** replace this fast pointer with the current that is now available */
		if(m_allocatedSizes[m_availablePointers[fast]]!=0){
			m_availablePointers[fast]=a;
			break;
		}
		fast++;
	}
}

/**
 * 	Move all elements starting at newOffset+allocationLength up to m_lastFreePosition
 * 	by allocationLength positions on the left
 */
void DefragmentationGroup::moveElementsToCloseGap(int offset,int allocationLength,int period){
/*
 * 	Move all elements starting at newOffset+allocationLength up to m_lastFreePosition
 * 	by allocationLength positions on the left
 */
	int lowerBound=offset+allocationLength;

	/* using int here to avoid overflow 
 * 	update m_allocatedOffsets
 *	O(allocationLength)
 * 	*/
	for(int smartPointer=0;smartPointer<ELEMENTS_PER_GROUP;smartPointer++){
		#ifdef ASSERT
		assert(smartPointer<ELEMENTS_PER_GROUP);
		#endif

		/* move the smart pointer */
		if(m_allocatedSizes[smartPointer]!=0 
		&& m_allocatedOffsets[smartPointer]>=lowerBound){
			m_allocatedOffsets[smartPointer]-=allocationLength;

			#ifdef ASSERT
			assert(m_allocatedOffsets[smartPointer]>=0);
			#endif
		}
	}

	/** 
 * move the bytes 
 * */
	uint8_t*destination=m_block+offset*period;
	uint8_t*source=m_block+(offset+allocationLength)*period;
	int bytes=(m_lastFreePosition-offset-allocationLength)*period;
	for(int i=0;i<bytes;i++)
		destination[i]=source[i];

/** update m_lastFreePosition 
 * 	*/

	m_lastFreePosition-=allocationLength;
}

/**
 * Kick-start a DefragmentationGroup.
 */
void DefragmentationGroup::constructor(int period,bool show){
	for(int i=0;i<FAST_POINTERS;i++)
		m_availablePointers[i]=i;

	m_lastFreePosition=0;
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
	int bitChunk=bit/64;
	int bitPosition=bit%64;
	uint64_t filter=m_bitmap[bitChunk];
	filter<<=(63-bitPosition);
	filter>>=63;
	int value=filter;
	
	#ifdef ASSERT
	assert(value==AVAILABLE||value==UTILISED);
	#endif

	return value;
}

/**
 * Set bit bit as value
 */
void DefragmentationGroup::setBit(int bit,int value){
	#ifdef ASSERT
	assert(m_bitmap!=NULL);
	assert(value==AVAILABLE||value==UTILISED);
	assert(getBit(bit)==AVAILABLE||getBit(bit)==UTILISED);
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
	assert(getBit(bit)==AVAILABLE||getBit(bit)==UTILISED);
	if(getBit(bit)!=value)
		cout<<"Expected: "<<value<<" Actual: "<<getBit(bit)<<endl;

	assert(getBit(bit)==value);
	#endif
}

/**
 * Resolvee a SmallSmartPointer
 */
void*DefragmentationGroup::getPointer(SmallSmartPointer a,int period){
	void*pointer=m_block+m_allocatedOffsets[a]*period;
	#ifdef ASSERT
	assert(m_allocatedSizes[a]!=0);
	#endif
	return pointer;
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
	return ELEMENTS_PER_GROUP-m_lastFreePosition;
}
