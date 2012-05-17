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

#ifndef _Machine
#define _Machine

/** Chapter I. stuff pulled from RayPlatform, called The Platform hereafter */

/** the heart of RayPlatform  **/
/** ComputeCore is a facade (a design pattern) **/
#include <core/ComputeCore.h>

/** communication */
#include <communication/VirtualCommunicator.h> /** virtual stuff */
#include <communication/Message.h>
#include <communication/MessageRouter.h>
#include <communication/MessagesHandler.h> /** MPI wrapper **/

/** scheduling, virtual pools */
#include <scheduling/VirtualProcessor.h> /** thread pool **/

/** distributed scheduler, this is a state machine */
/* design pattern: state */
#include <scheduling/SwitchMan.h>  

/** memory stuff */
#include <memory/RingAllocator.h>
#include <memory/MyAllocator.h>

/** data structure */
#include <structures/StaticVector.h>
#include <structures/SplayTree.h>
#include <structures/SplayTreeIterator.h>

/** run-time code profiling */
#include <profiling/TimePrinter.h>
#include <profiling/Profiler.h>
#include <profiling/TickLogger.h>

/** interfaces to define adapters compatible with RayPlatform **/
#include <handlers/MessageTagHandler.h>
#include <handlers/SlaveModeHandler.h>
#include <handlers/MessageTagHandler.h>


/** Chapter II.  stuff related to the Ray Genome Assembler **/

/** stuff specific to the Ray assembler that don't fit elsewhere */

#include <application_core/common_functions.h> /* common code for all plugins */
#include <application_core/Parameters.h> /* command-line parsing */

// also in this category is application_core/Machine.h 


/** list of RayPlatform CorePlugin objects for the Ray genome software suite. */
/* (in no particular order) */

#include <plugin_SeedExtender/DepthFirstSearchData.h>
#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood.h>
#include <plugin_Scaffolder/Scaffolder.h>
#include <plugin_VerticesExtractor/GridTable.h>
#include <plugin_Partitioner/Partitioner.h>
#include <plugin_SequencesLoader/ArrayOfReads.h>
#include <plugin_SeedingData/SeedingData.h>
#include <plugin_SequencesIndexer/SequencesIndexer.h>
#include <plugin_SeedExtender/SeedExtender.h>
#include <plugin_SequencesLoader/SequencesLoader.h>
#include <plugin_Library/Library.h>
#include <plugin_CoverageGatherer/CoverageGatherer.h>
#include <plugin_MessageProcessor/MessageProcessor.h>
#include <plugin_VerticesExtractor/Vertex.h>
#include <plugin_SeedExtender/OpenAssemblerChooser.h>
#include <plugin_SeedExtender/BubbleData.h>
#include <plugin_VerticesExtractor/VerticesExtractor.h>
#include <plugin_Amos/Amos.h>
#include <plugin_KmerAcademyBuilder/KmerAcademyBuilder.h>
#include <plugin_EdgePurger/EdgePurger.h>
#include <plugin_NetworkTest/NetworkTest.h>
#include <plugin_FusionTaskCreator/FusionTaskCreator.h>
#include <plugin_JoinerTaskCreator/JoinerTaskCreator.h>
#include <plugin_SequencesLoader/Read.h>
#include <plugin_Searcher/Searcher.h>
#include <plugin_MachineHelper/MachineHelper.h>
#include <plugin_PhylogenyViewer/PhylogenyViewer.h>
#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood.h>
#include <plugin_GeneOntology/GeneOntology.h>

/** C++ bits **/
#include <map>
#include <vector>
#include <set>
#include <time.h>

using namespace std;

/**
 * This class builds plugin objects
 * and register them using the API of
 * ComputeCore.
 *
 * Each plugin will register things with the
 * core.
 *
 * \author Sébastien Boisvert
 */
class Machine{

	ComputeCore m_computeCore;
	Chooser m_c;
	MachineHelper m_helper;

/** a plugin to compute a global map of the genome (or meta-genome or 
 * transcriptome) studied 
 */
	GenomeNeighbourhood m_genomeNeighbourhood;


	Searcher m_searcher;

	SwitchMan*m_switchMan;
	TickLogger*m_tickLogger;
	MessageRouter*m_router;

	Profiler*m_profiler;

	/** the virtual communicator of the MPI rank */
	VirtualCommunicator*m_virtualCommunicator;

