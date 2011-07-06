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

/* TODO: replace m_allocatedSizes with a bitmap */
/* TODO:  replace calls to Malloc by a single call to Malloc */
 /* TODO: estimated genome length
 * */

/* call defragment somewhere else */

/* run low-level assertions, pretty slow but that helped for the development. */
/* #define LOW_LEVEL_ASSERT */

#include <memory/DefragmentationGroup.h>
#include <memory/allocator.h>
#include <stdlib.h>
#include <string.h>
#include <memory/malloc_types.h>
#include <iostream>
using namespace std;

#ifdef  ASSERT
#include <assert.h>
#endif

/** bit encoding  in the bit array */
#define AVAILABLE 0
#define UTILISED 1

/**
 *  Return true if the DefragmentationGroup can allocate n elements 
 *  Time complexity: O(1)
 */
bool DefragmentationGroup::canAllocate(int n){
	/* we want fast allocation in the end...  
	if a contiguous segment is not available, we can't handle it... 
*/
	//return offset>=0;
	
	#ifdef ASSERT
	assert((ELEMENTS_PER_GROUP-m_freeSliceStart)<=m_availableElements);
	#endif

	return m_availableElements>=n;
}

/**
 *    fragmented slice             free slice
 * |---------------------------|---------------|
 *
 *                             ^               ^
 *                             |               |
 * 0                           |            ELEMENTS_PER_GROUP-1
 *                             |
 *                     m_freeSliceStart
 *
 *  fragmented slice:  
 *
 *      part of m_block that will undergo fragmentation and defragmentation
 *
 *  free slice:
 *      part of m_block that has no fragmentation
 *      free slice has (ELEMENTS_PER_GROUP-m_freeSliceStart) consecutive AVAILABLE elements
 *      starting at m_freeSliceStart
 *
 *  m_freeSliceStart:
 *      indicates where the free slice begins
 *      
 *      Allocation algorithm:
 *
 * First, we try to find n consecutive AVAILABLE elements in the
 * fragmented slice.
 * This is O(ELEMENTS_PER_GROUP), and is a read-only operation, thus pretty fast.
 *
 * 64-element chunks that are filled completely are just not investigated.
 * 
 * This will defragment the fragmented slice and make the free slice
 * larger.
 * AVAILABLE elements.
 *
 * Time complexity: O(n) where n <= 64
 */
SmallSmartPointer DefragmentationGroup::allocate(int n,int bytesPerElement,uint16_t*content){
	if(n>(ELEMENTS_PER_GROUP-m_freeSliceStart))
		defragment(bytesPerElement,content,true);

	#ifdef ASSERT
	assert(n>0);
	assert(canAllocate(n));
	assert(n<=ELEMENTS_PER_GROUP);
	assert(n<=m_availableElements);
	assert(n<=(ELEMENTS_PER_GROUP-m_freeSliceStart));
	assert(m_allocatedOffsets!=NULL);
	assert(m_allocatedSizes!=NULL);
	#endif

/** 
 * get an handle 
 * This is O(256) if an available handle is found in m_fastPointers
 * Otherwise, it is O(65536) -- but the worst case is actually very rare.
 * */
	SmallSmartPointer returnValue=getAvailableSmallSmartPointer();

	#ifdef ASSERT
	assert(returnValue<ELEMENTS_PER_GROUP);
	assert(m_allocatedSizes[returnValue]==0);
	#endif

	/** find an offset with n consecutive AVAILABLE elements */
	int offset=m_freeSliceStart;

	#ifdef ASSERT
	assert(offset>=0 && offset<ELEMENTS_PER_GROUP);
	#endif

	/** save meta-data for the SmallSmartPointer */
	m_allocatedOffsets[returnValue]=offset;
	m_allocatedSizes[returnValue]=n;

	#ifdef ASSERT
	assert(m_allocatedSizes[returnValue]==n);
	assert(m_allocatedOffsets[returnValue]==offset);
	#endif

	for(int i=0;i<n;i++){
		#ifdef ASSERT
		assert(offset+i<ELEMENTS_PER_GROUP);
		if(getBit(offset+i)!=AVAILABLE){
			cout<<offset+i<<" is not available. offset: "<<offset<<" n: "<<n<<endl;
			print();
		}
		assert(getBit(offset+i)==AVAILABLE);
		#endif

		setBit(offset+i,UTILISED);

		#ifdef ASSERT
		assert(getBit(offset+i)==UTILISED);
		#endif
	}

	m_freeSliceStart+=n;

	#ifdef LOW_LEVEL_ASSERT
	for(int i=m_freeSliceStart;i<ELEMENTS_PER_GROUP;i++){
		if(getBit(i)!=AVAILABLE){
			cout<<"Error, offset "<<i<<" is not AVAILABLE, allocated "<<n<<" at offset "<<offset<<"  m_freeSliceStart="<<m_freeSliceStart<<endl;
			print();
		}
		assert(getBit(i)==AVAILABLE);
	}
	#endif

	m_availableElements-=n;
	
	#ifdef ASSERT
	if(m_availableElements<0)
		cout<<"m_availableElements "<<m_availableElements<<endl;
	assert(m_availableElements>=0);
	assert(m_availableElements<=ELEMENTS_PER_GROUP);
	#endif

	return returnValue;
}

