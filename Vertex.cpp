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








#include<vector>
#include"Vertex.h"
#include<cstdlib>
#include"common_functions.h"
#include<iostream>
#include"types.h"
using namespace std;

void Vertex::constructor(){
	m_coverage=0;
	m_edges=0;
	m_assembled=false;
	m_readsStartingHere=NULL;
}

void Vertex::setCoverage(int coverage){
	if(m_coverage==255){ // maximum value for unsigned char.
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
	m_edges=m_edges|(1<<(4+((a<<(64-2*k))>>62)));
}

void Vertex::addRead(int rank,int i,MyAllocator*allocator){
	ReadAnnotation*e=(ReadAnnotation*)allocator->allocate(sizeof(ReadAnnotation));
	e->constructor(rank,i);
	if(m_readsStartingHere!=NULL){
		e->setNext(m_readsStartingHere);
	}
	m_readsStartingHere=e;
}



void Vertex::assemble(){
	m_assembled=true;
}

bool Vertex::isAssembled(){
	return m_assembled;
}


