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

#ifndef _Machine
#define _Machine

#include <scaffolder/Scaffolder.h>
#include <graph/GridTable.h>
#include <communication/MessagesHandler.h>
#include <core/common_functions.h>
#include <structures/MyForest.h>
#include <structures/ArrayOfReads.h>
#include <memory/OnDiskAllocator.h>
#include <assembler/MemoryConsumptionReducer.h>
#include <structures/StaticVector.h>
#include <assembler/SeedingData.h>
#include <map>
#include <vector>
#include <memory/RingAllocator.h>
#include <assembler/DepthFirstSearchData.h>
#include <assembler/TimePrinter.h>
#include <assembler/SequencesIndexer.h>
#include <assembler/SeedExtender.h>
#include <assembler/SequencesLoader.h>
#include <assembler/Library.h>
#include <assembler/Chooser.h>
#include <communication/MessageProcessor.h>
#include <structures/Vertex.h>
#include <assembler/OpenAssemblerChooser.h>
#include <structures/SplayTree.h>
#include <assembler/BubbleData.h>
#include <communication/Message.h>
#include <structures/SplayTreeIterator.h>
#include <structures/Read.h>
#include <core/Parameters.h>
#include <memory/MyAllocator.h>
#include <assembler/VerticesExtractor.h>
#include <format/Amos.h>
#include <set>
#include <time.h>

using namespace std;

/**
* Main class of the application. Runs the main program loop on each MPI rank.
*/
class Machine;
typedef void (Machine::*MachineMethod) ();

class Machine{
	Amos m_amos;
	VirtualCommunicator m_virtualCommunicator;
	bool m_killed;
	MachineMethod m_master_methods[64];
	MachineMethod m_slave_methods[64];

	bool m_mustStop;

	int MAX_ALLOCATED_MESSAGES_IN_OUTBOX;
	// always 1
	int MAX_ALLOCATED_MESSAGES_IN_INBOX;

	Library m_library;
	int m_currentCycleStep;
	MessagesHandler m_messagesHandler;
	MyAllocator m_diskAllocator;

	int m_numberOfRanksWithCoverageData;
	TimePrinter m_timePrinter;
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
	bool m_loadSequenceStep;
	char*m_inputFile;
	int m_sequence_ready_machines;
	bool m_messageSentForVerticesDistribution;

	bool m_waiting;
	map<int,uint64_t>::iterator m_coverageIterator;

	Chooser m_c;
	SequencesIndexer m_si;
	SeedExtender m_seedExtender;

	bool m_ready;

	// clearing
	int m_CLEAR_n;
	int m_readyToSeed;
	bool m_showMessages;
	bool m_mode_send_ingoing_edges;

	int m_slave_mode;
	int m_master_mode;

	MemoryConsumptionReducer m_reducer;

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
	int m_numberOfRanksDoneSendingDistances;


	StaticVector m_outbox;
	StaticVector m_inbox;


	ExtensionData*m_ed;

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
	GridTable m_subgraph;

	// SEQUENCE DISTRIBUTION
	bool m_reverseComplementVertex;

	Scaffolder m_scaffolder;

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
	int m_maximumAllocatedOutputBuffers;
	// COLLECTING things.
	vector<uint64_t> m_identifiers;
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
	#endif
	SequencesLoader m_sl;

	int m_repeatedLength;
	bool m_colorSpaceMode;

	OpenAssemblerChooser m_oa;
	// BUBBLE
	BubbleData*m_bubbleData;
	void killRanks();
	int getSize();
	bool isAlive();
/**
 * this is the function that runs a lots
 *
 * it
 * 	1) receives messages
 * 	3) process message. The function that deals with a message is selected with the message's tag
 * 	4) process data, this depends on the master-mode and slave-mode states.
 * 	5) send messages
 */
	void run();
	bool isMaster();
	void receiveMessages();
	void loadSequences();
	void processMessages();
	void processMessage(Message*message);
	void sendMessages();
	void checkRequests();
	void processData();
	int getRank();
	void runWithProfiler();
	void runVanilla();

	void assignMasterHandlers();
	void assignSlaveHandlers();

	#define MACRO_LIST_ITEM(x) void call_ ## x();
	#include <core/master_mode_macros.h>
	#include <core/slave_mode_macros.h>
	#undef MACRO_LIST_ITEM
	
public:
	/*
 * this is the only public bit
 */
	Machine(int argc,char**argv);
	void start();
	~Machine();
};


#endif


