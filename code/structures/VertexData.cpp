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

#include <structures/VertexData.h>

void VertexData::constructor(){
	m_readsStartingHere=NULL;
	m_directions=NULL;
}

void VertexData::addRead(Kmer*vertex,ReadAnnotation*e){
	e->setNext(m_readsStartingHere);
	m_readsStartingHere=e;
}

void VertexData::addDirection(Kmer*vertex,Direction*e){
	e->setNext(m_directions);
	m_directions=e;
}

ReadAnnotation*VertexData::getReads(Kmer*vertex){
	return m_readsStartingHere;
}

vector<Direction> VertexData::getDirections(Kmer*vertex){
	bool seekLower=false;
	if(vertex->isEqual(&m_lowerKey)){
		seekLower=true;
	}
	vector<Direction> a;
	Direction*e=m_directions;
	while(e!=NULL){
		if(e->isLower()==seekLower){
			a.push_back(*e);
		}
		e=e->getNext();
	}
	return a;
}

void VertexData::clearDirections(Kmer*a){
	m_directions=NULL;
}
