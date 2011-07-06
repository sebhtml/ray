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
	DefragmentationLane*lane=m_defragmentationLane;

	while(lane!=NULL){
		#ifdef ASSERT
		assert(lane!=NULL);
		#endif
		for(int group=0;group<GROUPS_PER_LANE;group++){
			if(lane->m_groups[group].isOnline()){
				if(lane->m_groups[group].defragment(m_period,m_content,false))
					return;
			}else{
				/* no group are online after this one since this one is not online */
				break;
			}
		}
		lane=(DefragmentationLane*)lane->m_next;
	}
}

/**
* print allocator information
*/
void ChunkAllocatorWithDefragmentation::print(){
	DefragmentationLane*lane=m_defragmentationLane;
	int lanes=0;
	while(lane!=NULL){
		lane=(DefragmentationLane*)lane->m_next;
		lanes++;
	}
	cout<<"DefragmentationLanes: "<<lanes<<endl;

	lanes=0;
	lane=m_defragmentationLane;
	while(lane!=NULL){
		#ifdef ASSERT
		assert(lane!=NULL);
		#endif
		cout<<"DefragmentationLane: "<<lanes<<endl;
		for(int group=0;group<GROUPS_PER_LANE;group++){
			if(lane->m_groups[group].isOnline()){
				int availableElements=lane->m_groups[group].getAvailableElements();
				#ifdef ASSERT
				int usedElements=(ELEMENTS_PER_GROUP-availableElements);
				if(usedElements<0)
					cout<<"group "<<group<<" ELEMENTS_PER_GROUP "<<ELEMENTS_PER_GROUP<<" availableElements "<<availableElements<<endl;
				assert(usedElements>=0);
				#endif

				cout<<"  DefragmentationGroup: "<<group<<" usage: "<<(ELEMENTS_PER_GROUP-availableElements)<<"/"<<ELEMENTS_PER_GROUP<<" FreeSliceStart: "<<lane->m_groups[group].getFreeSliceStart()<<endl;
			}
		}
		lane=(DefragmentationLane*)lane->m_next;
		lanes++;
	}
}

/** clear allocations */
void ChunkAllocatorWithDefragmentation::destructor(){
	/** destroy DefragmentationLanes */
	DefragmentationLane*lane=m_defragmentationLane;
	while(lane!=NULL){
		for(int group=0;group<GROUPS_PER_LANE;group++){
			if(!lane->m_groups[group].isOnline())
				lane->m_groups[group].destructor(m_show);
		}
		lane=(DefragmentationLane*)lane->m_next;
	}

	lane=m_defragmentationLane;
	while(lane!=NULL){
		DefragmentationLane*toFree=lane;
		lane=(DefragmentationLane*)lane->m_next;
		__Free(toFree,RAY_MALLOC_TYPE_DEFRAG_LANE,m_show);
	}
	m_defragmentationLane=NULL;
}

/** constructor almost does nothing  */
void ChunkAllocatorWithDefragmentation::constructor(int period,bool show){
	m_content=(uint16_t*)__Malloc(ELEMENTS_PER_GROUP*sizeof(uint16_t),RAY_MALLOC_TYPE_DEFRAG_LANE,show);
	m_show=show;
	m_period=period;
	m_defragmentationLane=NULL;
	m_fastLane=NULL;
}

/**
 * update:
 *  m_fastLane
 *  m_fastGroup
 *  m_fastLaneNumber
 */
