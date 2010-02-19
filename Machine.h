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

#ifdef SHOW_STATISTICS
#include<time.h>
#endif

#include<mpi.h>
#include<map>
#include<vector>
#include<Vertex.h>
#include<SplayTree.h>
#include<Message.h>
#include<SplayTreeIterator.h>
#include<set>
#include<Read.h>
#include<Parameters.h>
#include<MyAllocator.h>

// tags
#define TAG_WELCOME 0
#define TAG_SEND_SEQUENCE 1
#define TAG_SEQUENCES_READY 2
#define TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS 3
#define TAG_VERTICES_DATA 4
#define TAG_VERTICES_DISTRIBUTED 5
#define TAG_VERTEX_PTR_REQUEST 6
#define TAG_OUT_EDGE_DATA_WITH_PTR 7
#define TAG_OUT_EDGES_DATA 8
#define TAG_SHOW_VERTICES 9
#define TAG_START_VERTICES_DISTRIBUTION 10
#define TAG_EDGES_DISTRIBUTED 11
#define TAG_IN_EDGES_DATA 12
#define TAG_IN_EDGE_DATA_WITH_PTR 13
#define TAG_START_EDGES_DISTRIBUTION 14
#define TAG_START_EDGES_DISTRIBUTION_ASK 15
#define TAG_START_EDGES_DISTRIBUTION_ANSWER 16
#define TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION 17
#define TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER 18
#define TAG_PREPARE_COVERAGE_DISTRIBUTION 19
#define TAG_COVERAGE_DATA 20
#define TAG_COVERAGE_END 21
#define TAG_SEND_COVERAGE_VALUES 22
#define TAG_READY_TO_SEED 23
#define TAG_START_SEEDING 24
#define TAG_REQUEST_VERTEX_COVERAGE 25
#define TAG_REQUEST_VERTEX_COVERAGE_REPLY 26
#define TAG_REQUEST_VERTEX_KEY_AND_COVERAGE 28
#define TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY 30
#define TAG_REQUEST_VERTEX_OUTGOING_EDGES 31
#define TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY 32
#define TAG_SEEDING_IS_OVER 33
#define TAG_GOOD_JOB_SEE_YOU_SOON 34
#define TAG_I_GO_NOW 35
#define TAG_SET_WORD_SIZE 36
#define TAG_MASTER_IS_DONE_ATTACHING_READS 37
#define TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY 38
#define TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER 39
#define TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY 40
#define TAG_REQUEST_VERTEX_INGOING_EDGES 41
#define TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY 42
#define TAG_EXTENSION_IS_DONE 43
#define TAG_ASK_EXTENSION 44
#define TAG_ASK_IS_ASSEMBLED 45
#define TAG_ASK_REVERSE_COMPLEMENT 46
#define TAG_REQUEST_VERTEX_POINTER 47
#define TAG_ASK_IS_ASSEMBLED_REPLY 48
#define TAG_MARK_AS_ASSEMBLED 49
#define TAG_ASK_EXTENSION_DATA 50
#define TAG_EXTENSION_DATA 51
#define TAG_EXTENSION_END 52
#define TAG_EXTENSION_DATA_END 53
#define TAG_ATTACH_SEQUENCE 54
#define TAG_REQUEST_READS 55
#define TAG_REQUEST_READS_REPLY 56
#define TAG_ASK_READ_VERTEX_AT_POSITION 57
#define TAG_ASK_READ_VERTEX_AT_POSITION_REPLY 58
#define TAG_ASK_READ_LENGTH 59
#define TAG_ASK_READ_LENGTH_REPLY 60
#define TAG_SAVE_WAVE_PROGRESSION 61
#define TAG_COPY_DIRECTIONS 62
#define TAG_ASSEMBLE_WAVES 63
#define TAG_COPY_DIRECTIONS_DONE 64
#define TAG_SAVE_WAVE_PROGRESSION_REVERSE 65
#define TAG_ASSEMBLE_WAVES_DONE 66
#define TAG_START_FUSION 67
#define TAG_FUSION_DONE 68
#define TAG_ASK_VERTEX_PATHS 69
#define TAG_ASK_VERTEX_PATHS_REPLY 70
#define TAG_GET_PATH_LENGTH 71
#define TAG_GET_PATH_LENGTH_REPLY 72
#define TAG_CALIBRATION_MESSAGE 73
#define TAG_BEGIN_CALIBRATION 74
#define TAG_END_CALIBRATION 75

