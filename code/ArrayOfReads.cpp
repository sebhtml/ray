/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<ArrayOfReads.h>
#include<stdlib.h>
#include<assert.h>

ArrayOfReads::ArrayOfReads(){
	m_array=NULL;
}

void ArrayOfReads::push_back(Read*a){
	if(m_array==NULL){
		m_allocationPeriod=100000;
		m_maxSize=m_allocationPeriod;
		m_elements=0;
		m_array=(Read*)malloc(m_maxSize*sizeof(Read));
		assert(m_array!=NULL);
	}else if(m_elements==m_maxSize){
		m_maxSize+=m_allocationPeriod;
		m_array=(Read*)realloc(m_array,m_maxSize*sizeof(Read));
		assert(m_array!=NULL);
	}
	m_array[m_elements++]=*a;
}

int ArrayOfReads::size(){
	return m_elements;
}

Read*ArrayOfReads::at(int i){
	return m_array+i;
}

Read*ArrayOfReads::operator[](int i){
	return m_array+i;
}

void ArrayOfReads::clear(){
	if(m_array!=NULL){
		free(m_array);
	}
	m_array=NULL;
	m_elements=0;
	m_maxSize=0;
}
