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


#ifndef _Machine
#define _Machine

#include<MessagesHandler.h>
#include<common_functions.h>
#include<MyForest.h>
#include<EdgesExtractor.h>
#include<ArrayOfReads.h>
#include<StaticVector.h>
#include<SeedingData.h>
#include<map>
#include<vector>
#include<RingAllocator.h>
#include<DepthFirstSearchData.h>
#include<TimePrinter.h>
#include<SequencesIndexer.h>
#include<SeedExtender.h>
#include<SequencesLoader.h>
#include<Library.h>
#include<Chooser.h>
#include<MessageProcessor.h>
#include<ChooserData.h>
#include<Vertex.h>
#include<OpenAssemblerChooser.h>
#include<SplayTree.h>
#include<BubbleData.h>
#include<Message.h>
#include<time.h>
#include<SplayTreeIterator.h>
#include<set>
#include<Read.h>
#include<Parameters.h>
#include<MyAllocator.h>
#include<mpi.h>
#include<VerticesExtractor.h>


class StatisticsData{
public:
	time_t m_time_t_statistics;
	map<int,int> m_statistics_messages;
	map<int,int> m_statistics_bytes;
};

class ScaffolderData{
public:
	bool m_computedTopology;
	int m_pathId;
	MyStack<int> m_depthsToVisit;
	MyStack<uint64_t> m_verticesToVisit;
	set<uint64_t> m_visitedVertices;
	bool m_processedLastVertex;
	map<int,int> m_allIdentifiers;
};


using namespace std;


class Machine;
typedef void (Machine::*MachineMethod) ();

class Machine{
	MachineMethod m_master_methods[32];
	MachineMethod m_slave_methods[32];

	Library m_library;
	int m_currentCycleStep;
	MessagesHandler m_messagesHandler;

	EdgesExtractor m_edgesExtractor;
	int m_numberOfRanksWithCoverageData;
	TimePrinter m_timePrinter;
	ScaffolderData*m_sd;
	VerticesExtractor m_verticesExtractor;
	MessageProcessor m_mp;
	int m_argc;
	char**m_argv;
	int m_wordSize;
	int m_last_value;
	time_t m_lastTime;
	bool m_mode_send_outgoing_edges;
	int m_rank;
	int m_size;
	int m_totalLetters;
	bool m_alive;
	bool m_welcomeStep;
	bool m_loadSequenceStep;
	char*m_inputFile;
	int m_sequence_ready_machines;
	bool m_messageSentForVerticesDistribution;

	Chooser m_c;
	SequencesIndexer m_si;
	SeedExtender m_seedExtender;

	bool m_ready;

	// clearing
	int m_CLEAR_n;
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
	int m_numberOfRanksDoneSeeding;
	int m_numberOfRanksGone;
	map<int,uint64_t> m_distributionOfCoverage;

	FusionData*m_fusionData;

	int m_machineRank;

	int m_mode_send_coverage_iterator;

	SeedingData*m_seedingData;

	int m_numberOfRanksDoneDetectingDistances;
	// read, strand, position
	map<u64,int > m_readsPositions;
	int m_numberOfRanksDoneSendingDistances;


	StaticVector m_outbox;
	StaticVector m_inbox;


	ExtensionData*m_ed;
	ChooserData*m_cd;

	// coverage distribubtion
	map<int,uint64_t> m_coverageDistribution;
	int m_minimumCoverage;
	int m_peakCoverage;
	int m_seedCoverage;
	int m_numberOfMachinesDoneSendingCoverage;
	
	string m_VERSION;
	bool m_mode_sendDistribution;

	Parameters m_parameters;
	int m_numberOfMachinesDoneSendingEdges;
	MyForest m_subgraph;

	// SEQUENCE DISTRIBUTION
	bool m_reverseComplementVertex;

	// memory allocators
	// m_outboxAllocator, m_inboxAllocator, and m_distributionAllocator are
	// cleaned everynow and then.
	
	// allocator for outgoing messages
	RingAllocator m_outboxAllocator;
	
	// allocator for ingoing messages
	RingAllocator m_inboxAllocator;
	
	// allocator for persistent data
	MyAllocator m_persistentAllocator;

	// allocator for directions in the de Bruijn graph
	MyAllocator m_directionsAllocator;


	ArrayOfReads m_myReads;

	bool m_mode_send_vertices;
	int m_mode_send_vertices_sequence_id;
	int m_mode_send_vertices_sequence_id_position;
	int m_numberOfMachinesDoneSendingVertices;
	int m_sequence_id;
	int m_fileId;
	int m_sequence_idInFile;

