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

#include <structures/ReadAnnotation.h>
#include <structures/Direction.h>
#include <stdint.h>
#include <core/common_functions.h>
#include <vector>
using namespace std;

/*
 * the vertex is important in the algorithm
 * a DNA sequence is simply an ordered array of vertices. Two consecutive 
 * vertices always respect the de Bruijn property.
 * a Vertex actually stores two k-mers: only the lower is stored.
 * This halves the memory usage.
 */
class Vertex{

public:
	Kmer m_lowerKey;
	/*
 *	The coverage of the vertex
 */
	COVERAGE_TYPE m_coverage_lower;

	/*
 *	the ingoing and outgoing edges.
 */
	// outgoing  ingoing
	
	uint8_t m_edges_lower;
	uint8_t m_edges_higher;

	void addOutgoingEdge_ClassicMethod(Kmer*vertex,Kmer*a,int k);
	void addIngoingEdge_ClassicMethod(Kmer*vertex,Kmer*a,int k);

	void constructor();
	void setCoverage(Kmer*a,int coverage);
	int getCoverage(Kmer*p);
	void addOutgoingEdge(Kmer*vertex,Kmer*a,int k);
	void addIngoingEdge(Kmer*vertex,Kmer*a,int k);
	vector<Kmer> getIngoingEdges(Kmer*a,int k);
	vector<Kmer> getOutgoingEdges(Kmer*a,int k);
	uint8_t getEdges(Kmer*a);
	void deleteIngoingEdge(Kmer*vertex,Kmer*a,int k);
	void deleteOutgoingEdge(Kmer*vertex,Kmer*a,int k);

/*
 * 	read annotations
 * 	which reads start here?
 */
	ReadAnnotation*m_readsStartingHere;

/*
 *	which hyperfusions go on this vertex at least once?
 */
	Direction*m_directions;

	void addRead(Kmer*a,ReadAnnotation*e);
	ReadAnnotation*getReads(Kmer*a);
	void addDirection(Kmer*a,Direction*d);
	vector<Direction> getDirections(Kmer*a);
	void clearDirections(Kmer*a);

} ATTRIBUTE_PACKED;

#endif
