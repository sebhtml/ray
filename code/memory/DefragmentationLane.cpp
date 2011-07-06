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

void DefragmentationLane::constructor(int number,int bytePerElement,bool show){
	m_number=number;
	for(int i=0;i<GROUPS_PER_LANE;i++)
		m_groups[i].setPointers();
	m_next=NULL;

	m_groups[0].constructor(bytePerElement,show);
}

bool DefragmentationLane::canAllocate(int n,int bytesPerElement,bool show){
	for(int group=0;group<GROUPS_PER_LANE;group++){
		/** activate a lane */
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

SmallSmartPointer DefragmentationLane::allocate(int n,int bytesPerElement,uint16_t*content,int*group){
	#ifdef ASSERT
	assert(m_fastGroup!=-1);
	#endif
	(*group)=m_fastGroup;
	return m_groups[m_fastGroup].allocate(n,bytesPerElement,content);
}

int DefragmentationLane::getNumber(){
	return m_number;
}

DefragmentationGroup*DefragmentationLane::getGroup(int i){
	#ifdef ASSERT
	assert(i<GROUPS_PER_LANE);
	#endif
	return m_groups+i;
}
