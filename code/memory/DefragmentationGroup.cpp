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
/* TODO: use m_firstGapLength */
/* TODO: use m_largestGapStart and m_largestGapLength */
/* TODO fast skip in findGap
 * TODO: estimated genome length
 *
 * TODO: populate fast pointers in defragment
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
 * indicate that a chunk is full
 * Instead of checking all the bits of all uint64_t
 * only the uint64_t objects that are not equal to CHUNK_IS_FULL 
 * are investigated.
 * */
#define CHUNK_IS_FULL 18446744073709551615UL /* maximum value for uint64_t */

/**
 *  Return true if the DefragmentationGroup can allocate n elements 
 *  Time complexity: O(1)
 */
bool DefragmentationGroup::canAllocate(int n){
	/* we want fast allocation in the end...  
	if a contiguous segment is not available, we can't handle it... 
*/
	//int offset=findAtLeast(n);
	//return offset>=0;
	//return m_availableElements>=n;
	return (ELEMENTS_PER_GROUP-m_freeSliceStart)>=n;
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
 * If that fails, compact() is called.
 * This will defragment the fragmented slice and make the free slice
 * larger.
 * After the call to compact(), the free slice has at least n consecutive
 * AVAILABLE elements.
 *
 * Time complexity: O(n) where n <= 64
 */
SmallSmartPointer DefragmentationGroup::allocate(int n,int bytesPerElement){
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
	assert(n>0);
	assert(canAllocate(n));
	assert(n<=ELEMENTS_PER_GROUP);
	assert(n<=m_availableElements);
	assert(m_allocatedOffsets!=NULL);
	assert(m_allocatedSizes!=NULL);
	#endif

/** 
 * get an handle 
 * This is O(256) if an available handle is found in m_fastPointers
 * Otherwise, it is O(65536) -- but the worst case is actually very rare.
 * */
	SmallSmartPointer returnValue=getAvailableSmallSmartPointer();

	#ifdef LOW_LEVEL_ASSERT
	assert(returnValue<ELEMENTS_PER_GROUP);
	assert(m_allocatedSizes[returnValue]==0);
	#endif

	/** find an offset with n consecutive AVAILABLE elements */
	//int offset=findAtLeast(n);
	int offset=m_freeSliceStart;

	/** found no consecutive elements, must call compact() */
	if(offset<0){
		/** create space in the free slice. */
		compact(n,bytesPerElement);
		offset=m_freeSliceStart;
	}

	#ifdef LOW_LEVEL_ASSERT
	assert(offset>=0 && offset<ELEMENTS_PER_GROUP);
	#endif

	/** save meta-data for the SmallSmartPointer */
	m_allocatedOffsets[returnValue]=offset;
	m_allocatedSizes[returnValue]=n;

	#ifdef LOW_LEVEL_ASSERT
	assert(m_allocatedSizes[returnValue]==n);
	assert(m_allocatedOffsets[returnValue]==offset);
	#endif

	for(int i=0;i<n;i++){
		#ifdef LOW_LEVEL_ASSERT
		assert(offset+i<ELEMENTS_PER_GROUP);
		if(getBit(offset+i)!=AVAILABLE){
			cout<<offset+i<<" is not available. offset: "<<offset<<" n: "<<n<<endl;
			print();
		}
		assert(getBit(offset+i)==AVAILABLE);
		#endif

		setBit(offset+i,UTILISED);

		#ifdef LOW_LEVEL_ASSERT
		assert(getBit(offset+i)==UTILISED);
		#endif
	}

	if(offset==m_freeSliceStart){
		m_freeSliceStart+=n;
		if(m_firstGapStart==offset){
			//cout<<__func__<<" m_firstGapStart "<<m_firstGapStart<<" -> "<<m_firstGapStart+n<<endl;
			m_firstGapStart+=n;
		}
	}

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
	assert(m_availableElements>=0);
	assert(m_availableElements<=ELEMENTS_PER_GROUP);
	#endif

	//int old=m_firstGapStart;

	while(m_firstGapStart<ELEMENTS_PER_GROUP && getBit(m_firstGapStart)==1)
		m_firstGapStart++;

	//cout<<__func__<<" m_firstGapStart "<<old<<" -> "<<m_firstGapStart+n<<endl;

	#ifdef ASSERT
	if(!(m_firstGapStart<=m_freeSliceStart)){
		cout<<"m_firstGapStart: "<<m_firstGapStart<<" m_freeSliceStart: "<<m_freeSliceStart<<" allocated "<<n<<" at offset "<<offset<<endl;
		print();
	}
	assert(m_firstGapStart<=m_freeSliceStart);
	#endif

	//cout<<"allocate "<<offset<<" "<<n<<endl;
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
	for(int i=m_freeSliceStart;i<ELEMENTS_PER_GROUP;i++){
		if(getBit(i)!=AVAILABLE){
			cout<<"Error, offset "<<i<<" is not AVAILABLE"<<endl;
			print();
		}
		assert(getBit(i)==AVAILABLE);
	}
	#endif

	#ifdef LOW_LEVEL_ASSERT
	assert(a<ELEMENTS_PER_GROUP);
	if(m_allocatedSizes[a]==0)
		cout<<__func__<<" SmallSmartPointer "<<(int)a<<" has size 0."<<endl;
	assert(m_allocatedSizes[a]>0);
	#endif

	int allocatedSize=m_allocatedSizes[a];
	int offset=m_allocatedOffsets[a];

	//cout<<"deallocate "<<offset<<" "<<allocatedSize<<endl;

	/** set the size to 0 for this SmallSmartPointer */
	m_allocatedSizes[a]=0;

	/** update the bitmap */
	for(int i=0;i<allocatedSize;i++){
		#ifdef LOW_LEVEL_ASSERT
		assert(offset+i<ELEMENTS_PER_GROUP);
		assert(getBit(offset+i)==UTILISED);
		#endif

		setBit(offset+i,AVAILABLE);

		#ifdef LOW_LEVEL_ASSERT
		assert(getBit(offset+i)==AVAILABLE);
		#endif
	}

	if(offset+allocatedSize==m_freeSliceStart){
		m_freeSliceStart=offset;
	}

	if(offset<m_firstGapStart){
		//cout<<__func__<<" m_firstGapStart "<<m_firstGapStart<<" -> "<<offset<<endl;
		m_firstGapStart=offset;
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
	assert(m_firstGapStart<=m_freeSliceStart);
	#endif

	//closeGap(offset,allocatedSize,bytesPerElement);
}

/**
 * 	Move all elements starting at newOffset+allocationLength up to m_freeSliceStart
 * 	by allocationLength positions on the left
 */
void DefragmentationGroup::closeGap(int offset,int allocationLength,int bytesPerElement){
	//cout<<"close gap "<<offset<<" "<<allocationLength<<endl;
	#ifdef ASSERT
	if(!(offset>=m_firstGapStart))
		cout<<"close gap at offset "<<offset<<" but m_firstGapStart is "<<m_firstGapStart<<endl;
	assert(offset>=m_firstGapStart);
	if(!(m_firstGapStart<=m_freeSliceStart)){
		cout<<"m_firstGapStart "<<m_firstGapStart<<" m_freeSliceStart "<<m_freeSliceStart<<" closeGap at offset "<<offset<<" with length "<<allocationLength<<endl;
		print();
	}

	assert(m_firstGapStart<=m_freeSliceStart);
	#endif

	#ifdef LOW_LEVEL_ASSERT
	for(int i=m_freeSliceStart;i<ELEMENTS_PER_GROUP;i++){
		if(getBit(i)!=AVAILABLE){
			cout<<"Error, offset "<<i<<" is not AVAILABLE"<<endl;
			print();
		}
		assert(getBit(i)==AVAILABLE);
	}
	#endif

/* verify the gap */
	#ifdef LOW_LEVEL_ASSERT
	for(int i=0;i<allocationLength;i++){
		assert(offset+i<ELEMENTS_PER_GROUP);
		assert(getBit(offset+i)==AVAILABLE);
	}
	#endif

/**
 *
 * 	Before:
 *	                   YXAHXAaxpp
 * 	|-----------|------|---------|
 * 		offset
 * 			offset+n
 * 				    last
 *
 * 	elements to move = (last-offset-n)
 *
 * 	After:
 *
 *	            YXAHXAaxpp
 * 	|-----------|------|--|
 *
 * 			      last
 *
 *
 *	update the bitmap
 */
	int elementsToMove=m_freeSliceStart-offset-allocationLength;
	for(int i=0;i<elementsToMove;i++){
		#ifdef LOW_LEVEL_ASSERT
		assert(offset+allocationLength+i<ELEMENTS_PER_GROUP);
		#endif
		setBit(offset+i,getBit(offset+allocationLength+i));
	}

/*
 * 	Move all elements starting at newOffset+allocationLength up to m_freeSliceStart
 * 	by allocationLength positions on the left
 */
	int lowerBound=offset+allocationLength;

	/* using int here to avoid overflow 
 * 	update m_allocatedOffsets
 *	O(allocationLength)
 * 	*/
	for(int smartPointer=0;smartPointer<ELEMENTS_PER_GROUP;smartPointer++){
		#ifdef LOW_LEVEL_ASSERT
		assert(smartPointer<ELEMENTS_PER_GROUP);
		#endif

		/* move the smart pointer */
		if(m_allocatedSizes[smartPointer]!=0 
		&& m_allocatedOffsets[smartPointer]>=lowerBound){
			m_allocatedOffsets[smartPointer]-=allocationLength;

			#ifdef LOW_LEVEL_ASSERT
			assert(m_allocatedOffsets[smartPointer]>=0);
			#endif
		}
	}

	/** 
 * move the bytes 
 * */
	uint8_t*destination=m_block+offset*bytesPerElement;
	uint8_t*source=m_block+(offset+allocationLength)*bytesPerElement;
	int bytes=elementsToMove*bytesPerElement;
	for(int i=0;i<bytes;i++)
		destination[i]=source[i];

/** update m_freeSliceStart 
 * 	*/

	m_freeSliceStart-=allocationLength;
	

	/** update the bitmap to make available the things at the end */
	for(int i=0;i<allocationLength;i++)
		setBit(m_freeSliceStart+i,AVAILABLE);

/*
	if(offset==m_firstGapStart){
		cout<<"gap is closed, need to find m_firstGapStart"<<endl;
		print();
	}
*/

	if(offset==m_firstGapStart){
		//int old=m_firstGapStart;
		/* here we have to be careful because the gap will not always be filled with occupied
 * 		buckets */
		while(m_firstGapStart<ELEMENTS_PER_GROUP && getBit(m_firstGapStart)==1)
			m_firstGapStart++;

		//cout<<__func__<<" m_firstGapStart "<<old<<" -> "<<m_firstGapStart<<" closed gap at offset "<<offset<<" with length "<<allocationLength<<endl;
	}
	
	#ifdef ASSERT
	if(!(m_firstGapStart<=m_freeSliceStart)){
		cout<<"m_firstGapStart "<<m_firstGapStart<<" m_freeSliceStart "<<m_freeSliceStart<<" closeGap at offset "<<offset<<" with length "<<allocationLength<<endl;
		print();
	}

	assert(m_firstGapStart<=m_freeSliceStart);
	#endif

	#ifdef LOW_LEVEL_ASSERT
	for(int i=m_freeSliceStart;i<ELEMENTS_PER_GROUP;i++){
		assert(i<ELEMENTS_PER_GROUP);
		assert(getBit(i)==AVAILABLE);
	}
	#endif
}

/**
 * Kick-start a DefragmentationGroup.
 */
void DefragmentationGroup::constructor(int bytesPerElement,bool show){
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
	m_firstGapStart=0;
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

int DefragmentationGroup::findAtLeast(int n){
	int chunks=ELEMENTS_PER_GROUP/64;
	for(int chunk=m_firstGapStart/64;chunk<chunks;chunk++){
		/** the chunk is not full, we may find something. */
		if(m_bitmap[chunk]!=CHUNK_IS_FULL){
			/** if the chunk is not full, then there are so gaps */
			for(int bitInChunk=0;bitInChunk<64;bitInChunk++){
				for(int offsetFromBit=0;offsetFromBit<n;offsetFromBit++){
					int offset=chunk*64+bitInChunk+offsetFromBit;
					if(offset==ELEMENTS_PER_GROUP)
						break;
					/* TODO: skip already seen AVAILABLE bits */

					#ifdef LOW_LEVEL_ASSERT
					if(offset>=ELEMENTS_PER_GROUP)
						cout<<"Offset= "<<offset<<" chunk: "<<chunk<<" bitInChunk: "<<bitInChunk<<" offsetFromBit: "<<offsetFromBit<<endl;
					assert(offset<ELEMENTS_PER_GROUP);
					#endif

					if(getBit(offset)==UTILISED){
						break;
					}
					/* so far, we have offsetFromBit+1 consecutive AVAILABLE elements */
					if(offsetFromBit+1==n){
						return chunk*64+bitInChunk;
					}
				}
			}
		}
	}
	/* found nothing */
	return -1;
}

void DefragmentationGroup::findGap(int*gapOffset,int*gapLength,int n){
	int chunks=ELEMENTS_PER_GROUP/64;
	(*gapOffset)=-1;
	(*gapLength)=0;
	for(int chunk=m_firstGapStart/64 ;chunk<chunks;chunk++){
		/** the chunk is not full, we may find something. */
		if(m_bitmap[chunk]!=CHUNK_IS_FULL){	
			/**  chunk is no full, find the gaps. */
			for(int bitInChunk=0;bitInChunk<64;bitInChunk++){
				/* the largest gap we can find is of length n-1  */
				for(int offsetFromBit=0;offsetFromBit<n;offsetFromBit++){
					int offset=chunk*64+bitInChunk+offsetFromBit;

					if(offset==ELEMENTS_PER_GROUP)
						break;

					#ifdef LOW_LEVEL_ASSERT
					if(offset>=ELEMENTS_PER_GROUP)
						cout<<"Offset= "<<offset<<endl;
					assert(offset<ELEMENTS_PER_GROUP);
					#endif

					/* TODO: skip already seen AVAILABLE bits */
					if(getBit(offset)==UTILISED){
						if(offsetFromBit>0){
							(*gapOffset)=chunk*64+bitInChunk;
							(*gapLength)=offsetFromBit;
							/* not taking the largest. taking the first. */
							return; 
						}
						break;
					}
				}
			}
		}
	}
	#ifdef LOW_LEVEL_ASSERT
	assert(false);
	#endif
}

void DefragmentationGroup::compact(int n,int bytesPerElement){
	#ifdef LOW_LEVEL_ASSERT
	assert(m_availableElements>=n);
	#endif

	int gapOffset=0;
	int gapLength=0;

	/** check the number of elements available in the free slice */
	while(!((ELEMENTS_PER_GROUP-m_freeSliceStart)>=n)){

		/** find the largest gap -- its size is at most n-1 */
		findGap(&gapOffset,&gapLength,n);

		#ifdef ASSERT
		if(!(gapOffset>=m_firstGapStart)){
			cout<<"gapOffset "<<gapOffset<<" m_firstGapStart "<<m_firstGapStart<<endl;
			print();
		}
		assert(gapOffset>=m_firstGapStart);
		#endif

		/** check that the gap we found is really a gap */
		#ifdef LOW_LEVEL_ASSERT
		if(gapLength<=0){
			cout<<"Gap Length is: "<<gapLength<<" needs "<<n<<" availableElements= "<<m_availableElements<<endl;
		}
		assert(gapLength>0);
		for(int i=0;i<gapLength;i++){
			assert(gapOffset+i<ELEMENTS_PER_GROUP);
			assert(getBit(gapOffset+i)==AVAILABLE);
		}
		#endif
		
		/** close this gap by moving stuff in it. */
		closeGap(gapOffset,gapLength,bytesPerElement);
		
		/** assert that all elements in the free slice are AVAILABLE */
		#ifdef LOW_LEVEL_ASSERT
		int i=m_freeSliceStart;
		while(i<ELEMENTS_PER_GROUP){
			assert(i<ELEMENTS_PER_GROUP);
			assert(getBit(i)==AVAILABLE);
			i++;
		}
		#endif
	}

	/** make sure that the free slice is large enough for n elements */
	#ifdef LOW_LEVEL_ASSERT
	assert((ELEMENTS_PER_GROUP-m_freeSliceStart)>=n);

	/** make sure that the free slice contains only AVAILABLE elements */
	for(int i=0;i<n;i++){
		assert(m_freeSliceStart+i<ELEMENTS_PER_GROUP);
		if(getBit(m_freeSliceStart+i)!=AVAILABLE){
			cout<<"m_freeSliceStart= "<<m_freeSliceStart<<" i: "<<i<<" error is UTILISED"<<endl;
			print();
		}
		assert(getBit(m_freeSliceStart+i)==AVAILABLE);
	}
	#endif

	#ifdef ASSERT
	assert(m_firstGapStart<=m_freeSliceStart);
	#endif
}

void DefragmentationGroup::print(){
	int step=1024;
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

bool DefragmentationGroup::defragment(int bytesPerElement){
	if(m_availableElements==0)
		return false;

	int elementsInFreeSlice=ELEMENTS_PER_GROUP-m_freeSliceStart;
	if(m_availableElements==elementsInFreeSlice)
		return false;

	//double ratioInFreeSlice=elementsInFreeSlice/(0.0+m_availableElements);
	
	int requested=elementsInFreeSlice+1;
	compact(requested,bytesPerElement);
	return true;

	/* defragment only if this ratio is < 50% */
/*
	if(ratioInFreeSlice<0.3 && m_availableElements>16){
		int result=elementsInFreeSlice+16;
		if(m_availableElements<result)
			result=m_availableElements;
		//cout<<"Defragmenting, available: "<<m_availableElements<<" in free slice: "<<elementsInFreeSlice<<" Target: "<<result<<endl;
		compact(result,bytesPerElement);
		return true;
	}
	return false;
*/
}
