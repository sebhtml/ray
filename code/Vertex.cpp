/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#include<assert.h>
#include<vector>
#include<Vertex.h>
#include<cstdlib>
#include<common_functions.h>
#include<iostream>
using namespace std;

void Vertex::constructor(){
	m_coverage_lower=0;
	m_coverage_higher=0;
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	m_ingoingEdges=NULL;
	m_outgoingEdges=NULL;
	#else
	m_edges_lower=0;
	m_edges_higher=0;
	#endif
	m_readsStartingHere_lower=NULL;
	m_direction_lower=NULL;
	m_readsStartingHere_higher=NULL;
	m_direction_higher=NULL;
}

void Vertex::setCoverage(uint64_t a,int coverage){
	COVERAGE_TYPE max=0;
	max=max-1;// underflow.
	COVERAGE_TYPE*ptr=&m_coverage_lower;
	if(a!=m_lowerKey){
		ptr=&m_coverage_higher;
	}
	if(*ptr==max){ // maximum value for unsigned char.
		return;
	}

	*ptr=coverage;
}

int Vertex::getCoverage(uint64_t a){
	COVERAGE_TYPE*ptr=&m_coverage_lower;
	if(a!=m_lowerKey){
		ptr=&m_coverage_higher;
	}
	return *ptr;
}

vector<uint64_t> Vertex::getIngoingEdges(uint64_t a,int k){
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	VertexLinkedList*t=m_ingoingEdges;
	while(t!=NULL){
		b.push_back(t->getVertex());
		t=t->getNext();
	}
	#else
	if(a==m_lowerKey){
		return _getIngoingEdges(a,m_edges_lower,k);
	}
	return _getIngoingEdges(a,m_edges_higher,k);
	#endif
}

vector<uint64_t> Vertex::getOutgoingEdges(uint64_t a,int k){
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	VertexLinkedList*t=m_outgoingEdges;
	while(t!=NULL){
		b.push_back(t->getVertex());
		t=t->getNext();
	}
	#else
	if(a==m_lowerKey){
		return _getOutgoingEdges(a,m_edges_lower,k);
	}
	return _getOutgoingEdges(a,m_edges_higher,k);
	
	#endif
}

#ifndef USE_DISTANT_SEGMENTS_GRAPH
void Vertex::addIngoingEdge_ClassicMethod(uint64_t vertex,uint64_t a,int k){
	uint8_t s1First=getFirstSegmentFirstCode(a,k,_SEGMENT_LENGTH);
	// add s1First to edges.
	uint8_t newBits=(1<<(s1First));
	if(vertex==m_lowerKey){
		m_edges_lower=m_edges_lower|newBits;
	}else{
		m_edges_higher=m_edges_higher|newBits;
	}
}
#endif

void Vertex::deleteIngoingEdge(uint64_t vertex,uint64_t a,int k){
	uint8_t s1First=getFirstSegmentFirstCode(a,k,_SEGMENT_LENGTH);
	// delete s1First from edges.
	uint8_t newBits=(1<<(s1First));
	newBits=~newBits;
	if(vertex==m_lowerKey){
		m_edges_lower=m_edges_lower&newBits;
	}else{
		m_edges_higher=m_edges_higher&newBits;
	}
}

void Vertex::addIngoingEdge(uint64_t vertex,uint64_t a,int k){
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	VertexLinkedList*t=m_ingoingEdges;
	while(t!=NULL){
		if(t->getVertex()==a)
			return;
		t=t->getNext();
	}

	VertexLinkedList*n=(VertexLinkedList*)m->allocate(sizeof(VertexLinkedList));
	n->constructor(a);
	n->setNext(m_ingoingEdges);
	m_ingoingEdges=n;
	#else
	addIngoingEdge_ClassicMethod(vertex,a,k);
	#endif
}

#ifndef USE_DISTANT_SEGMENTS_GRAPH
void Vertex::addOutgoingEdge_ClassicMethod(uint64_t vertex,uint64_t a,int k){
	uint8_t s2Last=getSecondSegmentLastCode(a,k,_SEGMENT_LENGTH);
	// description of m_edges:
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0

	// put s2Last in m_edges
	uint64_t newBits=1<<(4+s2Last);
	if(vertex==m_lowerKey){
		m_edges_lower=m_edges_lower|newBits;
	}else{
		m_edges_higher=m_edges_higher|newBits;
	}
}

#endif

void Vertex::deleteOutgoingEdge(uint64_t vertex,uint64_t a,int k){
	uint8_t s2Last=getSecondSegmentLastCode(a,k,_SEGMENT_LENGTH);
	uint64_t newBits=1<<(4+s2Last);
	newBits=~newBits;
	if(vertex==m_lowerKey){
		m_edges_lower=m_edges_lower&newBits;
	}else{
		m_edges_higher=m_edges_higher&newBits;
	}
}

void Vertex::addOutgoingEdge(uint64_t vertex,uint64_t a,int k){
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	VertexLinkedList*t=m_outgoingEdges;
	while(t!=NULL){
		if(t->getVertex()==a)
			return;
		t=t->getNext();
	}

	VertexLinkedList*n=(VertexLinkedList*)m->allocate(sizeof(VertexLinkedList));
	n->constructor(a);
	n->setNext(m_outgoingEdges);
	m_outgoingEdges=n;

	#else
	addOutgoingEdge_ClassicMethod(vertex,a,k);
	#endif
}

void Vertex::addRead(uint64_t vertex,ReadAnnotation*e){
	if(vertex==m_lowerKey){
		if(m_readsStartingHere_lower!=NULL){
			e->setNext(m_readsStartingHere_lower);
		}
		m_readsStartingHere_lower=e;
	}else{
		if(m_readsStartingHere_higher!=NULL){
			e->setNext(m_readsStartingHere_higher);
		}
		m_readsStartingHere_higher=e;
	}
}

void Vertex::addDirection(uint64_t vertex,Direction*e){
	if(vertex==m_lowerKey){
		e->setNext(m_direction_lower);
		m_direction_lower=e;
	}else{
		e->setNext(m_direction_higher);
		m_direction_higher=e;
	}
}

ReadAnnotation*Vertex::getReads(uint64_t vertex){
	if(vertex==m_lowerKey){
		return m_readsStartingHere_lower;
	}
	return m_readsStartingHere_higher;
}

vector<Direction> Vertex::getDirections(uint64_t vertex){
	if(vertex==m_lowerKey){
		vector<Direction> a;
		Direction*e=m_direction_lower;
		while(e!=NULL){
			a.push_back(*e);
			e=e->getNext();
		}
		return a;
	}
	vector<Direction> a;
	Direction*e=m_direction_higher;
	while(e!=NULL){
		a.push_back(*e);
		e=e->getNext();
	}
	return a;
}

void Vertex::clearDirections(uint64_t a){
	if(a==m_lowerKey){
		m_direction_lower=NULL;
	}else{
		m_direction_higher=NULL;
	}
}

uint8_t Vertex::getEdges(uint64_t a){
	if(a==m_lowerKey){
		return m_edges_lower;
	}
	return m_edges_higher;
}
