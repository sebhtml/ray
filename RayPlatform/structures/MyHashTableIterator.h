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

#ifndef _MyHashTableIterator_H
#define _MyHashTableIterator_H

#include <structures/MyHashTable.h>
#include <assert.h>

/**
 * \author Sébastien Boisvert
 */
template<class KEY,class VALUE>
class MyHashTableIterator{
	MyHashTable<KEY,VALUE>*m_table;
	uint64_t m_i;
	bool m_hasNext;
	
	void getNext();

public: 
	void constructor(MyHashTable<KEY,VALUE>*a);
	bool hasNext();
	VALUE*next();
};

template<class KEY,class VALUE>
void MyHashTableIterator<KEY,VALUE>::constructor(MyHashTable<KEY,VALUE>*a){
	m_i=0;
	m_table=a;
	getNext();

	#ifdef ASSERT
	assert(!a->needsToCompleteResizing());
	#endif
}

template<class KEY,class VALUE>
void MyHashTableIterator<KEY,VALUE>::getNext(){
	while(m_i<m_table->capacity()&&m_table->isAvailable(m_i))
		m_i++;
	m_hasNext=m_i<m_table->capacity()&&!m_table->isAvailable(m_i);
}

template<class KEY,class VALUE>
bool MyHashTableIterator<KEY,VALUE>::hasNext(){
	return m_hasNext;
}

template<class KEY,class VALUE>
VALUE*MyHashTableIterator<KEY,VALUE>::next(){
	#ifdef ASSERT
	assert(m_i<m_table->capacity());
	#endif
	VALUE*a=m_table->at(m_i);
	m_i++;
	getNext();
	return a;
}

#endif
