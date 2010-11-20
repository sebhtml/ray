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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#include<RingAllocator.h>
#include<common_functions.h>
#include<assert.h>

void RingAllocator::constructor(int chunks,int size){
	m_chunks=chunks;
	m_max=size;
	m_numberOfBytes=m_chunks*m_max;
	m_memory=(uint8_t*)__Malloc(sizeof(uint8_t)*m_chunks*m_max);
	m_current=0;
}

RingAllocator::RingAllocator(){
}

/*
 * allocate a chunk of m_max bytes in constant time
 */
void*RingAllocator::allocate(int a){
	#ifdef ASSERT
	if(a>m_max){
		cout<<a<<endl;
	}
	assert(a<=m_max);
	#endif
	void*address=(void*)(m_memory+m_current*m_max);
	m_current++;
/*
	if(m_current==m_chunks){
		cout<<"Ring completed."<<endl;
	}
*/
	if(m_current==m_chunks){
		m_current=0;
	}
	return address;
}


