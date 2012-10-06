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

#ifndef _Vertex
#define _Vertex

#include <plugin_SequencesIndexer/ReadAnnotation.h>
#include <plugin_SeedExtender/Direction.h>
#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <application_core/common_functions.h>
#include <plugin_Searcher/ColorSet.h>

#include <fstream>
#include <stdint.h>
#include <vector>
using namespace std;

/**
 * The vertex is important in the algorithm.
 * A DNA sequence is simply an ordered array of vertices. 
 * Two consecutive 
 * vertices always respect the de Bruijn property.
 * A Vertex actually stores two k-mers: only the lower is stored.
 * This halves the memory usage.
 * It is also required when using the probabilistic error
 * module implemented in the Bloom filter for 
 * coherency.
 *
 * \author Sébastien Boisvert
 * \date 2012-08-29 now using only one bitmap for the edges of 
 * both k-mers of the pair.
 */
class Vertex{
	
/**
 * This is for the colored graph subsystem.
 * Right now, this subsystem stores only the origins
 * of a k-mer, not an array of coverage depths.
 * Therefore, this attribute is not required
 * as the number of references is tracked elsewhere
 * anyway.
 * TODO: remove m_color because the references are tracked elsewhere too 
 */
	VirtualKmerColorHandle m_color;


/**
 *	the ingoing and outgoing edges.
 *
 *      A=0x0,C=0x1,G=0x2,T=0x3
 *     
 *	bits 0-3: ingoing edges
 *	bits 4-7: outgoing edges
 *
 *	7 6 5 4 3 2 1 0
 *
 *
 *      T G C A T G C A
 *
 *      Example:
 *
 *       parents    main k-mer       children
 *
 *       GATGA --->   ATGAC  --->  TGACT
 *                           --->  TGACA
 *
 *      In this case, the bitmap is:
 *
 *      children   parents
 *      |--------|-------|
 *      |7 6 5 4 |3 2 1 0| bit index
 *      |T G C A |T G C A| nucleotide
 *      |--------|-------|
 *     < 1 0 0 1  0 1 0 0 > bit value
 *       ................
 *
 *
 *      In the graph, there will also be this:
 *
 *       parents    main k-mer       children
 *
 *       AGTCA --->   GTCAT  ---> TCATC
 *       TGTCA --->
 *
 *      In this case, the bitmap is:
 *
 *      children   parents
 *      |--------|-------|
 *      |7 6 5 4 |3 2 1 0| bit index
 *      |T G C A |T G C A| nucleotide
 *      |--------|-------|
 *     < 0 0 1 0  1 0 0 1 > bit value
 *       ................
 *
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
 *  This is exactly 8 operations and no dynamic memory allocation.
 */
	uint8_t m_edges_lower;

/*
 * Below are the methods
 */
	void setEdges(Kmer*kmer,uint8_t edgeData);

	uint8_t convertBitmap(uint8_t bitmap);
	uint8_t swapBits(uint8_t map,int bit1,int bit2);
/*
 * TODO: move these attributes in the private or protected zone
 */

public:
	Kmer m_lowerKey;
	/*
 *	The coverage of the vertex
 */
	CoverageDepth m_coverage_lower;


/*
 * 	read annotations
 * 	which reads start here?
 */
	ReadAnnotation*m_readsStartingHere;

/*
 *	which hyperfusions go on this vertex at least once?
 */
	Direction*m_directions;

/** the greatest rank that assembled the k-mer **/
	Rank m_assembled;


	void addOutgoingEdge_ClassicMethod(Kmer*vertex,Kmer*a,int k);
	void addIngoingEdge_ClassicMethod(Kmer*vertex,Kmer*a,int k);

	void constructor();
	void setCoverage(Kmer*a,CoverageDepth coverage);
	CoverageDepth getCoverage(Kmer*p);
	void addOutgoingEdge(Kmer*vertex,Kmer*a,int k);
	void addIngoingEdge(Kmer*vertex,Kmer*a,int k);

/*
 * TODO, the vector should be a out parameter
 */
	vector<Kmer> getIngoingEdges(Kmer*a,int k);

/*
 * TODO, the vector should be a out parameter
 */
	vector<Kmer> getOutgoingEdges(Kmer*a,int k);

	uint8_t getEdges(Kmer*a);
	void deleteIngoingEdge(Kmer*vertex,Kmer*a,int k);
	void deleteOutgoingEdge(Kmer*vertex,Kmer*a,int k);


	void addRead(Kmer*a,ReadAnnotation*e);
	ReadAnnotation*getReads(Kmer*a);
	void addDirection(Kmer*a,Direction*d);
	vector<Direction> getDirections(Kmer*a);
	void clearDirections(Kmer*a);

	void assemble(Rank origin);
	bool isAssembled();
	bool isAssembledByGreaterRank(Rank origin);

	void write(Kmer*key,ofstream*f,int kmerLength);
	void writeAnnotations(Kmer*key,ofstream*f,int kmerLength,bool color);

	VirtualKmerColorHandle getVirtualColor();

	void setVirtualColor(VirtualKmerColorHandle handle);

	Kmer getKey();
	void setKey(Kmer key);

} ATTRIBUTE_PACKED;

#endif /* _Vertex */
