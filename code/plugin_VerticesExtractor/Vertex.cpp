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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include<assert.h>
#include<vector>
#include<plugin_VerticesExtractor/Vertex.h>
#include<cstdlib>
#include<application_core/common_functions.h>
#include<iostream>
using namespace std;

#define __NO_ORIGIN -999

void Vertex::constructor(){
	m_coverage_lower=0;
	m_edges_lower=0;
	m_readsStartingHere=NULL;
	m_directions=NULL;
	m_assembled=__NO_ORIGIN;

	m_color=0;
}

void Vertex::assemble(Rank origin){
	if(origin>m_assembled)
		m_assembled=origin;

	#ifdef ASSERT
	assert(m_assembled!=__NO_ORIGIN);
	#endif
}

bool Vertex::isAssembled(){
	return m_assembled!=__NO_ORIGIN;
}

bool Vertex::isAssembledByGreaterRank(Rank origin){
	return origin<m_assembled;
}

void Vertex::setCoverage(Kmer*a,CoverageDepth coverage){
	if(*a==m_lowerKey){

		CoverageDepth max=0;
		max=max-1;// underflow.

		if(m_coverage_lower==max){ // maximum value
			return;
		}

		m_coverage_lower=coverage;
	}
}

CoverageDepth Vertex::getCoverage(Kmer*a){
	return m_coverage_lower;
}

vector<Kmer> Vertex::getIngoingEdges(Kmer *a,int k){
	return a->_getIngoingEdges(getEdges(a),k);
}

vector<Kmer> Vertex::getOutgoingEdges(Kmer*a,int k){
	return a->_getOutgoingEdges(getEdges(a),k);
}

void Vertex::addIngoingEdge_ClassicMethod(Kmer*vertex,Kmer*a,int k){
	uint8_t s1First=a->getFirstSegmentFirstCode(k);
	// add s1First to edges.
	uint8_t newBits=(1<<(s1First));

	setEdges(vertex,getEdges(vertex)|newBits);
}

void Vertex::deleteIngoingEdge(Kmer*vertex,Kmer*a,int k){
	uint8_t s1First=a->getFirstSegmentFirstCode(k);
	// delete s1First from edges.
	uint8_t newBits=(1<<(s1First));
	newBits=~newBits;

	setEdges(vertex,getEdges(vertex)&newBits);
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
	uint8_t s2Last=a->getSecondSegmentLastCode(k);
	uint64_t newBits=1<<(4+s2Last);

	setEdges(vertex,getEdges(vertex)|newBits);
}

void Vertex::deleteOutgoingEdge(Kmer*vertex,Kmer*a,int k){
	uint8_t s2Last=a->getSecondSegmentLastCode(k);
	uint64_t newBits=1<<(4+s2Last);
	newBits=~newBits;

	setEdges(vertex,getEdges(vertex)&newBits);
}

void Vertex::addOutgoingEdge(Kmer*vertex,Kmer*a,int k){
	addOutgoingEdge_ClassicMethod(vertex,a,k);
}

void Vertex::setEdges(Kmer*a,uint8_t edges){
	if(*a==m_lowerKey)
		m_edges_lower=edges;
	else
		m_edges_lower=convertBitmap(edges);
}

uint8_t Vertex::getEdges(Kmer*a){
	if(*a==m_lowerKey)
		return m_edges_lower;
	return convertBitmap(m_edges_lower);
}

void Vertex::addRead(Kmer*vertex,ReadAnnotation*e){
	e->setNext(m_readsStartingHere);
	m_readsStartingHere=e;
}

void Vertex::addDirection(Kmer*vertex,Direction*e){
	#ifdef ASSERT
	Vertex copy0=*this;

	if(m_directions != NULL){
		Direction copy1=*m_directions;
		Direction*next=copy1.getNext();
		assert(next ==NULL || next != NULL);
	}

	assert(e!=NULL);
	Direction copy2=*e;
	assert(copy2.getNext() == NULL);
	#endif

	e->setNext(m_directions);
	m_directions=e;

	#ifdef ASSERT
	assert(m_directions != NULL);
	#endif
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

	if(e==NULL)
		return a;

	//#define DEBUG_getDirections

	#ifdef DEBUG_getDirections
	cout<<"DEBUG_getDirections Initial Pointer= "<<e<<endl;
	#endif

	while(e!=NULL){
		#ifdef ASSERT
		assert(e!=NULL);
		#endif

		if(e->isLower()==seekLower){
			a.push_back(*e);
		}

		// #define DEBUG_bug_20110921

		#ifdef DEBUG_bug_20110921
		if(e->getProgression() == 0){
			cout<<"Direction "<<e<<" path "<<e->getWave()<<" position "<<e->getProgression()<<" lower "<<e->isLower()<<" next "<<e->getNext()<<endl;
		}
		#endif

		e=e->getNext();
	}

	#ifdef ASSERT
	assert(e==NULL);
	#endif

	return a;
}