/**
 * Usually very fast, getAvailableSmallSmartPointer returns a SmallSmartPointer
 * that is available. To do so, the list m_fastPointers is searched.
 * This list contains FAST_POINTERS entries (default is 256).
 * If there is a hit, it is returned to the caller.
 * 
 * Otherwise, all the possible SmallSmartPointer handles are probed
 * (there are ELEMENTS_PER_GROUP (default is 65536) of them)
 * and those available are appended to m_fastPointers.
 * 
 * Finally, the SmallSmartPointer that was the last to be recorded to m_fastPointers
 * is returned.
 */
SmallSmartPointer DefragmentationGroup::getAvailableSmallSmartPointer(){
	for(int i=0;i<FAST_POINTERS;i++)
		if(m_allocatedSizes[m_fastPointers[i]]==0)
			return m_fastPointers[i];

	/**
 * 	Otherwise, populate m_fastPointers
 *  find an alternative available handle */
	/** O(65536) */

	SmallSmartPointer returnValue=0;

	int fast=0;
	while(m_allocatedSizes[m_fastPointers[fast]]==0 && fast<FAST_POINTERS)
		fast++;

	int i=0;

	for(i=0;i<ELEMENTS_PER_GROUP;i++){
		if(m_allocatedSizes[i]==0){
			m_fastPointers[fast++]=i;
			/* always update returnValue to return the last one observed */
			returnValue=i;
		}
		while(m_allocatedSizes[m_fastPointers[fast]]==0 && fast<FAST_POINTERS)
			fast++;
		/** we populated m_fastPointers */
		if(fast==FAST_POINTERS)
			break;
	}

	return returnValue;
}

/**
 * deallocate a SmallSmartPointer
 * defragment
 * done.
 *
 * Time complexity: To be determined.
 */
void DefragmentationGroup::deallocate(SmallSmartPointer a,int bytesPerElement){

	#ifdef LOW_LEVEL_ASSERT
	assert((ELEMENTS_PER_GROUP-m_freeSliceStart)<=m_availableElements);
	for(int i=m_freeSliceStart;i<ELEMENTS_PER_GROUP;i++){
		if(getBit(i)!=AVAILABLE){
			cout<<"Error, offset "<<i<<" is not AVAILABLE"<<endl;
			print();
		}
		assert(getBit(i)==AVAILABLE);
	}
	#endif

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

	/** update the bitmap */
	for(int i=0;i<allocatedSize;i++){
		#ifdef ASSERT
		assert(offset+i<ELEMENTS_PER_GROUP);
		if(getBit(offset+i)!=UTILISED)
			cout<<"offset "<<offset<<" i "<<i<<" not utilised"<<endl;
		assert(getBit(offset+i)==UTILISED);
		#endif

		setBit(offset+i,AVAILABLE);

		#ifdef ASSERT
		assert(getBit(offset+i)==AVAILABLE);
		#endif
	}

	if(offset+allocatedSize==m_freeSliceStart){
		m_freeSliceStart=offset;
	}

	/* move m_freeSliceStart */
	while((m_freeSliceStart-1)>=0 && getBit(m_freeSliceStart-1)==AVAILABLE)
		m_freeSliceStart--;

	#ifdef LOW_LEVEL_ASSERT
	for(int i=m_freeSliceStart;i<ELEMENTS_PER_GROUP;i++){
		if(getBit(i)!=AVAILABLE){
			cout<<"Error, offset "<<i<<" is not AVAILABLE"<<endl;
			print();
		}
		assert(getBit(i)==AVAILABLE);
	}
	#endif

	#ifdef LOW_LEVEL_ASSERT
	assert(m_allocatedSizes[a]==0);
	#endif
	
	m_availableElements+=allocatedSize;

	#ifdef ASSERT
	assert(m_availableElements<=ELEMENTS_PER_GROUP);
	assert(m_availableElements>=0);
	if(!((ELEMENTS_PER_GROUP-m_freeSliceStart)<=m_availableElements))
		cout<<"elements in freeSlice: "<<ELEMENTS_PER_GROUP-m_freeSliceStart<<" available "<<m_availableElements<<" m_freeSliceStart "<<m_freeSliceStart<<endl;
	assert((ELEMENTS_PER_GROUP-m_freeSliceStart)<=m_availableElements);
	#endif
}

