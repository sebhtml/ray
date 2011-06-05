/*
 	Ray
    Copyright (C)  2010, 2011  Sébastien Boisvert

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

#include <ReusableMemoryStore.h>
#include <assert.h>

void ReusableMemoryStore::constructor(){
}

bool ReusableMemoryStore::hasAddressesToReuse(int size){
	bool test=m_toReuse.count(size)>0;
	#ifdef ASSERT
	if(test){
		assert(m_toReuse[size]!=NULL);
	}
	#endif
	return test;
}

void*ReusableMemoryStore::reuseAddress(int size){
	#ifdef ASSERT
	assert(m_toReuse.count(size)>0 && m_toReuse[size]!=NULL);
	#endif

	Element*tmp=m_toReuse[size];
	#ifdef ASSERT
	assert(tmp!=NULL);
	#endif
	Element*next=(Element*)tmp->m_next;
	m_toReuse[size]=next;
	if(m_toReuse[size]==NULL){
		m_toReuse.erase(size);
	}
	#ifdef ASSERT
	if(m_toReuse.count(size)>0){
		assert(m_toReuse[size]!=NULL);
	}
	#endif
	return tmp;
}

void ReusableMemoryStore::addAddressToReuse(void*p,int size){
	if(size<(int)sizeof(Element)){
		return;
	}
	#ifdef ASSERT
	assert(p!=NULL);
	#endif
	Element*ptr=(Element*)p;
	ptr->m_next=NULL;
	if(m_toReuse.count(size)>0){
		Element*next=m_toReuse[size];
		#ifdef ASSERT
		assert(next!=NULL);
		#endif
		ptr->m_next=next;
	}
	m_toReuse[size]=ptr;
}

void ReusableMemoryStore::reset(){
	m_toReuse.clear();
}
