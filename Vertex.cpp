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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include<assert.h>
#include<vector>
#include<Vertex.h>
#include<cstdlib>
#include<common_functions.h>
#include<iostream>
using namespace std;

void Vertex::constructor(){
	m_coverage=0;
	m_edges=0;
	m_readsStartingHere=NULL;
	m_direction=NULL;
}

void Vertex::setCoverage(int coverage){
	COVERAGE_TYPE max=0;
	max=max-1;// underflow.
	if(m_coverage==max){ // maximum value for unsigned char.
		return;
	}

	m_coverage=coverage;
}

int Vertex::getCoverage(){
	return m_coverage;
}

vector<uint64_t> Vertex::getIngoingEdges(uint64_t a,int k){
	vector<uint64_t> b;
	for(int i=0;i<4;i++){
		int j=((((uint64_t)m_edges)<<(63-i))>>63);
		if(j==1){
			uint64_t l=((a<<(64-2*k+2))>>(64-2*k))|((uint64_t)i);
			b.push_back(l);
		}
	}
	return b;
}

vector<uint64_t> Vertex::getOutgoingEdges(uint64_t a,int k){
	vector<uint64_t> b;
	for(int i=0;i<4;i++){
		int j=((((uint64_t)m_edges)<<(59-i))>>63);
		if(j==1){
			uint64_t l=(a>>2)|(((uint64_t)i)<<(2*(k-1)));
			b.push_back(l);
		}
	}

	return b;
}

void Vertex::addIngoingEdge(uint64_t a,int k){
	m_edges=m_edges|(1<<((a<<(62))>>62));
}

void Vertex::addOutgoingEdge(uint64_t a,int k){
	uint8_t s1Last=getFirstSegmentLastCode(a,k,_SEGMENT_LENGTH);
	uint8_t s2Last=getSecondSegmentLastCode(a,k,_SEGMENT_LENGTH);

	m_edges=m_edges|(1<<(4+((a<<(64-2*k))>>62)));
}

void Vertex::addRead(int rank,int i,char c,MyAllocator*allocator){
	ReadAnnotation*e=(ReadAnnotation*)allocator->allocate(sizeof(ReadAnnotation));
	#ifdef DEBUG
	assert(e!=NULL);
	#endif
	e->constructor(rank,i,c);
	if(m_readsStartingHere!=NULL){
		e->setNext(m_readsStartingHere);
	}
	m_readsStartingHere=e;
}

void Vertex::addDirection(int wave,int progression,MyAllocator*allocator){
	Direction*e=(Direction*)allocator->allocate(sizeof(Direction));
	e->constructor(wave,progression);
	if(m_direction!=NULL){
		e->setNext(m_direction);
	}
	m_direction=e;
}




bool Vertex::isAssembled(){
	return m_direction!=NULL;
}

ReadAnnotation*Vertex::getReads(){
	return m_readsStartingHere;
}

vector<Direction> Vertex::getDirections(){
	vector<Direction> a;
	Direction*e=m_direction;
	while(e!=NULL){
		a.push_back(*e);
		e=e->getNext();
	}
	return a;
}

void Vertex::clearDirections(){
	m_direction=NULL;
}
