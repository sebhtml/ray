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

#include <memory/ChunkAllocatorWithDefragmentation.h>
#include <memory/malloc_types.h>
#include <memory/allocator.h>
#ifdef ASSERT
#include <assert.h>
#endif
#include <iostream>
using namespace std;

void ChunkAllocatorWithDefragmentation::defragment(){
}

/**
* print allocator information
*/
void ChunkAllocatorWithDefragmentation::print(){
	cout<<"DefragmentationLanes: "<<m_numberOfLanes<<endl;

	for(int i=0;i<m_numberOfLanes;i++){
		DefragmentationLane*lane=m_defragmentationLanes[i];
		#ifdef ASSERT
		assert(lane!=NULL);
		#endif
		cout<<"DefragmentationLane: "<<lane->getNumber()<<endl;
		for(int group=0;group<GROUPS_PER_LANE;group++){
			if(lane->getGroup(group)->isOnline()){
				int availableElements=lane->getGroup(group)->getAvailableElements();

				cout<<"  DefragmentationGroup: "<<group<<" usage: "<<(ELEMENTS_PER_GROUP-availableElements)<<"/"<<ELEMENTS_PER_GROUP<<" FreeSliceStart: "<<lane->getGroup(group)->getFreeSliceStart()<<endl;
			}
		}
	}
}

/** clear allocations */
void ChunkAllocatorWithDefragmentation::destructor(){
	/** destroy DefragmentationLanes */
	for(int i=0;i<m_numberOfLanes;i++){
		DefragmentationLane*lane=m_defragmentationLanes[i];
		for(int group=0;group<GROUPS_PER_LANE;group++){
			if(lane->getGroup(group)->isOnline())
				lane->getGroup(group)->destructor(m_show);
		}
		__Free(lane,RAY_MALLOC_TYPE_DEFRAG_LANE,m_show);
		m_defragmentationLanes[i]=NULL;
	}
}

/** constructor almost does nothing  */
void ChunkAllocatorWithDefragmentation::constructor(int bytesPerElement,bool show){
	m_content=(uint16_t*)__Malloc(ELEMENTS_PER_GROUP*sizeof(uint16_t),RAY_MALLOC_TYPE_DEFRAG_LANE,show);
	m_show=show;
	m_bytesPerElement=bytesPerElement;
	for(int i=0;i<NUMBER_OF_LANES;i++)
		m_defragmentationLanes[i]=NULL;
	m_fastLane=NULL;
	m_numberOfLanes=0;
}

/**
 * update:
 *  m_fastLane
 *  m_fastGroup
 *  m_fastLaneNumber
 */
void ChunkAllocatorWithDefragmentation::updateFastLane(int n){
	/** find a DefragmentationGroup in a DefragmentationLane that 
 * can accomodate the query */
	for(int i=0;i<m_numberOfLanes;i++){
		DefragmentationLane*lane=m_defragmentationLanes[i];
		if(lane->canAllocate(n,m_bytesPerElement,m_show)){
			m_fastLane=lane;
			return;
		}
	}

	/** we need to add a defragmentation lane because the existing lanes have
 * 	no group that can allocate the query */
	
	DefragmentationLane*defragmentationLane=(DefragmentationLane*)__Malloc(sizeof(DefragmentationLane),RAY_MALLOC_TYPE_DEFRAG_LANE,m_show);
	defragmentationLane->constructor(m_numberOfLanes,m_bytesPerElement,m_show);

	m_defragmentationLanes[m_numberOfLanes++]=defragmentationLane;

	m_fastLane=defragmentationLane;
}

/**
 * allocate memory
 */
SmartPointer ChunkAllocatorWithDefragmentation::allocate(int n){ /** 64 is the number of buckets in a MyHashTableGroup */
	#ifdef ASSERT
	if(!(n>=1 && n<=64))
		cout<<"n= "<<n<<endl;
	assert(n>=1&&n<=64);
	#endif

	if(m_fastLane==NULL || !m_fastLane->canAllocate(n,m_bytesPerElement,m_show)){
		updateFastLane(n);
	}

	int group;
	SmallSmartPointer smallSmartPointer=m_fastLane->allocate(n,m_bytesPerElement,m_content,&group);

	/** build the SmartPointer with the
 *	SmallSmartPointer, DefragmentationLane id, and DefragmentationGroup id */
	int globalGroup=m_fastLane->getNumber()*GROUPS_PER_LANE+group;
	SmartPointer smartPointer=globalGroup*ELEMENTS_PER_GROUP+smallSmartPointer;
	return smartPointer;
}

/**
 * deallocate liberates the space.
 * the bitmap is updated and m_allocatedSizes is set to 0 for a.
 * Finally, a call to defragment is performed.
 */
void ChunkAllocatorWithDefragmentation::deallocate(SmartPointer a){
	/** NULL is easy to free 
 * 	Not sure if the code should vomit an error instead.. */
	if(a==SmartPointer_NULL)
		return;

	/** get the DefragmentationGroup */
	int group=a/ELEMENTS_PER_GROUP;
	int correctLaneId=group/GROUPS_PER_LANE;
	int groupInLane=group%GROUPS_PER_LANE;

	/** forward the SmallSmartPointer to the DefragmentationGroup */
	SmallSmartPointer smallSmartPointer=a%ELEMENTS_PER_GROUP;
	m_defragmentationLanes[correctLaneId]->getGroup(groupInLane)->deallocate(smallSmartPointer,m_bytesPerElement,m_content);
}

/** this one is easy,
 * Just forward the SmallSmartPointer to the DefragmentationGroup...
 */
void*ChunkAllocatorWithDefragmentation::getPointer(SmartPointer a){
	if(a==SmartPointer_NULL)
		return NULL;

	int group=a/ELEMENTS_PER_GROUP;
	int correctLaneId=group/GROUPS_PER_LANE;
	int groupInLane=group%GROUPS_PER_LANE;

	/** the DefragmentationGroup knows how to do that */
	SmallSmartPointer smallSmartPointer=a%ELEMENTS_PER_GROUP;
	return m_defragmentationLanes[correctLaneId]->getGroup(groupInLane)->getPointer(smallSmartPointer,m_bytesPerElement);
}

ChunkAllocatorWithDefragmentation::ChunkAllocatorWithDefragmentation(){}

ChunkAllocatorWithDefragmentation::~ChunkAllocatorWithDefragmentation(){}