#define MASTER_RANK 0
#define BARRIER_PERIOD 250

#define CALIBRATION_DURATION 10 // in seconds.

// modes
#define MODE_EXTENSION_ASK 0
#define MODE_START_SEEDING 1
#define MODE_DO_NOTHING 2
#define MODE_ASK_EXTENSIONS 3
#define MODE_SEND_EXTENSION_DATA 4
#define MODE_ASSEMBLE_WAVES 5
#define MODE_COPY_DIRECTIONS 6
#define MODE_ASSEMBLE_GRAPH 7
#define MODE_FUSION 8
#define MODE_MASTER_ASK_CALIBRATION 9
#define MODE_PERFORM_CALIBRATION 10

#define OUTBOX_ALLOCATOR_CHUNK_SIZE 10*1024*1024 // 10 MB
#define DISTRIBUTION_ALLOCATOR_CHUNK_SIZE 10*1024*1024 // 10 MB
#define INBOX_ALLOCATOR_CHUNK_SIZE 10*1024*1024 // 10 MB
#define PERSISTENT_ALLOCATOR_CHUNK_SIZE 10*1024*1024 // 10 MB

using namespace std;

class Machine{
	int m_USE_MPI_Isend;
	int m_USE_MPI_Send;
	int m_Sending_Mechanism;
	int m_ticks;
	int m_maxTicks;
	bool m_watchMaxTicks;
	int m_wordSize;
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

	// speed calibration to make OpenMPI handle the communication in its shared memory.
	int m_calibrationStart;
	bool m_speedLimitIsOn;
	int m_calibration_numberOfMessagesSent;
	bool m_calibrationAskedCalibration;
	int m_calibration_MaxSpeed;
	bool m_calibrationIsDone;
	int*m_messagesSent;

	int m_readyToSeed;
	bool m_showMessages;
	bool m_mode_send_ingoing_edges;

	int m_mode;
	int m_master_mode;

	// TODO: merge all m_mode_* variables with m_mode and m_master_mode.
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
	int m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage;
	uint64_t m_SEEDING_currentChildVertex;
	uint64_t m_SEEDING_currentParentVertex;
	uint64_t m_SEEDING_receivedKey;
	bool m_SEEDING_vertexKeyAndCoverageReceived;
	vector<uint64_t> m_SEEDING_nodes;
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


	int m_mode_send_coverage_iterator;
	vector<Message> m_outbox;
	vector<Message> m_inbox;



