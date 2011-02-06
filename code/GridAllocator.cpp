/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#include <GridAllocator.h>

void GridAllocator::constructor(){
	int size=16777216;
	m_allocator.constructor(size);
}

GridData*GridAllocator::allocate(int a,uint16_t*reserved){
	
	if(m_toReuse.count(a)>0){
		GridData*tmp=m_toReuse[a];
		GridData*next=(GridData*)tmp->m_key;
		if(next==NULL){
			m_toReuse.erase(a);
		}else{
			m_toReuse[a]=next;
		}
		*reserved=a;
		return tmp;
	}

	//a=roundNumber(a,4);
	GridData*addr=(GridData*)m_allocator.allocate(sizeof(GridData)*a);
	*reserved=a;
	return addr;
}

void GridAllocator::free(GridData*a,int b){
	if(m_toReuse.count(b)==0){
		a->m_key=(uint64_t)NULL;
	}else{
		a->m_key=(uint64_t)m_toReuse[b];
	}
	m_toReuse[b]=a;
}

MyAllocator*GridAllocator::getAllocator(){
	return &m_allocator;
}


