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









#include"Vertex.h"
#include<iostream>
using namespace std;

void Vertex::constructor(){
	m_coverage=0;
	m_outgoingEdges=NULL;
	m_ingoingEdges=NULL;
}

void Vertex::setCoverage(int coverage){
	m_coverage=coverage;
}

int Vertex::getCoverage(){
	return m_coverage;
}

void Vertex::addOutgoingEdge(int rank,void*ptr,MyAllocator*allocator){
	if(hasEdge(m_outgoingEdges,rank,ptr))
		return;
	
	Edge*e=(Edge*)allocator->allocate(sizeof(Edge));
	e->constructor(rank,ptr);
	if(m_outgoingEdges!=NULL){
		e->setNext(m_outgoingEdges);
	}
	m_outgoingEdges=e;
}

void Vertex::addIngoingEdge(int rank,void*ptr,MyAllocator*allocator){
	if(hasEdge(m_ingoingEdges,rank,ptr))
		return;
	
	Edge*e=(Edge*)allocator->allocate(sizeof(Edge));
	e->constructor(rank,ptr);
	if(m_ingoingEdges!=NULL){
		e->setNext(m_ingoingEdges);
	}
	m_ingoingEdges=e;
}

bool Vertex::hasEdge(Edge*e,int rank,void*ptr){
	Edge*t=e;
	int i=0;
	while(t!=NULL){
		if(t->getRank()==rank and t->getPtr()==ptr){
			//cout<<"Found"<<endl;
			return true;
		}
		t=t->getNext();
		i++;
	}
	if(i>4){
		cout<<"Too many edges "<<i<<endl;
		cout<<rank<<" "<<ptr<<endl;
	}
	return false;
}

