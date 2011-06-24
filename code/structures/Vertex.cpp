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
#include<structures/Vertex.h>
#include<cstdlib>
#include<core/common_functions.h>
#include<iostream>
using namespace std;

void Vertex::constructor(){
	m_coverage_lower=0;
	m_edges_lower=0;
	m_edges_higher=0;
	m_readsStartingHere=NULL;
	m_directions=NULL;
}

void Vertex::setCoverage(Kmer*a,int coverage){
	COVERAGE_TYPE max=0;
	max=max-1;// underflow.
	if(*a==m_lowerKey){
		if(m_coverage_lower==max){ // maximum value
			return;
		}

		m_coverage_lower=coverage;
	}
}

int Vertex::getCoverage(Kmer*a){
	return m_coverage_lower;
}

vector<Kmer> Vertex::getIngoingEdges(Kmer *a,int k){
	if((*a)==m_lowerKey){
		return _getIngoingEdges(a,m_edges_lower,k);
	}
	return _getIngoingEdges(a,m_edges_higher,k);
}

vector<Kmer> Vertex::getOutgoingEdges(Kmer*a,int k){
	if(*a==m_lowerKey){
		return _getOutgoingEdges(a,m_edges_lower,k);
	}
	return _getOutgoingEdges(a,m_edges_higher,k);
}

void Vertex::addIngoingEdge_ClassicMethod(Kmer*vertex,Kmer*a,int k){
	uint8_t s1First=getFirstSegmentFirstCode(a,k);
	// add s1First to edges.
	uint8_t newBits=(1<<(s1First));
	if(*vertex==m_lowerKey){
		m_edges_lower=m_edges_lower|newBits;
	}else{
		m_edges_higher=m_edges_higher|newBits;
	}
}

void Vertex::deleteIngoingEdge(Kmer*vertex,Kmer*a,int k){
	uint8_t s1First=getFirstSegmentFirstCode(a,k);
	// delete s1First from edges.
	uint8_t newBits=(1<<(s1First));
	newBits=~newBits;
	if(*vertex==m_lowerKey){
		m_edges_lower=m_edges_lower&newBits;
	}else{
		m_edges_higher=m_edges_higher&newBits;
	}
}

void Vertex::addIngoingEdge(Kmer*vertex,Kmer*a,int k){
	addIngoingEdge_ClassicMethod(vertex,a,k);
}

void Vertex::addOutgoingEdge_ClassicMethod(Kmer*vertex,Kmer*a,int k){
	// description of m_edges:
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0

	// put s2Last in m_edges
	uint8_t s2Last=getSecondSegmentLastCode(a,k);
	uint64_t newBits=1<<(4+s2Last);
	if(*vertex==m_lowerKey){
		m_edges_lower=m_edges_lower|newBits;
	}else{
		m_edges_higher=m_edges_higher|newBits;
	}
}

void Vertex::deleteOutgoingEdge(Kmer*vertex,Kmer*a,int k){
	uint8_t s2Last=getSecondSegmentLastCode(a,k);
	uint64_t newBits=1<<(4+s2Last);
	newBits=~newBits;
	if(*vertex==m_lowerKey){
		m_edges_lower=m_edges_lower&newBits;
	}else{
		m_edges_higher=m_edges_higher&newBits;
	}
}

void Vertex::addOutgoingEdge(Kmer*vertex,Kmer*a,int k){
	addOutgoingEdge_ClassicMethod(vertex,a,k);
}

uint8_t Vertex::getEdges(Kmer*a){
	if(*a==m_lowerKey){
		return m_edges_lower;
	}
	return m_edges_higher;
}

void Vertex::addRead(Kmer*vertex,ReadAnnotation*e){
	e->setNext(m_readsStartingHere);
	m_readsStartingHere=e;
}

void Vertex::addDirection(Kmer*vertex,Direction*e){
	e->setNext(m_directions);
	m_directions=e;
}

ReadAnnotation*Vertex::getReads(Kmer*vertex){
	return m_readsStartingHere;
}

vector<Direction> Vertex::getDirections(Kmer*vertex){
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

void Vertex::clearDirections(Kmer*a){
	m_directions=NULL;
}
