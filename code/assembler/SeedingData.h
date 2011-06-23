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

#ifndef _SeedingData
#define _SeedingData

class SeedExtender;

#include <structures/Kmer.h>
#include <communication/VirtualCommunicator.h>
#include <assembler/SeedExtender.h>
#include <structures/SplayTreeIterator.h>
#include <structures/SplayNode.h>
#include <structures/Vertex.h>
#include <structures/MyForest.h>
#include <graph/GridTableIterator.h>
#include <core/common_functions.h>
#include <structures/MyForestIterator.h>
#include <assembler/SeedWorker.h>
#include <set>
using namespace std;

/*
 * Computes seeds in the k-mer graph.
 * The computation is not done actually by
 * this class. It is a pool of workers that does the job.
 * These workers push messages on the virtual
 * communicator and the later groups messages.
 */
class SeedingData{
	map<int,int>::iterator m_iterator;

	bool m_flushAllMode;
	VirtualCommunicator*m_virtualCommunicator;
	bool m_initiatedIterator;
	int m_rank;
	int m_size;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	int*m_seedCoverage;
	int*m_mode;
	set<uint64_t> m_activeWorkers;
	set<uint64_t>::iterator m_activeWorkerIterator;
	map<uint64_t,SeedWorker> m_aliveWorkers;
	bool m_communicatorWasTriggered;
	vector<uint64_t> m_workersDone;
	vector<uint64_t> m_waitingWorkers;
	vector<uint64_t> m_activeWorkersToRestore;
	Parameters*m_parameters;
	GridTable*m_subgraph;
	GridTableIterator m_splayTreeIterator;
	int m_wordSize;
	int m_completedJobs;
	int m_maximumAliveWorkers;
	int m_maximumWorkers;
	time_t m_last;
public:

	map<int,int> m_masterSeedLengths;
	map<int,int> m_slaveSeedLengths;
	

	bool m_SEEDING_edgesRequested;
	int m_SEEDING_ingoingEdgeIndex;
	vector<Kmer> m_SEEDING_receivedOutgoingEdges;
	Kmer m_SEEDING_currentVertex;
	int m_SEEDING_receivedVertexCoverage;
	vector<Kmer> m_SEEDING_receivedIngoingEdges;
	bool m_SEEDING_InedgesReceived;
	int m_SEEDING_outgoingEdgeIndex;
	bool m_SEEDING_vertexCoverageRequested;
	bool m_SEEDING_vertexCoverageReceived;
	SplayTreeIterator<uint64_t,Vertex>*m_SEEDING_iterator;
	SplayNode<Kmer,Vertex>*m_SEEDING_node;
	uint64_t m_SEEDING_receivedKey;
	bool m_SEEDING_vertexKeyAndCoverageReceived;
	int m_SEEDING_currentChildRank;
	bool m_SEEDING_edge_initiated;
	bool m_SEEDING_NodeInitiated;
	bool m_SEEDING_passedCoverageTest;
	bool m_SEEDING_passedParentsTest;
	bool m_SEEDING_Extended;
	bool m_SEEDING_edgesReceived;
	uint64_t m_SEEDING_i;

	int m_SEEDING_outgoing_index;
	bool m_SEEDING_outgoing_choice_done;
	int m_SEEDING_currentRank;
	vector<vector<Kmer> > m_SEEDING_seeds;
	vector<int> m_SEEDING_outgoingCoverages;
	vector<uint64_t> m_SEEDING_outgoingKeys;
	bool m_SEEDING_vertexKeyAndCoverageRequested;
	int m_SEEDING_currentParentRank;
	
	SeedExtender*m_seedExtender;
	int getRank();
	int getSize();
	void computeSeeds();

	void constructor(SeedExtender*seedExtender,int rank,int size,StaticVector*outbox,RingAllocator*outboxAllocator,
		int*mode,Parameters*parameters,int*wordSize,GridTable*subgraph,
		StaticVector*inbox,VirtualCommunicator*vc);
	void updateStates();
	void sendSeedLengths();
	bool m_initialized;
	void writeSeedStatistics();
};

#endif