	vector<vector<uint64_t> > m_allPaths;
	bool m_aborted;

	bool m_messageSentForEdgesDistribution;

	// COLLECTING things.
	vector<int> m_identifiers;
	// FINISHING.
	int m_cycleNumber;
	int m_FINISH_n;
	int m_DISTRIBUTE_n;
	bool m_isFinalFusion;
	bool m_FINISH_hits_computed;
	int m_FINISH_hit;

	#ifdef ASSERT
	set<int> m_collisions;
	#endif
	bool m_nextReductionOccured;
	bool m_cycleStarted;
	bool m_reductionOccured;


	#ifdef SHOW_SENT_MESSAGES
	StatisticsData*m_stats;
	#endif
	SequencesLoader m_sl;

	int m_repeatedLength;
	bool m_colorSpaceMode;

	OpenAssemblerChooser m_oa;
	// BUBBLE
	BubbleData*m_bubbleData;


	DepthFirstSearchData*m_dfsData;

	int milliSeconds();
	void enumerateChoices();
	void killRanks();
	void printStatus();
	void doChoice();
	void checkIfCurrentVertexIsAssembled();
	void markCurrentVertexAsAssembled();
	int getSize();
	bool isAlive();
	void run();
	void do_1_1_test();
	bool isMaster();
	void receiveMessages();
	void loadSequences();
	void processMessages();
	void processMessage(Message*message);
	void sendMessages();
	void checkRequests();
	void processData();
	int getRank();
	void receiveWelcomeMessage(MPI_Status*status);
	int vertexRank(uint64_t a);
	void getPaths(uint64_t vertex);
	void extendSeeds();
	void finishFusions();
	void makeFusions();
	void depthFirstSearch(uint64_t root,uint64_t a,int b);
	void showUsage();
	

	void call_MASTER_MODE_LOAD_CONFIG();
	void call_MASTER_MODE_LOAD_SEQUENCES();
	void call_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION();
	void call_MASTER_MODE_SEND_COVERAGE_VALUES();
	void call_MASTER_MODE_TRIGGER_EDGES_DISTRIBUTION();
	void call_MASTER_MODE_START_EDGES_DISTRIBUTION();
	void call_MODE_EXTRACT_VERTICES();
	void call_MASTER_MODE_TRIGGER_EDGES();
	void call_MASTER_MODE_TRIGGER_INDEXING();
	void call_MASTER_MODE_PREPARE_DISTRIBUTIONS();
	void call_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS();
	void call_MASTER_MODE_PREPARE_SEEDING();
	void call_MODE_ASSEMBLE_WAVES();
	void call_MODE_PERFORM_CALIBRATION();
	void call_MODE_FINISH_FUSIONS();
	void call_MODE_DISTRIBUTE_FUSIONS();
	void call_MODE_SEND_DISTRIBUTION();
	void call_MODE_PROCESS_OUTGOING_EDGES();
	void call_MODE_PROCESS_INGOING_EDGES();
	void call_MASTER_MODE_TRIGGER_SEEDING();
	void call_MODE_START_SEEDING();
	void call_MASTER_MODE_TRIGGER_DETECTION();
	void call_MASTER_MODE_ASK_DISTANCES();
	void call_MASTER_MODE_START_UPDATING_DISTANCES();
	void call_MASTER_MODE_INDEX_SEQUENCES();
	void call_MASTER_MODE_TRIGGER_EXTENSIONS();
	void call_MODE_SEND_EXTENSION_DATA();
	void call_MODE_FUSION();
	void call_MODE_AUTOMATIC_DISTANCE_DETECTION();
	void call_MODE_SEND_LIBRARY_DISTANCES();
	void call_MASTER_MODE_UPDATE_DISTANCES();
	void call_MASTER_MODE_TRIGGER_FUSIONS();
	void call_MASTER_MODE_ASSEMBLE_WAVES();
	void call_MASTER_MODE_TRIGGER_FIRST_FUSIONS();
	void call_MASTER_MODE_START_FUSION_CYCLE();
	void call_MASTER_MODE_ASK_EXTENSIONS();
	void call_MASTER_MODE_AMOS();
	void call_MODE_EXTENSION();
	void call_MASTER_MODE_DO_NOTHING();
	void call_MODE_DO_NOTHING();
	void call_MODE_INDEX_SEQUENCES();

public:
	/*
 * this is the only public bit
 */
	Machine(int argc,char**argv);
	void start();
	~Machine();
};


#endif


