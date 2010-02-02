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
}

void Vertex::setCoverage(int coverage){
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
			uint64_t l=((a<<(64-2*(k+1)+2))>>(64-2*(k+1)+2)<<2)|i;
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
			uint64_t l=((a>>2))|(i<<(2*(k-1)));
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

void Vertex::addRead(int rank,void*ptr,MyAllocator*allocator){
	Edge*e=(Edge*)allocator->allocate(sizeof(Edge));
	e->constructor(rank,ptr);
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


