/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _SeedingData
#define _SeedingData

class SeedExtender;

#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <communication/VirtualCommunicator.h>
#include <plugin_SeedExtender/SeedExtender.h>
#include <structures/SplayTreeIterator.h>
#include <structures/SplayNode.h>
#include <plugin_VerticesExtractor/Vertex.h>
#include <plugin_SeedingData/AssemblySeed.h>
#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <application_core/common_functions.h>
#include <plugin_SeedingData/SeedWorker.h>
#include <core/ComputeCore.h>

#include <set>
using namespace std;



/*
 * Computes seeds in the k-mer graph.
 * The computation is not done actually by
 * this class. It is a pool of workers that does the job.
 * These workers push messages on the virtual
 * communicator and the later groups messages.
 * \author Sébastien Boisvert
 */
class SeedingData : public CorePlugin{

/**
 * the minimum seed coverage allowed
 */
	
	CoverageDepth m_minimumSeedCoverageDepth;

	MessageTag RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS;
	MessageTag RAY_MPI_TAG_SEEDING_IS_OVER;
	MessageTag RAY_MPI_TAG_SEND_SEED_LENGTHS;
	MessageTag RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY;

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;

	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;
	SlaveMode RAY_SLAVE_MODE_START_SEEDING;
	SlaveMode RAY_SLAVE_MODE_SEND_SEED_LENGTHS;



	/** checkpointing */
	bool m_checkedCheckpoint;

	map<int,int>::iterator m_iterator;

	bool m_flushAllMode;
	VirtualCommunicator*m_virtualCommunicator;
	bool m_initiatedIterator;
	Rank m_rank;
	int m_size;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	int*m_seedCoverage;
	int*m_mode;
	set<WorkerHandle> m_activeWorkers;
	set<WorkerHandle>::iterator m_activeWorkerIterator;
	map<WorkerHandle,SeedWorker> m_aliveWorkers;
	bool m_communicatorWasTriggered;
	vector<WorkerHandle> m_workersDone;
	vector<WorkerHandle> m_waitingWorkers;
	vector<WorkerHandle> m_activeWorkersToRestore;
	Parameters*m_parameters;
	GridTable*m_subgraph;
	GridTableIterator m_splayTreeIterator;
	int m_wordSize;
	int m_completedJobs;
	int m_maximumAliveWorkers;
	int m_maximumWorkers;
	time_t m_last;

	void loadCheckpoint();
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
	LargeIndex m_SEEDING_i;

	int m_SEEDING_outgoing_index;
	bool m_SEEDING_outgoing_choice_done;
	int m_SEEDING_currentRank;
	vector<AssemblySeed> m_SEEDING_seeds;
	vector<int> m_SEEDING_outgoingCoverages;
	vector<uint64_t> m_SEEDING_outgoingKeys;
	bool m_SEEDING_vertexKeyAndCoverageRequested;
	int m_SEEDING_currentParentRank;
	
	SeedExtender*m_seedExtender;
	int getRank();
	int getSize();
	void call_RAY_SLAVE_MODE_START_SEEDING();

	void constructor(SeedExtender*seedExtender,int rank,int size,StaticVector*outbox,RingAllocator*outboxAllocator,
		int*mode,Parameters*parameters,int*wordSize,GridTable*subgraph,
		StaticVector*inbox,VirtualCommunicator*vc);
	void updateStates();

	void call_RAY_SLAVE_MODE_SEND_SEED_LENGTHS();

	bool m_initialized;
	void writeSeedStatistics();

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif
