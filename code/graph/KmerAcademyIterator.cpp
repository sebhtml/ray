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

#include <graph/KmerAcademyIterator.h>
#include <structures/MyHashTableIterator.h>
#include <assert.h>
#include <iostream>
using namespace std;

void KmerAcademyIterator::constructor(KmerAcademy*a,int wordSize,Parameters*parameters){
	m_parameters=parameters;
	m_mustProcessOtherKey=false;
	m_iterator.constructor(a->getHashTable());
}

bool KmerAcademyIterator::hasNext(){
	bool iteratorHasNext=m_iterator.hasNext()||m_mustProcessOtherKey;
	return iteratorHasNext;
}

KmerCandidate*KmerAcademyIterator::next(){
	if(m_mustProcessOtherKey){
		m_mustProcessOtherKey=false;
		m_currentKey=complementVertex(&m_currentKey,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
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

Kmer*KmerAcademyIterator::getKey(){
	return &m_currentKey;
}
