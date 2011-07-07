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

#include <memory/DefragmentationLane.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
using namespace std;

/**
 * Time complexity: O(1)
 */
void DefragmentationLane::constructor(int number,int bytePerElement,bool show){
	m_number=number;
	for(int i=0;i<GROUPS_PER_LANE;i++)
		m_groups[i].setPointers();

	m_fastGroup=0;
	m_groups[0].constructor(bytePerElement,show);
}

/**
 * can the DefragmentationLane allocates rapidly n elements ? 
 * Time complexity: O(1) if m_fastGroup can deliver
 * otherwise, O(GROUPS_PER_LANE)
 * */
bool DefragmentationLane::canAllocate(int n,int bytesPerElement,bool show){
	if(m_groups[m_fastGroup].canAllocate(n))
		return true;

	for(int group=0;group<GROUPS_PER_LANE;group++){
		if(!m_groups[group].isOnline()){
			m_groups[group].constructor(bytesPerElement,show);
		}

		#ifdef ASSERT
		assert(m_groups[group].isOnline());
		#endif

		/** use this DefragmentationGroup if it can handle the query */
		if(m_groups[group].canAllocate(n)){
			m_fastGroup=group;
			#ifdef ASSERT
			assert(group<GROUPS_PER_LANE);
			assert(m_groups[group].isOnline());
			#endif
			return true;
		}
	}
	m_fastGroup=-1;
	return false;
}

/**
 * Time complexity: O(1)
 */
SmallSmartPointer DefragmentationLane::allocate(int n,int bytesPerElement,uint16_t*content,int*group){
	#ifdef ASSERT
	assert(m_fastGroup!=-1);
	if(!(m_fastGroup<GROUPS_PER_LANE))
		cout<<"m_fastGroup= "<<m_fastGroup<<endl;
	assert(m_fastGroup<GROUPS_PER_LANE);
	assert(m_fastGroup>=0);
	assert(n>0);
	assert(bytesPerElement>0);
	assert(group!=NULL);
	assert(content!=NULL);
	#endif
	(*group)=m_fastGroup;
	return m_groups[m_fastGroup].allocate(n,bytesPerElement,content);
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
