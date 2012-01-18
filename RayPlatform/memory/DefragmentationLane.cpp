/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#include <memory/DefragmentationLane.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
using namespace std;

/**
 * Time complexity: O(GROUPS_PER_LANE) to initializing DefragmentationGroup objects 
 */
void DefragmentationLane::constructor(int number,int ,bool ){
	m_number=number;
	for(int i=0;i<GROUPS_PER_LANE;i++)
		m_groups[i].setPointers();

	m_fastGroup=INVALID_GROUP;
	m_numberOfActiveGroups=0;
	m_numberOfFastGroups=0;
}

/** Update m_fastGroup rapidly */
void DefragmentationLane::getFastGroup(int n,int bytesPerElement,bool show){
	/* if m_fastGroup can allocate n elements, use it right away
	Time complexity:  O(1) */
	if(m_fastGroup!=INVALID_GROUP && m_groups[m_fastGroup].canAllocate(n))
		return;

	/* try to find the fast group in the list m_fastGroups
	Time complexity: O(NUMBER_OF_FAST_GROUPS) (default=128)*/
	for(int i=0;i<m_numberOfFastGroups;i++){
		if(m_fastGroups[i]!=INVALID_GROUP && m_groups[m_fastGroups[i]].canAllocate(n)){
			m_fastGroup=m_fastGroups[i];
			return;
		}
	}

	/* update m_fastGroups using the m_numberOfActiveGroups DefragmentationGroup objects 
	Time complexity: O(m_numberOfActiveGroups) where m_numberOfActiveGroups <= GROUPS_PER_LANE */
	int target=64;
	m_numberOfFastGroups=0;
	int group=0;

	while(m_numberOfFastGroups<NUMBER_OF_FAST_GROUPS && group<m_numberOfActiveGroups){
		if(m_groups[group].canAllocate(target))
			m_fastGroups[m_numberOfFastGroups++]=group;
		group++;
	}
		
	/* if there is at least 1 fast group */
	if(m_numberOfFastGroups>0){
		m_fastGroup=m_fastGroups[0];
		return;
	}

	/* no more group can be created and therefore the DefragmentationLane is too busy */
	if(m_numberOfActiveGroups==GROUPS_PER_LANE){
		m_fastGroup=INVALID_GROUP;
		#ifdef ASSERT
		assert(m_numberOfFastGroups==0);
		#endif
		return;
	}

	/** activate a DefragmentationGroup */
	m_groups[m_numberOfActiveGroups].constructor(bytesPerElement,show);

	/* there is only 1 one fast group */
	m_fastGroups[m_numberOfFastGroups]=m_numberOfActiveGroups;
	m_fastGroup=m_fastGroups[0];
	m_numberOfActiveGroups++;
	m_numberOfFastGroups++;

	#ifdef ASSERT
	assert(m_numberOfFastGroups==1);
	#endif

	return;
}

/**
 * can the DefragmentationLane allocates rapidly n elements ? 
 * Time complexity: O(1) if m_fastGroup can deliver
 * otherwise, O(GROUPS_PER_LANE)
 * */
bool DefragmentationLane::canAllocate(int n,int bytesPerElement,bool show){
	if(m_fastGroup!=INVALID_GROUP)
		if(m_groups[m_fastGroup].canAllocate(n))
			return true;

	getFastGroup(n,bytesPerElement,show);
	return m_fastGroup!=INVALID_GROUP;
}

/**
 * Time complexity: O(1)
 */
SmallSmartPointer DefragmentationLane::allocate(int n,int bytesPerElement,int*group){
	#ifdef ASSERT
	assert(m_fastGroup!=-1);
	if(!(m_fastGroup<GROUPS_PER_LANE))
		cout<<"m_fastGroup= "<<m_fastGroup<<endl;
	assert(m_fastGroup<GROUPS_PER_LANE);
	assert(m_fastGroup>=0);
	assert(n>0);
	assert(bytesPerElement>0);
	assert(group!=NULL);
	assert(m_fastGroup!=INVALID_GROUP);
	assert(m_fastGroup<m_numberOfActiveGroups);
	#endif

	// the fast group is always valid
	// this is an invariant
	(*group)=m_fastGroup;

	return m_groups[m_fastGroup].allocate(n);
}

/**
 * Time complexity: O(1)
 */
int DefragmentationLane::getNumber(){
	return m_number;
}

/**
 * Time complexity: O(1)
 */
DefragmentationGroup*DefragmentationLane::getGroup(int i){
	#ifdef ASSERT
	assert(i<GROUPS_PER_LANE);
	#endif
	return m_groups+i;
}
