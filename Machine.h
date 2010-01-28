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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/




#ifndef _Machine
#define _Machine
#include<mpi.h>
#include<map>
#include<vector>
#include"Vertex.h"
#include<SplayTree.h>
#include"Message.h"
#include"SplayTreeIterator.h"
#include<set>
#include<Read.h>
#include"Parameters.h"
#include"MyAllocator.h"
using namespace std;

class Machine{
	int m_USE_MPI_Isend;
	int m_USE_MPI_Send;
	int m_Sending_Mechanism;
	int m_ticks;
	int m_maxTicks;
	bool m_watchMaxTicks;
	unsigned long long int m_receivedMessages;
	unsigned long long int m_sentMessages;
	int m_wordSize;
	int MASTER_RANK;
	int m_last_value;
	bool m_mode_send_outgoing_edges;
	int m_mode_send_edge_sequence_id;
	int m_mode_send_edge_sequence_id_position;
	int m_rank;
	int m_size;
	char m_name[255];
	int m_nameLen;
	bool m_reverseComplementEdge;
	int m_totalLetters;
	bool m_alive;
	bool m_welcomeStep;
	bool m_loadSequenceStep;
	char*m_inputFile;
	int m_sequence_ready_machines;
	bool m_messageSentForVerticesDistribution;

	// MPI TAGs
	int m_TAG_OUT_EDGE_DATA_WITH_PTR;
	int m_TAG_VERTEX_PTR_REQUEST;
	int m_TAG_VERTICES_DATA;
	int m_TAG_SEND_SEQUENCE;
	int m_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS;
	int m_TAG_START_VERTICES_DISTRIBUTION;
	int m_TAG_SEQUENCES_READY;
	int m_TAG_SHOW_VERTICES;
	int m_TAG_VERTICES_DISTRIBUTED;
	int m_TAG_WELCOME;
	int m_TAG_OUT_EDGES_DATA;
	int m_TAG_IN_EDGES_DATA;
	int m_TAG_EDGES_DISTRIBUTED;
	int m_TAG_IN_EDGE_DATA_WITH_PTR;
	int m_TAG_START_EDGES_DISTRIBUTION;
	int m_TAG_START_EDGES_DISTRIBUTION_ASK;
	int m_TAG_START_EDGES_DISTRIBUTION_ANSWER;
	int m_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION;
	int m_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER;
	int m_TAG_PREPARE_COVERAGE_DISTRIBUTION;
	int m_TAG_COVERAGE_DATA;
	int m_TAG_COVERAGE_END;
	int m_TAG_SEND_COVERAGE_VALUES;
	int m_TAG_READY_TO_SEED;
	int m_TAG_START_SEEDING;
	int m_TAG_REQUEST_VERTEX_COVERAGE;
	int m_TAG_REQUEST_VERTEX_COVERAGE_REPLY;
	int m_TAG_REQUEST_VERTEX_OUTGOING_EDGES;
	int m_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY;
	int m_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE;
	int m_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY;
	int m_TAG_SEEDING_IS_OVER;
	int m_TAG_GOOD_JOB_SEE_YOU_SOON;
	int m_TAG_I_GO_NOW;
	int m_TAG_SET_WORD_SIZE;
	int m_TAG_MASTER_IS_DONE_ATTACHING_READS;
	int m_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY;
	int m_TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER;
	int m_TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY;
	
	int m_readyToSeed;
	bool m_mode_send_ingoing_edges;

	// MODE
	int m_MODE_START_SEEDING;
	int m_MODE_DO_NOTHING;
	int m_mode;
	bool m_startEdgeDistribution;
	bool m_mode_AttachSequences;

	// Counters.
	int m_numberOfMachinesReadyForEdgesDistribution;
	int m_ranksDoneAttachingReads;
	int m_numberOfMachinesReadyToSendDistribution;
	int m_vertices_sent;
	int m_numberOfRanksDoneSeeding;
	int m_numberOfRanksGone;
	map<int,uint64_t> m_distributionOfCoverage;

	int m_machineRank;

	// SEEDING
	SplayTreeIterator<uint64_t,Vertex>*m_SEEDING_iterator;
	SplayNode<uint64_t,Vertex>*m_SEEDING_node;
	bool m_SEEDING_edgesReceived;
	uint64_t m_SEEDING_receivedKey;
	bool m_SEEDING_vertexKeyAndCoverageReceived;
	int m_SEEDING_receivedVertexCoverage;
	bool m_SEEDING_vertexCoverageReceived;
	int m_SEEDING_numberOfIngoingEdgesWithSeedCoverage;
	Edge*m_SEEDING_edge;
	bool m_SEEDING_vertexCoverageRequested;
	bool m_SEEDING_edge_initiated;
	bool m_SEEDING_NodeInitiated;
	bool m_SEEDING_passedCoverageTest;
	bool m_SEEDING_passedParentsTest;
	bool m_SEEDING_Extended;
	int m_SEEDING_i;
	uint64_t m_SEEDING_currentVertex;
	vector<uint64_t> m_SEEDING_seed;
	
	bool m_SEEDING_edgesRequested;
	int m_SEEDING_currentRank;
	void*m_SEEDING_currentPointer;
	vector<int> m_SEEDING_outgoingRanks;
	vector<int> m_SEEDING_outgoingCoverages;
	vector<uint64_t> m_SEEDING_outgoingKeys;
	vector<void*>m_SEEDING_outgoingPointers;
	bool m_SEEDING_vertexKeyAndCoverageRequested;
	int m_SEEDING_numberOfIngoingEdges;
	set<uint64_t> m_SEEDING_vertices;

	int m_mode_send_coverage_iterator;
	vector<Message> m_outbox;
	vector<Message> m_inbox;

	map<int,uint64_t> m_coverageDistribution;
	int m_minimumCoverage;
	int m_peakCoverage;
	int m_seedCoverage;
	int m_numberOfMachinesDoneSendingCoverage;
	
	string m_VERSION;
	bool m_mode_sendDistribution;

	uint64_t m_send_buffer[10];
	set<uint64_t> m_test;
	vector<uint64_t> m_test2;
	vector<MPI_Request> m_pendingMpiRequest;
	vector<int> m_requestIterations;
	Parameters m_parameters;
	int m_numberOfMachinesDoneSendingEdges;
	SplayTree<uint64_t,Vertex> m_subgraph;

	bool m_reverseComplementVertex;
	int m_distribution_currentSequenceId;
	int m_distribution_file_id;
	int m_distribution_sequence_id;


	MyAllocator m_outboxAllocator;
	MyAllocator m_inboxAllocator;

	MyAllocator m_seedingAllocator;
	MyAllocator m_distributionAllocator;
	MyAllocator m_persistentAllocator;
	vector<Read*> m_distribution_reads;


	vector<Read*> m_myReads;

	bool m_mode_send_vertices;
	int m_mode_send_vertices_sequence_id;
	int m_mode_send_vertices_sequence_id_position;
	int m_numberOfMachinesDoneSendingVertices;

	bool m_aborted;

	void killRanks();
	void attachReads();
	void printStatus();
	int getSize();
	bool isAlive();
	void run();
	bool isMaster();
	void checkRequests();
	void receiveMessages();
	void loadSequences();
	void processMessages();
	void processMessage(Message*message);
	void sendMessages();
	void processData();
	int getRank();
	void receiveWelcomeMessage(MPI_Status*status);
	int vertexRank(uint64_t a);
public:
	Machine(int argc,char**argv);
};


#endif

