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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <structures/MyHashTableIterator.h>
#include <assert.h>
#include <iostream>
using namespace std;

void GridTableIterator::constructor(GridTable*a,int wordSize,Parameters*parameters){
	m_parameters=parameters;
	m_mustProcessOtherKey=false;
	m_iterator.constructor(a->getHashTable());
}

bool GridTableIterator::hasNext(){
	bool iteratorHasNext=m_iterator.hasNext()||m_mustProcessOtherKey;
	return iteratorHasNext;
}

Vertex*GridTableIterator::next(){
	if(m_mustProcessOtherKey){
		m_mustProcessOtherKey=false;
		m_currentKey=m_currentKey.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
		return m_currentEntry;
	}
	#ifdef ASSERT
	assert(hasNext());
	#endif
	m_currentEntry=m_iterator.next();
	m_currentKey=m_currentEntry->m_lowerKey;
	m_mustProcessOtherKey=true;
	return m_currentEntry;
}

Kmer*GridTableIterator::getKey(){
	return &m_currentKey;
}
