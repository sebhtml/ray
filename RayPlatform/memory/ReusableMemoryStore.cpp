/*
 	Ray
    Copyright (C)  2010, 2011  Sébastien Boisvert

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

#include <memory/ReusableMemoryStore.h>
#include <iostream>
#include <assert.h>
using namespace std;

void ReusableMemoryStore::constructor(){
}

/** returns true if size bytes can be allocated
 * from the garbage stack
 */
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
/**
 * add size bytes to the garbage stack
 */
void ReusableMemoryStore::addAddressToReuse(void*p,int size){
	/** don't free these tiny bits -- sizeof(Element) is 8 bytes on a 64-bit system */
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

void ReusableMemoryStore::print(){
	cout<<"ReusableMemoryStore Information"<<endl;
	int totalBytes=0;
	for(map<int,Element*>::iterator i=m_toReuse.begin();
		i!=m_toReuse.end();i++){
		int bytes=i->first;
		Element*ptr=i->second;
		#ifdef ASSERT
		assert(ptr!=NULL);
		#endif
		int count=0;
		while(ptr!=NULL){
			count++;
			ptr=(Element*)ptr->m_next;
		}
		cout<<"("<<bytes<<": "<<count<<") ";
		cout.flush();
		totalBytes+=bytes*count;
	}
	cout<<endl;
	cout<<"Total: "<<totalBytes/1024<<" KiB"<<endl;
	cout.flush();

}
