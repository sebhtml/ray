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
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	m_ingoingEdges=NULL;
	m_outgoingEdges=NULL;
	#else
	m_edges_lower=0;
	#endif
	m_readsStartingHere=NULL;
	m_directions=NULL;
}

void Vertex::setCoverage(uint64_t a,int coverage){
	COVERAGE_TYPE max=0;
	max=max-1;// underflow.
	if(a==m_lowerKey){
		if(m_coverage_lower==max){ // maximum value for unsigned char.
			return;
		}

		m_coverage_lower=coverage;
	}
}

int Vertex::getCoverage(uint64_t a){
	return m_coverage_lower;
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
	return _getIngoingEdges(a,invertEdges(m_edges_lower),k);
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
	return _getOutgoingEdges(a,invertEdges(m_edges_lower),k);
	
	#endif
}

#ifndef USE_DISTANT_SEGMENTS_GRAPH
void Vertex::addIngoingEdge_ClassicMethod(uint64_t vertex,uint64_t a,int k){
	uint8_t s1First=getFirstSegmentFirstCode(a,k,_SEGMENT_LENGTH);
	// add s1First to edges.
	uint8_t newBits=(1<<(s1First));
	if(vertex==m_lowerKey){
		m_edges_lower=m_edges_lower|newBits;
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
	}
}

#endif

void Vertex::deleteOutgoingEdge(uint64_t vertex,uint64_t a,int k){
	uint8_t s2Last=getSecondSegmentLastCode(a,k,_SEGMENT_LENGTH);
	uint64_t newBits=1<<(4+s2Last);
	newBits=~newBits;
	if(vertex==m_lowerKey){
		m_edges_lower=m_edges_lower&newBits;
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
	e->setNext(m_readsStartingHere);
	m_readsStartingHere=e;
}

void Vertex::addDirection(uint64_t vertex,Direction*e){
	e->setNext(m_directions);
	m_directions=e;
}

ReadAnnotation*Vertex::getReads(uint64_t vertex){
	return m_readsStartingHere;
}

vector<Direction> Vertex::getDirections(uint64_t vertex){
	bool seekLower=false;
	if(vertex==m_lowerKey){
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

void Vertex::clearDirections(uint64_t a){
	m_directions=NULL;
}

uint8_t Vertex::getEdges(uint64_t a){
	if(a==m_lowerKey){
		return m_edges_lower;
	}
	return invertEdges(m_edges_lower);
}