/**
 * Kick-start a DefragmentationGroup.
 */
void DefragmentationGroup::constructor(int bytesPerElement,bool show){
	m_offlineDefrags=0;
	m_onlineDefrags=0;
	#ifdef ASSERT
	assert(m_block==NULL);
	#endif

	m_availableElements=ELEMENTS_PER_GROUP;
	
	#ifdef ASSERT
	assert(m_availableElements<=ELEMENTS_PER_GROUP);
	assert(m_availableElements>=0);
	#endif
	
	for(int i=0;i<FAST_POINTERS;i++)
		m_fastPointers[i]=i;

	m_freeSliceStart=0;
	/** allocate the memory */
	m_block=(uint8_t*)__Malloc(ELEMENTS_PER_GROUP*bytesPerElement,RAY_MALLOC_TYPE_DEFRAG_GROUP,show);

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

	#ifdef ASSERT
	assert((ELEMENTS_PER_GROUP-m_freeSliceStart)<=m_availableElements);
	#endif
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
	#ifdef LOW_LEVEL_ASSERT
	if(!(bit>=0 && bit<ELEMENTS_PER_GROUP))
		cout<<"Invalid bit: "<<bit<<endl;
	assert(bit>=0 && bit<ELEMENTS_PER_GROUP);
	#endif

	int bitChunk=bit/64;
	int bitPosition=bit%64;
	uint64_t filter=m_bitmap[bitChunk];
	filter<<=(63-bitPosition);
	filter>>=63;
	int value=filter;
	
	#ifdef LOW_LEVEL_ASSERT
	assert(value==AVAILABLE||value==UTILISED);
	#endif

	return value;
}

/**
 * Set bit bit as value
 */
void DefragmentationGroup::setBit(int bit,int value){
	#ifdef LOW_LEVEL_ASSERT
	assert(bit>=0 && bit<ELEMENTS_PER_GROUP);
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

	#ifdef LOW_LEVEL_ASSERT
	assert(getBit(bit)==AVAILABLE||getBit(bit)==UTILISED);
	if(getBit(bit)!=value)
		cout<<"Expected: "<<value<<" Actual: "<<getBit(bit)<<endl;

	assert(getBit(bit)==value);
	#endif
}

/**
 * Resolvee a SmallSmartPointer
 */
void*DefragmentationGroup::getPointer(SmallSmartPointer a,int bytesPerElement){
	#ifdef ASSERT
	if(m_allocatedSizes[a]==0)
		cout<<"this= "<<this<<" can not getPointer on SmallSmartPointer "<<(int)a<<" because it is not allocated."<<endl;
	assert(m_allocatedSizes[a]!=0);
	#endif
	int offset=m_allocatedOffsets[a];
	void*pointer=m_block+offset*bytesPerElement;
	return pointer;
}

int DefragmentationGroup::getAllocationSize(SmallSmartPointer a){
	return m_allocatedSizes[a];
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
	#ifdef ASSERT
	assert(m_block!=NULL);
	assert(m_availableElements<=ELEMENTS_PER_GROUP);
	assert(m_availableElements>=0);
	#endif
	return m_availableElements;
}

void DefragmentationGroup::print(){
	int step=1024;
	int bitTo1=0;

	for(int i=0;i<ELEMENTS_PER_GROUP;i++){
		bitTo1+=getBit(i);
	}

	cout<<"bit set to 1: "<<bitTo1<<"/"<<ELEMENTS_PER_GROUP<<endl;
	cout<<"entries"<<endl;
	for(int i=0;i<ELEMENTS_PER_GROUP;i++){
		if(m_allocatedSizes[i]!=0)
			cout<<"SmallSmartPointer "<<i<<" offset "<<m_allocatedOffsets[i]<<" size "<<(int)m_allocatedSizes[i]<<endl;
	}
	cout<<"bitmap"<<endl;
	for(int i=0;i<ELEMENTS_PER_GROUP;i++){
		if(i%step==0){
			cout<<endl<<i<<"-"<<i+step-1<<" ";
		}
		cout<<getBit(i);
	}
	cout<<endl;
}

int DefragmentationGroup::getFreeSliceStart(){
	return m_freeSliceStart;
}

/*
 * TODO remove 
 */
