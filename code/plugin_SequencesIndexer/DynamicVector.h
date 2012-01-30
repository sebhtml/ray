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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _DynamicVector_H
#define _DynamicVector_H

#include <memory/MyAllocator.h>

/**
 * a dynamic vector, this is a template
 * \author Sébastien Boisvert
 *
 */
template<class TYPE>
class DynamicVector{
	int m_size;
	int m_capacity;
	TYPE*m_array;
public:
	void constructor();
	void destructor(MyAllocator*allocator);
	int size();
	void push_back(TYPE a,MyAllocator*allocator);
	TYPE at(int i);
};


template<class TYPE>
void DynamicVector<TYPE>::constructor(){
	m_size=0;
	m_capacity=0;
	m_array=NULL;
}

template<class TYPE>
void DynamicVector<TYPE>::destructor(MyAllocator*allocator){
	allocator->free(m_array,m_capacity*sizeof(TYPE));
	m_array=NULL;
	m_size=0;
	m_capacity=0;
}

template<class TYPE>
int DynamicVector<TYPE>::size(){
	return m_size;
}

template<class TYPE>
void DynamicVector<TYPE>::push_back(TYPE a,MyAllocator*allocator){
	if(m_size==m_capacity){// needs to grow
		int step=1;
		if(m_capacity!=0)
			step=m_capacity;
		m_capacity+=step;
		TYPE*newArray=(TYPE*)allocator->allocate(m_capacity*sizeof(TYPE));
		if(m_array!=NULL){
			int currentBytes=m_size*sizeof(TYPE);
			memcpy(newArray,m_array,currentBytes); /* this is fast */
			allocator->free(m_array,currentBytes);
		}
		m_array=newArray;
	}
	m_size++;
	m_array[m_size-1]=a;
}

template<class TYPE>
TYPE DynamicVector<TYPE>::at(int i){
	return m_array[i];
}

#endif

