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

#ifndef _Vertex
#define _Vertex

#include<ReadAnnotation.h>
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

public:
	uint64_t m_lowerKey;
	/*
 *	The coverage of the vertex
 */
	COVERAGE_TYPE m_coverage_lower;

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
	uint8_t m_edges_lower;
	#endif

	/*
 * 	read annotations
 * 	which reads start here?
 */
	ReadAnnotation*m_readsStartingHere;

/*
 *	which hyperfusions go on this vertex at least once?
 */
	Direction*m_directions;

	#ifndef USE_DISTANT_SEGMENTS_GRAPH
	void addOutgoingEdge_ClassicMethod(uint64_t vertex,uint64_t a,int k);
	void addIngoingEdge_ClassicMethod(uint64_t vertex,uint64_t a,int k);
	#endif

	void constructor();
	void setCoverage(uint64_t a,int coverage);
	int getCoverage(uint64_t p);
	void addOutgoingEdge(uint64_t vertex,uint64_t a,int k);
	void addIngoingEdge(uint64_t vertex,uint64_t a,int k);
	void addRead(uint64_t a,ReadAnnotation*e);
	vector<uint64_t> getIngoingEdges(uint64_t a,int k);
	vector<uint64_t> getOutgoingEdges(uint64_t a,int k);
	uint8_t getEdges(uint64_t a);
	ReadAnnotation*getReads(uint64_t a);
	void addDirection(uint64_t a,Direction*d);
	vector<Direction> getDirections(uint64_t a);
	void clearDirections(uint64_t a);
	void deleteIngoingEdge(uint64_t vertex,uint64_t a,int k);
	void deleteOutgoingEdge(uint64_t vertex,uint64_t a,int k);
}
#ifdef __GNUC__ 
__attribute((packed)) 
#endif
;

#endif
