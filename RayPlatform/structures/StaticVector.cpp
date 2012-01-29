/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#include <structures/StaticVector.h>
#include <memory/allocator.h>

#include <assert.h>
#include <string.h>

void StaticVector::constructor(int size,const char* type,bool show){
	strcpy(m_type,type);

	#ifdef ASSERT
	assert(size!=0);
	#endif

	m_maxSize=size;
	m_messages=(Message*)__Malloc(sizeof(Message)*m_maxSize,m_type,show);
	m_size=0;
}

Message*StaticVector::operator[](int i){
	return at(i);
}

Message*StaticVector::at(int i){
	#ifdef ASSERT
	assert(i<m_size);
	#endif
	return m_messages+i;
}

// TODO: the message a should be passed as a pointer
void StaticVector::push_back(Message a){
	m_messages[m_size++]=a;
}

int StaticVector::size(){
	return m_size;
}

void StaticVector::clear(){
	m_size=0;
}

bool StaticVector::hasMessage(MessageTag tag){
	for(int i=0;i<m_size;i++){
		if(at(i)->getTag()==tag)
			return true;
	}

	return false;
}
