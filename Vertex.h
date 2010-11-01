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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/



#ifndef _Vertex

#define _Vertex
#include<ReadAnnotation.h>
#include<MyAllocator.h>
#include<Direction.h>
#include<VertexLinkedList.h>
#include<stdint.h>
#include<common_functions.h>
#include<vector>
using namespace std;



/*
 * the vertex is important in the algorithm
 * a DNA sequence is simply an ordered array of vertices. Two consecutive 
 * vertices always respect the de Bruijn property.
 */
class Vertex{
	/*
 *	The coverage of the vertex
 */
	COVERAGE_TYPE m_coverage;

	/*
 *	the ingoing and outgoing edges.
 */
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0
	
	#ifdef USE_DISTANT_SEGMENTS_GRAPH
	VertexLinkedList*m_ingoingEdges;
	VertexLinkedList*m_outgoingEdges;
	#else
	uint8_t m_edges;
	#endif

	/*
 * 	read annotations
 * 	which reads start here?
 */
	ReadAnnotation*m_readsStartingHere;

/*
 *	which hyperfusions go on this vertex at least once?
 */
	Direction*m_direction;
	#ifndef USE_DISTANT_SEGMENTS_GRAPH
	void addOutgoingEdge_ClassicMethod(VERTEX_TYPE a,int k);
	void addIngoingEdge_ClassicMethod(VERTEX_TYPE a,int k);
	#endif
public:
	void constructor();
	void setCoverage(int coverage);
	int getCoverage();
	void addOutgoingEdge(VERTEX_TYPE a,int k,MyAllocator*m);
	void addIngoingEdge(VERTEX_TYPE a,int k,MyAllocator*m);
	void addRead(int rank,int i,char c,MyAllocator*allocator);
	bool isAssembled();
	void assemble();
	vector<VERTEX_TYPE> getIngoingEdges(VERTEX_TYPE a,int k);
	vector<VERTEX_TYPE> getOutgoingEdges(VERTEX_TYPE a,int k);
	ReadAnnotation*getReads();
	void addDirection(int wave,int progression,MyAllocator*a);
	vector<Direction> getDirections();
	void clearDirections();
};

#endif