	// EXTENSION MODE
	vector<int> m_EXTENSION_identifiers;
	set<int> m_FUSION_eliminated;
	vector<uint64_t> m_EXTENSION_extension;
	vector<int> m_EXTENSION_coverages;
	bool m_EXTENSION_complementedSeed;
	vector<uint64_t> m_EXTENSION_currentSeed;
	int m_EXTENSION_numberOfRanksDone;
	vector<vector<uint64_t> > m_EXTENSION_contigs;
	bool m_EXTENSION_checkedIfCurrentVertexIsAssembled;
	bool m_EXTENSION_VertexMarkAssembled_requested;
	bool m_EXTENSION_reverseComplement_requested;
	bool m_EXTENSION_vertexIsAssembledResult;
	set<ReadAnnotation,ReadAnnotationComparator>::iterator m_EXTENSION_readIterator;
	bool m_EXTENSION_readLength_requested;
	bool m_EXTENSION_readLength_received;
	bool m_EXTENSION_readLength_done;
	bool m_EXTENSION_read_vertex_received;
	bool m_EXTENSION_read_vertex_requested;
	uint64_t m_EXTENSION_receivedReadVertex;
	bool m_mode_EXTENSION;
	bool m_EXTENSION_currentRankIsDone;
	bool m_EXTENSION_currentRankIsSet;
	bool m_EXTENSION_currentRankIsStarted;
	int m_EXTENSION_rank;
	bool m_EXTENSION_initiated;
	int m_EXTENSION_currentSeedIndex;
	bool m_EXTENSION_VertexAssembled_received;
	int m_EXTENSION_currentPosition;
	bool m_EXTENSION_VertexMarkAssembled_received;
	bool m_EXTENSION_markedCurrentVertexAsAssembled;
	bool m_EXTENSION_enumerateChoices;
	bool m_EXTENSION_choose;
	bool m_EXTENSION_directVertexDone;
	bool m_EXTENSION_VertexAssembled_requested;
	bool m_EXTENSION_receivedAssembled;
	bool m_EXTENSION_reverseComplement_received;
	vector<ReadAnnotation> m_EXTENSION_receivedReads;
	bool m_EXTENSION_reads_requested;
	bool m_EXTENSION_reads_received;
	vector<ReadAnnotation> m_EXTENSION_readsOutOfRange;
	int m_EXTENSION_receivedLength;
	bool m_EXTENSION_reverseVertexDone;
	// reads used so far
	set<int> m_EXTENSION_usedReads;
	// reads to check (the ones "in range")
	set<ReadAnnotation,ReadAnnotationComparator> m_EXTENSION_readsInRange;
	bool m_EXTENSION_singleEndResolution;
	map<int,vector<int> > m_EXTENSION_readPositionsForVertices;
	map<int,int> m_EXTENSION_reads_startingPositionOnContig;

	
	// COPY Directions.
	int m_COPY_ranks;

	// FUSION
	bool m_FUSION_direct_fusionDone;
	bool m_FUSION_first_done;
	int m_FUSION_numberOfRanksDone;
	bool m_FUSION_last_done;
	bool m_FUSION_paths_requested;
	bool m_FUSION_paths_received;
	vector<Direction> m_FUSION_firstPaths;
	int m_FUSION_receivedLength;
	bool m_FUSION_reverse_fusionDone;
	vector<Direction> m_FUSION_lastPaths;
	vector<int> m_FUSION_matches;
	bool m_FUSION_matches_done;
	bool m_FUSION_pathLengthReceived;
	bool m_FUSION_matches_length_done;
	int m_FUSION_match_index;
	bool m_FUSION_pathLengthRequested;
	vector<Direction> m_FUSION_receivedPaths;
	// coverage distribubtion
	map<int,uint64_t> m_coverageDistribution;
	int m_minimumCoverage;
	int m_peakCoverage;
	int m_seedCoverage;
	int m_numberOfMachinesDoneSendingCoverage;
	
	string m_VERSION;
	bool m_mode_sendDistribution;

	vector<MPI_Request> m_pendingMpiRequest;
	Parameters m_parameters;
	int m_numberOfMachinesDoneSendingEdges;
	SplayTree<uint64_t,Vertex> m_subgraph;

	bool m_reverseComplementVertex;
	int m_distribution_currentSequenceId;
	int m_distribution_file_id;
	int m_distribution_sequence_id;


	MyAllocator m_outboxAllocator;
	MyAllocator m_inboxAllocator;
	MyAllocator m_distributionAllocator;
	MyAllocator m_persistentAllocator;
	vector<Read*>m_distribution_reads;
	

	vector<Read*> m_myReads;

	bool m_mode_send_vertices;
	int m_mode_send_vertices_sequence_id;
	int m_mode_send_vertices_sequence_id_position;
	int m_numberOfMachinesDoneSendingVertices;

	vector<vector<uint64_t> > m_allPaths;
	bool m_aborted;

	#ifdef SHOW_STATISTICS
	map<int,int> m_statistics;
	#endif

	time_t m_lastTimeStamp;
	int m_numberOfBarriers;
	void enumerateChoices();
	void killRanks();
	void attachReads();
	void printStatus();
	void doChoice();
	void checkIfCurrentVertexIsAssembled();
	void markCurrentVertexAsAssembled();
	int getSize();
	bool isAlive();
	void run();
	void do_1_1_test();
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
	void extendSeeds();
public:
	Machine(int argc,char**argv);
};


#endif

