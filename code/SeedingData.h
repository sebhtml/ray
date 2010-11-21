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



#ifndef _SeedingData
#define _SeedingData

#include<SplayTreeIterator.h>
#include<SplayNode.h>
#include<Vertex.h>
#include<set>
#include<common_functions.h>
using namespace std;

class SeedingData{
public:
	SplayTreeIterator<VERTEX_TYPE,Vertex>*m_SEEDING_iterator;
	SplayNode<VERTEX_TYPE,Vertex>*m_SEEDING_node;
	bool m_SEEDING_edgesReceived;
	int m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage;
	VERTEX_TYPE m_SEEDING_currentChildVertex;
	VERTEX_TYPE m_SEEDING_currentParentVertex;
	VERTEX_TYPE m_SEEDING_receivedKey;
	bool m_SEEDING_vertexKeyAndCoverageReceived;
	vector<VERTEX_TYPE> m_SEEDING_nodes;
	VERTEX_TYPE m_SEEDING_first;
	int m_SEEDING_receivedVertexCoverage;
	bool m_SEEDING_vertexCoverageReceived;
	int m_SEEDING_currentChildRank;
	int m_SEEDING_numberOfIngoingEdgesWithSeedCoverage;
	bool m_SEEDING_vertexCoverageRequested;
	bool m_SEEDING_edge_initiated;
	bool m_SEEDING_NodeInitiated;
	bool m_SEEDING_passedCoverageTest;
	bool m_SEEDING_passedParentsTest;
	bool m_SEEDING_Extended;
	int m_SEEDING_i;
	VERTEX_TYPE m_SEEDING_currentVertex;

	bool m_SEEDING_InedgesReceived;
	bool m_SEEDING_InedgesRequested;
	int m_SEEDING_outgoing_index;
	int m_SEEDING_numberOfSeedCoverageCandidates;
	bool m_SEEDING_outgoing_choice_done;
	bool m_SEEDING_edgesRequested;
	int m_SEEDING_ingoingEdgeIndex;
	int m_SEEDING_outgoingEdgeIndex;
	int m_SEEDING_currentRank;
	vector<vector<VERTEX_TYPE> > m_SEEDING_seeds;
	vector<VERTEX_TYPE> m_SEEDING_seed;
	vector<VERTEX_TYPE> m_SEEDING_receivedIngoingEdges;
	vector<VERTEX_TYPE> m_SEEDING_receivedOutgoingEdges;
	vector<int> m_SEEDING_outgoingCoverages;
	vector<VERTEX_TYPE> m_SEEDING_outgoingKeys;
	bool m_SEEDING_vertexKeyAndCoverageRequested;
	int m_SEEDING_numberOfIngoingEdges;
	set<VERTEX_TYPE> m_SEEDING_vertices;
	int m_SEEDING_numberOfOutgoingEdges;
	bool m_SEEDING_testInitiated;
	bool m_SEEDING_1_1_test_result;
	int m_SEEDING_currentParentRank;
	bool m_SEEDING_1_1_test_done;
	bool m_SEEDING_firstVertexTestDone;
	bool m_SEEDING_firstVertexParentTestDone;	
	bool m_SEEDING_ingoingEdgesDone;
	bool m_SEEDING_outgoingEdgesDone;

};

#endif
