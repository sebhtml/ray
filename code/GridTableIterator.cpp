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

#include <GridTableIterator.h>
#include <assert.h>

void GridTableIterator::constructor(GridTable*a){
	m_table=a;
	m_currentBin=0;
	m_currentPosition=0;
	#ifdef ASSERT
	assert(m_table!=NULL);
	#endif
}

bool GridTableIterator::hasNext(){
	#ifdef ASSERT
	assert(m_table!=NULL);
	#endif
	getNext();
	return m_currentBin<m_table->getNumberOfBins()&&m_currentPosition<m_table->getNumberOfElementsInBin(m_currentBin);
}

GridData*GridTableIterator::next(){
	getNext();
	#ifdef ASSERT
	assert(m_currentBin<m_table->getNumberOfBins());
	if(m_currentPosition>=m_table->getNumberOfElementsInBin(m_currentBin)){
		cout<<"bin="<<m_currentBin<<" bins="<<m_table->getNumberOfBins()<<" i="<<m_currentPosition<<" size="<<m_table->getNumberOfElementsInBin(m_currentBin)<<endl;
	}
	assert(m_currentPosition<m_table->getNumberOfElementsInBin(m_currentBin));
	#endif
	GridData*element=m_table->getElementInBin(m_currentBin,m_currentPosition);
	m_currentPosition++;
	return element;
}

void GridTableIterator::getNext(){
	while(m_currentBin<m_table->getNumberOfBins()){
		if(m_currentPosition>m_table->getNumberOfElementsInBin(m_currentBin)-1){
			m_currentPosition=0;
			m_currentBin++;
		}else{
			#ifdef ASSERT
			assert(m_currentBin<m_table->getNumberOfBins());
			assert(m_currentPosition<m_table->getNumberOfElementsInBin(m_currentBin));
			#endif
			return;
		}
	}
}