void Vertex::clearDirections(Kmer*a){
	m_directions=NULL;
}

void Vertex::write(Kmer*key,ofstream*f,int kmerLength){
	int coverage=getCoverage(key);
	key->write(f);
	f->write((char*)&coverage,sizeof(int));
	vector<Kmer> parents=getIngoingEdges(key,kmerLength);
	vector<Kmer> children=getOutgoingEdges(key,kmerLength);
	int numberOfParents=parents.size();;
	f->write((char*)&numberOfParents,sizeof(int));
	for(int i=0;i<(int)parents.size();i++){
		parents[i].write(f);
	}
	int numberOfChildren=children.size();
	f->write((char*)&numberOfChildren,sizeof(int));
	for(int i=0;i<(int)children.size();i++){
		children[i].write(f);
	}
}

void Vertex::writeAnnotations(Kmer*key,ofstream*f,int kmerLength,bool color){
	key->write(f);

	Kmer complement=key->complementVertex(kmerLength,color);
	bool isLower=(*key)<complement;

	int annotations=0;
	ReadAnnotation*ptr=m_readsStartingHere;
	while(ptr!=NULL){
		if(ptr->isLower()==isLower){
			annotations++;
		}
		ptr=ptr->getNext();
	}

	ptr=m_readsStartingHere;
	f->write((char*)&annotations,sizeof(int));
	while(ptr!=NULL){
		if(ptr->isLower()==isLower){
			ptr->write(f);
		}
		ptr=ptr->getNext();
	}
}

VirtualKmerColorHandle Vertex::getVirtualColor(){
	return m_color;
}

void Vertex::setVirtualColor(VirtualKmerColorHandle handle){
	m_color=handle;
}


Kmer Vertex::getKey(){
	return m_lowerKey;
}

void Vertex::setKey(Kmer key){
	m_lowerKey=key;
}

/*
 *   The algorithm to convert these maps:
 *
 * [Swap the 4 bits for children with the 4 bits for parents]
 *   swap 7 and 3
 *   swap 6 and 2
 *   swap 5 and 1
 *   swap 4 and 0
 * [Swap nucleotides]
 *   swap 7 and 4
 *   swap 6 and 5
 *   swap 3 and 0
 *   swap 2 and 1
 *  
 *      children   parents
 *      |--------|-------|
 *      |7 6 5 4 |3 2 1 0| bit index
 *      |T G C A |T G C A| nucleotide
 *      |--------|-------|
 *     < 0 0 1 0  1 0 0 1 > bit value
 *       ................
*
 *  see also the .h that has more documentation for this.
 */
uint8_t Vertex::convertBitmap(uint8_t bitMap){

/*
 * [Swap the 4 bits for children with the 4 bits for parents]
 * This is equivalent to swapping bits as described
 * above.
 */
	uint8_t newBits4_7=bitMap<<4;
	uint8_t newBits0_3=bitMap>>4;
	uint8_t baseMap=newBits4_7|newBits0_3;

/*
 * Swap nucleotides
 */
	baseMap=swapBits(baseMap,7,4);
	baseMap=swapBits(baseMap,6,5);
	baseMap=swapBits(baseMap,3,0);
	baseMap=swapBits(baseMap,2,1);

	return baseMap;
}

uint8_t Vertex::swapBits(uint8_t map,int bit1,int bit2){
	int bit1Value=((uint64_t)map<<(63-bit1))>>63;
	int bit2Value=((uint64_t)map<<(63-bit2))>>63;

	#ifdef ASSERT
	assert(bit1>=0);
	assert(bit1<8);
	assert(bit2>=0);
	assert(bit2<8);
	#endif

/*
 * The bit are the same, no need to swap anything.
 */
	if(bit1Value==bit2Value)
		return map;

	#ifdef ASSERT
	if(bit1Value+bit2Value!=1){
		cout<<"bit values: "<<bit1Value<<" "<<bit2Value<<endl;
	}

	assert(bit1Value==0||bit2Value==0);
	assert(bit1Value==1||bit2Value==1);
	assert(bit1Value+bit2Value==1);
	#endif

	if(bit1Value==1){
		// set bit2 to 1 
		map|=(1<<bit2);
		// and bit1 to 0
		map&=~(1<<bit1);
	}else if(bit2Value==1){
		// set bit1 to 1 
		map|=(1<<bit1);
		// and bit2 to 0
		map&=~(1<<bit2);
	}

	#ifdef ASSERT
	int newBit1Value=((uint64_t)map<<(63-bit1))>>63;
	int newBit2Value=((uint64_t)map<<(63-bit2))>>63;

	assert(newBit1Value==bit2Value);
	assert(newBit2Value==bit1Value);
	assert(newBit1Value+newBit2Value==1);
	assert(newBit1Value==0||newBit2Value==0);
	assert(newBit1Value==1||newBit2Value==1);
	#endif /* ASSERT */

	return map;
}