bool DefragmentationGroup::defragment(int bytesPerElement,uint16_t*content,bool online){

	/* update the look-up table */
	for(int i=0;i<ELEMENTS_PER_GROUP;i++){
		/* otherwise we don't want to register it because it may overwrite someone else */
		if(m_allocatedSizes[i]!=0)
			content[m_allocatedOffsets[i]]=i;
	}

	#ifdef ASSERT
	assert((ELEMENTS_PER_GROUP-m_freeSliceStart)<=m_availableElements);
	#endif

	/* nothing is available */
	if(m_availableElements==0)
		return false;

	/* everything is already in the free slice */
	int elementsInFreeSlice=ELEMENTS_PER_GROUP-m_freeSliceStart;
	if(m_availableElements==elementsInFreeSlice)
		return false;

	if(online)
		m_onlineDefrags++;
	else
		m_offlineDefrags++;

	int destination=0;
	int source=0;

	while(getBit(destination)==1 && destination<ELEMENTS_PER_GROUP)
		destination++;
	source=destination+1;

	while(getBit(source)==0 && source<ELEMENTS_PER_GROUP)
		source++;

	while(source<ELEMENTS_PER_GROUP){
		/* if source is empty, we are done. */
		if(getBit(source)==0)
			break;

		/* at this point, source points to a offset where a SmallSmartPointer starts */
		/* and destination is an empty space. */
		#ifdef ASSERT
		if(!(source>destination)){
			cout<<"destination "<<destination<<" source "<<source<<endl;
			print();
		}
		assert(source>destination);
		#endif

		#ifdef ASSERT
		assert(getBit(destination)==0);
		assert(getBit(source)==1);
		#endif

		/* here we copy source in destination */
		/* copy the data */
		SmallSmartPointer smartPointer=content[source];

		#ifdef ASSERT
		assert(getBit(source-1)==0);
		if(!(m_allocatedSizes[smartPointer]!=0)){
			cout<<"SmallSmartPointer "<<smartPointer<<" must move, but is has size 0 and bit is 1"<<endl;
			print();
		}
		assert(m_allocatedSizes[smartPointer]!=0);
		#endif
		
		int n=m_allocatedSizes[smartPointer];
		for(int i=0;i<n;i++){
			for(int byte=0;byte<bytesPerElement;byte++){
				m_block[(destination+i)*bytesPerElement+byte]=m_block[(source+i)*bytesPerElement+byte];
			
			}
		}

		for(int i=destination;i<destination+n;i++)
			setBit(i,1);

		for(int i=destination+n;i<source+n;i++)
			setBit(i,0);

		m_allocatedOffsets[smartPointer]=destination;

		/* move destination after the new location of smartPointer */
		destination+=n;
		source+=n;

		/* skip occupied elements */
		while(getBit(destination)==1 && destination<ELEMENTS_PER_GROUP)
			destination++;
		/* now destination is at a free block */

		/* skip available elements */
		while(getBit(source)==0 && source<ELEMENTS_PER_GROUP)
			source++;
	}
	
	m_freeSliceStart=destination;

	#ifdef ASSERT
	assert((ELEMENTS_PER_GROUP-m_freeSliceStart)<=m_availableElements);
	#endif

	#ifdef LOW_LEVEL_ASSERT
	for(int i=m_freeSliceStart;i<ELEMENTS_PER_GROUP;i++)
		assert(getBit(i)==AVAILABLE);

	for(int i=0;i<m_freeSliceStart;i++)
		assert(getBit(i)==UTILISED);

	int bitTo1=0;
	int elements=0;
	for(int i=0;i<ELEMENTS_PER_GROUP;i++){
		bitTo1+=getBit(i);
		if(m_allocatedSizes[i]!=0){
			if(!(m_allocatedOffsets[i]<m_freeSliceStart)){
				cout<<"SmallSmartPointer "<<i<<" has offset is "<<m_allocatedOffsets[i]<<" but m_freeSliceStart is "<<m_freeSliceStart<<" size: "<<(int)m_allocatedSizes[i]<<endl;
				print();
			}
			assert(m_allocatedOffsets[i]<m_freeSliceStart);
			elements+=m_allocatedSizes[i];
			for(int j=0;j<m_allocatedSizes[i];j++){
				if(!(getBit(m_allocatedOffsets[i]+j)==UTILISED)){
					cout<<"Error, SmallSmartPointer "<<i<<" offset "<<(int)m_allocatedOffsets[i]<<" size "<<(int)m_allocatedSizes[i]<<" is not in the bitmap at j="<<j<<endl;
					print();
				}
				assert(getBit(m_allocatedOffsets[i]+j)==UTILISED);
			}
		}
	}
	assert(bitTo1==elements);
	#endif

	return true;
}
