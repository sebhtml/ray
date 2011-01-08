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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _DepthFirstSearchData
#define _DepthFirstSearchData

#include<vector>
#include<MyStack.h>
#include<map>
#include<set>
#include<Parameters.h>
#include<RingAllocator.h>
#include<StaticVector.h>
#include<common_functions.h>
using namespace std;

class DepthFirstSearchData{
	bool m_outgoingEdgesDone;

	map<uint64_t,vector<uint64_t> > m_outgoingEdges;
	map<uint64_t,vector<uint64_t> > m_ingoingEdges;

public:

	bool m_maxDepthReached;
	bool m_doChoice_tips_dfs_initiated;

	// depth first search
	bool m_doChoice_tips_Initiated;
	bool m_doChoice_tips_dfs_done;

	int m_depthFirstSearch_maxDepth;
	MyStack<int> m_depthFirstSearchDepths;
	int m_doChoice_tips_i;
	vector<int> m_doChoice_tips_newEdges;
	set<uint64_t> m_depthFirstSearchVisitedVertices;
	MyStack<uint64_t> m_depthFirstSearchVerticesToVisit;
	vector<uint64_t> m_depthFirstSearchVisitedVertices_vector;
	vector<int> m_depthFirstSearchVisitedVertices_depths;
	map<uint64_t,int> m_coverages;


	void depthFirstSearch(uint64_t root,uint64_t a,int maxDepth,
	bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,vector<uint64_t>*receivedOutgoingEdges,
		int minimumCoverage,bool*edgesReceived);

	void depthFirstSearchBidirectional(uint64_t a,int maxDepth,
	bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,vector<uint64_t>*receivedOutgoingEdges,
		int minimumCoverage,bool*edgesReceived,Parameters*parameters);

	
	map<uint64_t,vector<uint64_t> >*getIngoingEdges();
	map<uint64_t,vector<uint64_t> >*getOutgoingEdges();


};

#endif
