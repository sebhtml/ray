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

/**
* print allocator information
*/
void ChunkAllocatorWithDefragmentation::print(){
	DefragmentationLane*lane=m_defragmentationLane;
	int lanes=0;
	int availableElements=0;
	while(lane!=NULL){
		lanes++;
		for(int group=0;group<GROUPS_PER_LANE;group++){
			if(lane->m_groups[group].isOnline()){
				availableElements+=lane->m_groups[group].getAvailableElements();
			}
		}
		lane=(DefragmentationLane*)lane->m_next;
	}
	
	cout<<"DefragmentationLanes: "<<lanes<<endl;
	cout<<"GROUPS_PER_LANE: "<<GROUPS_PER_LANE<<endl;
	cout<<"DefragmentationGroups: "<<GROUPS_PER_LANE*lanes<<endl;
	cout<<"ActiveDefragmentationGroups: "<<m_activeGroups<<endl;
	cout<<"ELEMENTS_PER_GROUP: "<<ELEMENTS_PER_GROUP<<endl;
	int totalElements=m_activeGroups*ELEMENTS_PER_GROUP;
	int allocated=totalElements-availableElements;
	double ratio=(0.0+allocated)/totalElements*100;
	cout<<"AllocatedElements: "<<allocated<<"/"<<totalElements<<" ("<<ratio<<"%"<<")"<<endl;
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
	m_activeGroups=0;
	m_show=show;
	m_period=period;
	m_defragmentationLane=NULL;
}

/**
 * allocate memory
 */
SmartPointer ChunkAllocatorWithDefragmentation::allocate(int n){
/**
 * first, try to find a DefragmentationGroup that can allocate the query 
 */
	DefragmentationLane*lane=m_defragmentationLane;
	DefragmentationLane*lastValidLane=m_defragmentationLane;
	int laneId=0;
	while(lane!=NULL){
		for(int group=0;group<GROUPS_PER_LANE;group++){
			if(!lane->m_groups[group].isOnline()){
				lane->m_groups[group].constructor(m_period,m_show);
				m_activeGroups++;
			}

			#ifdef ASSERT
			assert(lane->m_groups[group].isOnline());
			#endif

			if(lane->m_groups[group].canAllocate(n)){
				SmallSmartPointer smallSmartPointer=lane->m_groups[group].allocate(n);
				int globalGroup=laneId*GROUPS_PER_LANE+group;
				SmartPointer smartPointer=globalGroup*ELEMENTS_PER_GROUP+smallSmartPointer;
				return smartPointer;
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

	if(lastValidLane==NULL)
		m_defragmentationLane=defragmentationLane;
	else
		lastValidLane->m_next=defragmentationLane;
	
	/** now we can call the same code again with the new line 
 * 	this is a recursive call
 * 	*/
	return allocate(n);
}

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
	/** NULL is easy to free */
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
	group->deallocate(smallSmartPointer);
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

