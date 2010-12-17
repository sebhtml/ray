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

class SeedExtender;

#include<SeedExtender.h>
#include<SplayTreeIterator.h>
#include<SplayNode.h>
#include<Vertex.h>
#include<MyForest.h>
#include<set>
#include<common_functions.h>
using namespace std;

class SeedingData{
	bool m_initialized;
	bool m_initiatedIterator;
	bool*m_colorSpaceMode;
	int m_rank;
	int m_size;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	int*m_seedCoverage;
	int*m_mode;
	int*m_wordSize;
	Parameters*m_parameters;
	MyForest*m_subgraph;
	int m_currentTreeIndex;
	SplayTreeIterator<uint64_t,Vertex> m_splayTreeIterator;

public:
	SeedingData();
	SplayTreeIterator<uint64_t,Vertex>*m_SEEDING_iterator;
	SplayNode<uint64_t,Vertex>*m_SEEDING_node;
	bool m_SEEDING_edgesReceived;
	int m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage;
	uint64_t m_SEEDING_currentChildVertex;
	uint64_t m_SEEDING_currentParentVertex;
	uint64_t m_SEEDING_receivedKey;
	bool m_SEEDING_vertexKeyAndCoverageReceived;
	uint64_t m_SEEDING_first;
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
	uint64_t m_SEEDING_currentVertex;

	bool m_SEEDING_InedgesReceived;
	bool m_SEEDING_InedgesRequested;
	int m_SEEDING_outgoing_index;
	int m_SEEDING_numberOfSeedCoverageCandidates;
	bool m_SEEDING_outgoing_choice_done;
	bool m_SEEDING_edgesRequested;
	int m_SEEDING_ingoingEdgeIndex;
	int m_SEEDING_outgoingEdgeIndex;
	int m_SEEDING_currentRank;
	vector<vector<uint64_t> > m_SEEDING_seeds;
	vector<uint64_t> m_SEEDING_seed;
	vector<uint64_t> m_SEEDING_receivedIngoingEdges;
	vector<uint64_t> m_SEEDING_receivedOutgoingEdges;
	vector<int> m_SEEDING_outgoingCoverages;
	vector<uint64_t> m_SEEDING_outgoingKeys;
	bool m_SEEDING_vertexKeyAndCoverageRequested;
	int m_SEEDING_numberOfIngoingEdges;
	set<uint64_t> m_SEEDING_vertices;
	int m_SEEDING_numberOfOutgoingEdges;
	bool m_SEEDING_testInitiated;
	bool m_SEEDING_1_1_test_result;
	int m_SEEDING_currentParentRank;
	bool m_SEEDING_1_1_test_done;
	bool m_SEEDING_firstVertexTestDone;
	bool m_SEEDING_firstVertexParentTestDone;	
	bool m_SEEDING_ingoingEdgesDone;
	bool m_SEEDING_outgoingEdgesDone;
	
	SeedExtender*m_seedExtender;
	int getRank();
	int getSize();
	void computeSeeds();
	void do_1_1_test();

	void constructor(SeedExtender*seedExtender,int rank,int size,StaticVector*outbox,RingAllocator*outboxAllocator,
		int*seedCoverage,int*mode,Parameters*parameters,int*wordSize,MyForest*subgraph,bool*colorSpaceMode);
};

#endif