	/** the virtual processor of the MPI rank */
	VirtualProcessor*m_virtualProcessor;

	FusionTaskCreator m_fusionTaskCreator;
	JoinerTaskCreator m_joinerTaskCreator;

	Partitioner m_partitioner;
	NetworkTest m_networkTest;
	EdgePurger m_edgePurger;
	PhylogenyViewer m_phylogeny;
	GeneOntology m_ontologyPlugin;

	KmerAcademyBuilder m_kmerAcademyBuilder;
	bool m_initialisedAcademy;
	CoverageGatherer m_coverageGatherer;

	Amos m_amos;

	Library m_library;
	MessagesHandler*m_messagesHandler;
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
	Rank m_rank;
	int m_size;
	int m_totalLetters;
	bool*m_alive;
	char*m_inputFile;
	int m_sequence_ready_machines;
	bool m_messageSentForVerticesDistribution;

	SequencesIndexer m_si;
	SeedExtender m_seedExtender;

	bool m_ready;

	// clearing
	int m_CLEAR_n;
	int m_readyToSeed;
	bool m_showMessages;
	bool m_mode_send_ingoing_edges;


	bool m_startEdgeDistribution;
	bool m_mode_AttachSequences;

	// Counters.
	int m_numberOfMachinesReadyForEdgesDistribution;
	int m_ranksDoneAttachingReads;
	int m_numberOfMachinesReadyToSendDistribution;
	int m_numberOfRanksDoneSeeding;
	bool m_writeKmerInitialised;
	FusionData*m_fusionData;

	int m_mode_send_coverage_iterator;

	SeedingData*m_seedingData;

	int m_numberOfRanksDoneDetectingDistances;
	// read, strand, position
	int m_numberOfRanksDoneSendingDistances;

	StaticVector*m_outbox;
	StaticVector*m_inbox;

	ExtensionData*m_ed;

	// coverage distribubtion
	map<CoverageDepth,LargeCount> m_coverageDistribution;
	int m_numberOfMachinesDoneSendingCoverage;
	
	string m_VERSION;
	bool m_mode_sendDistribution;

	Parameters m_parameters;
	int m_numberOfMachinesDoneSendingEdges;
	GridTable m_subgraph;

	Scaffolder m_scaffolder;

	// memory allocators
	// m_outboxAllocator, m_inboxAllocator, and m_distributionAllocator are
	// cleaned everynow and then.
	
	// allocator for outgoing messages
	RingAllocator*m_outboxAllocator;
	
	// allocator for ingoing messages
	RingAllocator*m_inboxAllocator;
	
	// allocator for persistent data
	MyAllocator m_persistentAllocator;

	// allocator for directions in the de Bruijn graph
	MyAllocator m_directionsAllocator;

	ArrayOfReads m_myReads;

	bool m_mode_send_vertices;
	int m_mode_send_vertices_sequence_id;
	int m_numberOfMachinesDoneSendingVertices;
	int m_sequence_id;
	int m_fileId;
	int m_sequence_idInFile;

	vector<vector<uint64_t> > m_allPaths;
	bool m_aborted;

	bool m_messageSentForEdgesDistribution;
	int m_maximumAllocatedOutputBuffers;
	// COLLECTING things.
	vector<PathHandle> m_identifiers;
	// FINISHING.
	int m_FINISH_n;
	int m_DISTRIBUTE_n;
	bool m_isFinalFusion;
	bool m_FINISH_hits_computed;
	int m_FINISH_hit;

	#ifdef ASSERT
	set<int> m_collisions;
	#endif
	bool m_reductionOccured;

	#ifdef SHOW_SENT_MESSAGES
	#endif
	SequencesLoader m_sl;

	int m_repeatedLength;

	OpenAssemblerChooser m_oa;
	// BUBBLE
	BubbleData*m_bubbleData;
	int getSize();
/**
 * this is the function that runs a lots
 *
 * it
 * 	1) receives messages
 * 	3) process message. The function that deals with a message is selected with the message's tag
 * 	4) process data, this depends on the master-mode and slave-mode states.
 * 	5) send messages
 */
	bool isMaster();
	void loadSequences();
	void checkRequests();


	int getRank();

	void showRayVersion(MessagesHandler*messagesHandler,bool fullReport);

	void showRayVersionShort();

	void registerPlugins();

public:
	/*
 * this is the only public bit
 */
	Machine(int argc,char**argv);
	void start();
	~Machine();
};

#endif