void ChunkAllocatorWithDefragmentation::updateFastLane(int n){
	DefragmentationLane*lane=m_defragmentationLane;
	DefragmentationLane*lastValidLane=m_defragmentationLane;
	int laneId=0;

	/** find a DefragmentationGroup in a DefragmentationLane that 
 * can accomodate the query */
	while(lane!=NULL){
		for(int group=0;group<GROUPS_PER_LANE;group++){
			/** activate a lane */
			if(!lane->m_groups[group].isOnline()){
				lane->m_groups[group].constructor(m_period,m_show);
			}

			#ifdef ASSERT
			assert(lane->m_groups[group].isOnline());
			#endif

			/** use this DefragmentationGroup if it can handle the query */
			if(lane->m_groups[group].canAllocate(n)){
				#ifdef ASSERT
				assert(group<GROUPS_PER_LANE);
				assert(lane->m_groups[group].isOnline());
				#endif
			
				m_fastLane=lane;
				m_fastLaneNumber=laneId;
				m_fastGroup=group;
				return;
			}
		}
		lane=(DefragmentationLane*)lane->m_next;
		if(lane!=NULL)
			lastValidLane=lane;

		laneId++;
	}

	/** we need to add a defragmentation lane because the existing lanes have
 * 	no group that can allocate the query */
	
	DefragmentationLane*defragmentationLane=(DefragmentationLane*)__Malloc(sizeof(DefragmentationLane),RAY_MALLOC_TYPE_DEFRAG_LANE,m_show);
	for(int i=0;i<GROUPS_PER_LANE;i++)
		defragmentationLane->m_groups[i].setPointers();
	defragmentationLane->m_next=NULL;

	defragmentationLane->m_groups[0].constructor(m_period,m_show);

	m_fastLane=defragmentationLane;
	m_fastGroup=0;
	m_fastLaneNumber=laneId;

	if(lastValidLane==NULL)
		m_defragmentationLane=defragmentationLane;
	else
		lastValidLane->m_next=defragmentationLane;
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

	/** TODO keep fastLaneId and fastGroupId for faster look-up. */
	if(m_fastLane==NULL || !m_fastLane->m_groups[m_fastGroup].canAllocate(n)){
		updateFastLane(n);
	}
	#ifdef ASSERT
	assert(m_fastLane->m_groups[m_fastGroup].canAllocate(n));
	#endif

	SmallSmartPointer smallSmartPointer=m_fastLane->m_groups[m_fastGroup].allocate(n,m_period,m_content);

	/** build the SmartPointer with the
 *	SmallSmartPointer, DefragmentationLane id, and DefragmentationGroup id */
	int globalGroup=m_fastLaneNumber*GROUPS_PER_LANE+m_fastGroup;
	SmartPointer smartPointer=globalGroup*ELEMENTS_PER_GROUP+smallSmartPointer;
	return smartPointer;
}

/**
 * get the DefragmentationGroup responsible for the SmartPointer a 
 */
DefragmentationGroup*ChunkAllocatorWithDefragmentation::getGroup(SmartPointer a){
	/** to get  the group of a small pointer, we just need
 * 	to walk the list  */
	int group=a/ELEMENTS_PER_GROUP;
	int correctLaneId=group/GROUPS_PER_LANE;
	int groupInLane=group%GROUPS_PER_LANE;

	DefragmentationLane*lane=m_defragmentationLane;
	int laneId=0;
	while(lane!=NULL){
		if(laneId==correctLaneId){
			return &(lane->m_groups[groupInLane]);
		}
		lane=(DefragmentationLane*)lane->m_next;
		laneId++;
	}

	/** this is a fatal error and it should never happen */
	#ifdef ASSERT
	cout<<"Failed to get group for SmartPointer "<<a<<endl;
	assert(false);
	#endif

	return NULL;
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
	DefragmentationGroup*group=getGroup(a);

	/** the DefragmentationGroup must exist */
	#ifdef ASSERT
	assert(group!=NULL);
	assert(group->isOnline());
	#endif

	/** forward the SmallSmartPointer to the DefragmentationGroup */
	SmallSmartPointer smallSmartPointer=a%ELEMENTS_PER_GROUP;
	group->deallocate(smallSmartPointer,m_period);
}

/** this one is easy,
 * Just forward the SmallSmartPointer to the DefragmentationGroup...
 */
void*ChunkAllocatorWithDefragmentation::getPointer(SmartPointer a){
	if(a==SmartPointer_NULL)
		return NULL;

	DefragmentationGroup*group=getGroup(a);

	#ifdef ASSERT
	assert(group!=NULL);
	assert(group->isOnline());
	#endif

	/** the DefragmentationGroup knows how to do that */
	SmallSmartPointer smallSmartPointer=a%ELEMENTS_PER_GROUP;
	return group->getPointer(smallSmartPointer,m_period);
}

int ChunkAllocatorWithDefragmentation::getAllocationSize(SmartPointer a){
	if(a==SmartPointer_NULL)
		return 0;

	DefragmentationGroup*group=getGroup(a);

	#ifdef ASSERT
	assert(group!=NULL);
	assert(group->isOnline());
	#endif

	/** the DefragmentationGroup knows how to do that */
	SmallSmartPointer smallSmartPointer=a%ELEMENTS_PER_GROUP;
	return group->getAllocationSize(smallSmartPointer);
}

ChunkAllocatorWithDefragmentation::ChunkAllocatorWithDefragmentation(){}

ChunkAllocatorWithDefragmentation::~ChunkAllocatorWithDefragmentation(){}

