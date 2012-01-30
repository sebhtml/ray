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

#ifndef _DepthFirstSearchData
#define _DepthFirstSearchData

#include <vector>
#include <structures/MyStack.h>
#include <map>
#include <plugin_SeedingData/SeedingData.h>
#include <set>
#include <application_core/Parameters.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <application_core/common_functions.h>
using namespace std;

/*
 * Data for depth first search.
 *
 * \author Sébastien Boisvert
 */
class DepthFirstSearchData{

	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_EDGES;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES;

	bool m_outgoingEdgesDone;

	map<Kmer,vector<Kmer> > m_outgoingEdges;
	map<Kmer,vector<Kmer> > m_ingoingEdges;

public:
	void setTags(
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_EDGES,
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES
);

	bool m_maxDepthReached;
	bool m_doChoice_tips_dfs_initiated;

	// depth first search
	bool m_doChoice_tips_Initiated;
	bool m_doChoice_tips_dfs_done;

	int m_depthFirstSearch_maxDepth;
	MyStack<int> m_depthFirstSearchDepths;
	int m_doChoice_tips_i;
	vector<int> m_doChoice_tips_newEdges;
	set<Kmer> m_depthFirstSearchVisitedVertices;
	MyStack<Kmer> m_depthFirstSearchVerticesToVisit;
	vector<Kmer> m_depthFirstSearchVisitedVertices_vector;
	vector<int> m_depthFirstSearchVisitedVertices_depths;
	map<Kmer,int> m_coverages;


	void depthFirstSearch(Kmer root,Kmer a,int maxDepth,
	bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,vector<Kmer>*receivedOutgoingEdges,
		int minimumCoverage,bool*edgesReceived,int wordSize,Parameters*parameters);

	void depthFirstSearchBidirectional(Kmer a,int maxDepth,
	bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,SeedingData*seedingData,
		int minimumCoverage,bool*edgesReceived,Parameters*parameters);

	
	map<Kmer,vector<Kmer> >*getIngoingEdges();
	map<Kmer,vector<Kmer> >*getOutgoingEdges();


};

#endif
