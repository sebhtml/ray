/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
 *  Copyright (C) 2013 Sébastien Boisvert
 *
 *  http://DeNovoAssembler.SourceForge.Net/
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You have received a copy of the GNU General Public License
 *  along with this program (gpl-3.0.txt).
 *  see <http://www.gnu.org/licenses/>
 */

#define GOSSIP_ALGORITHM_FOR_SEEDS "Godzy algorithm v13.0.7"

#include "SeedFilteringWorkflow.h"
#include "SeedMergingWorkflow.h"
#include "SeedGossipSolver.h"
#include "GossipAssetManager.h"

#include <code/SeedingData/GraphPath.h>
#include <code/Mock/Parameters.h>
#include <code/VerticesExtractor/GridTable.h>

#include <RayPlatform/core/ComputeCore.h>
#include <RayPlatform/communication/VirtualCommunicator.h>
#include <RayPlatform/communication/BufferedData.h>

#include <vector>
#include <map>
using namespace std;

__DeclarePlugin(SpuriousSeedAnnihilator);

__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);
__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);
__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_MERGE_SEEDS);
__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PROCESS_MERGING_ASSETS);

__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_MERGE_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS);

__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_MERGE_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION);

#ifndef _SpuriousSeedAnnihilator_h
#define _SpuriousSeedAnnihilator_h

/**
 * This class cleans undesired seeds.
 *
 * Undesired seeds are produced sometimes when using
 * large kmers.
 *
 * Steps:
 *
 * 1. Distribute seeds in the graph;
 * 2. Run the filter on seeds;
 * 3. Clean the seed annotations.
 *
 * Design:
 *
 * SpuriousSeedAnnihilator
 *          |           \---------------------------------
 *          |                                            |
 *          |                                            |
 *    SeedFilteringWorkflow                          SeedMergingWorkflow
 *                   |                                   |
 *                   |                                   |
 *                   |                                   |
 *              AnnihilationWorker                    NanoMerger
 *              |               |                     |      |
 *              |               |                     |      |
 *              |               |                     |      |
 *     AttributeFetcher         |                     |  AnnotationFetcher
 *                              |                     |
 *                      AnnotationFetcher             |
 *                                                  AttributeFetcher
 *
 * \author Sébastien Boisvert
 *
 */
class SpuriousSeedAnnihilator: public CorePlugin {

	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_MERGE_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PROCESS_MERGING_ASSETS);

	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_MERGE_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS);

	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_MERGE_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION);

	int m_toDistribute;
	SeedGossipSolver m_seedGossipSolver;
	vector<GraphSearchResult> m_newSeedBluePrints;

	MasterMode RAY_MASTER_MODE_REGISTER_SEEDS;
	MasterMode RAY_MASTER_MODE_FILTER_SEEDS;
	MasterMode RAY_MASTER_MODE_CLEAN_SEEDS;
	MasterMode RAY_MASTER_MODE_PUSH_SEED_LENGTHS;
	MasterMode RAY_MASTER_MODE_TRIGGER_DETECTION;
	MasterMode RAY_MASTER_MODE_MERGE_SEEDS;
	MasterMode RAY_MASTER_MODE_PROCESS_MERGING_ASSETS;

	SlaveMode RAY_SLAVE_MODE_REGISTER_SEEDS;
	SlaveMode RAY_SLAVE_MODE_FILTER_SEEDS;
	SlaveMode RAY_SLAVE_MODE_CLEAN_SEEDS;
	SlaveMode RAY_SLAVE_MODE_PUSH_SEED_LENGTHS;
	SlaveMode RAY_SLAVE_MODE_MERGE_SEEDS;
	SlaveMode RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS;

	MessageTag RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION;
	MessageTag RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION_REPLY;

	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY;

	MessageTag RAY_MESSAGE_TAG_REGISTER_SEEDS;
	MessageTag RAY_MESSAGE_TAG_FILTER_SEEDS;
	MessageTag RAY_MESSAGE_TAG_CLEAN_SEEDS;
	MessageTag RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS;
	MessageTag RAY_MESSAGE_TAG_MERGE_SEEDS;
	MessageTag RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS;

	MessageTag RAY_MESSAGE_TAG_PUSH_SEEDS;
	MessageTag RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY;
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;

	MessageTag RAY_MESSAGE_TAG_SEND_SEED_LENGTHS;
	MessageTag RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY;
	MessageTag RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS;
	MessageTag RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY;
	MessageTag RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY;
	MessageTag RAY_MESSAGE_TAG_ARBITER_SIGNAL;
	MessageTag RAY_MESSAGE_TAG_SEED_GOSSIP;
	MessageTag RAY_MESSAGE_TAG_SEED_GOSSIP_REPLY;
	MessageTag RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER;

	GossipAssetManager m_gossipAssetManager;

	int MODE_SPREAD_DATA;
	int MODE_GATHER_COVERAGE_VALUES;
	int MODE_STOP_THIS_SITUATION;
	int MODE_REBUILD_SEED_ASSETS;
	int MODE_SHARE_PUSH_DATA_IN_KEY_VALUE_STORE;
	int MODE_CHECK_RESULTS;
	int MODE_SHARE_WITH_LINKED_ACTORS;
	int MODE_WAIT_FOR_ARBITER;
	int MODE_EVALUATE_GOSSIPS;
	int MODE_CLEAN_KEY_VALUE_STORE;
	int MODE_GENERATE_NEW_SEEDS;
	Rank m_rankToAdvise;

	bool m_mustAdviseRanks;
	int m_synced;
	int m_mode;
	int m_nextMode;
	bool m_initializedProcessing;
	int m_entryIndex;

	bool m_processingIsStarted;
	bool m_debug;
	bool m_mergingIsStarted;
	bool m_skip;
	bool m_distributionIsStarted;
	bool m_filteringIsStarted;
	bool m_cleaningIsStarted;
	bool m_gatheringHasStarted;

	bool m_filteredSeeds;
	bool m_mergedSeeds;
	bool m_initializedSeedRegistration;

	int m_cleaningIterations;
	int m_registrationIterations;
	map<int,int> m_masterSeedLengths;
	map<int,int> m_slaveSeedLengths;
	map<int,int>::iterator m_iterator;

	KeyValueStoreRequest m_request;

/*
 * The workflow implements the TaskCreator interface so that it's possible for a 
 * single CorePlugin to implement several workflows that use VirtualCommunicator and
 * VirtualProcessor via TaskCreator.
 */
	SeedFilteringWorkflow m_workflow;
	SeedMergingWorkflow m_mergingTechnology;

	GridTable*m_subgraph;
	MyAllocator*m_directionsAllocator;

	vector<GraphPath>*m_seeds;
	vector<GraphPath> m_newSeeds;

	Parameters*m_parameters;

	int m_seedIndex;
	int m_seedPosition;

	int m_activeQueries;
	BufferedData m_buffers;

	VirtualCommunicator * m_virtualCommunicator;
	VirtualProcessor * m_virtualProcessor;

	Rank m_rank;
	int m_size;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_inbox;
	StaticVector*m_outbox;

	bool m_hasCheckpointFilesForSeeds;
	bool m_initialized;
	bool m_communicatorWasTriggered;

	void writeSeedStatistics();
	void writeSingleSeedFile();
	void writeCheckpointForSeeds();

	bool m_debugCode;
	bool m_messageWasSent;
	bool m_messageWasReceived;

#ifdef GOSSIP_ALGORITHM_FOR_SEEDS
	bool m_hasNewGossips;
	time_t m_lastGossipingEventTime;
	map<int, set<Rank> > m_gossipStatus;
	set<Rank> m_linkedActorsForGossip;
#endif /* GOSSIP_ALGORITHM_FOR_SEEDS */

	ComputeCore * getCore();

	/* some useless methods. */
	SpuriousSeedAnnihilator * getThis();
	SpuriousSeedAnnihilator * getThat();

	// stuff to gather coverage values

	BufferedData * m_buffersForMessages;
	BufferedData * m_buffersForPaths;
	BufferedData * m_buffersForPositions;
	int m_pendingMessages;

	int m_pathIndex;
	int m_location;

	Rank getArbiter();
	bool isPrimeNumber(int number);

	void initializeMergingProcess();
	void spreadAcquiredData();
	void shareWithLinkedActors();
	void checkResults();
	void evaluateGossips();
	void rebuildSeedAssets();
	void pushDataInKeyValueStore();
	void sendMessageToArbiter();

	void getSeedKey(PathHandle & handle, string & keyObject);

	void generateNewSeeds();
	void cleanKeyValueStore();
	void gatherCoverageValues();

public:

	SpuriousSeedAnnihilator();

/*
 * Methods to implement for the CorePlugin interface.
 */
	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

/*
 * handlers.
 */
	void call_RAY_MASTER_MODE_REGISTER_SEEDS();
	void call_RAY_MASTER_MODE_FILTER_SEEDS();
	void call_RAY_MASTER_MODE_CLEAN_SEEDS();
	void call_RAY_MASTER_MODE_PUSH_SEED_LENGTHS();
	void call_RAY_MASTER_MODE_MERGE_SEEDS();
	void call_RAY_MASTER_MODE_PROCESS_MERGING_ASSETS();

	void call_RAY_SLAVE_MODE_REGISTER_SEEDS();
	void call_RAY_SLAVE_MODE_FILTER_SEEDS();
	void call_RAY_SLAVE_MODE_CLEAN_SEEDS();
	void call_RAY_SLAVE_MODE_PUSH_SEED_LENGTHS();
	void call_RAY_SLAVE_MODE_MERGE_SEEDS();
	void call_RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS();

	void call_RAY_MESSAGE_TAG_REGISTER_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_FILTER_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_CLEAN_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS(Message*message);
	void call_RAY_MESSAGE_TAG_SEND_SEED_LENGTHS(Message*message);
	void call_RAY_MESSAGE_TAG_MERGE_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS(Message * message);
	void call_RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY(Message * message);
	void call_RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY(Message * message);
	void call_RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER(Message * message);

	void call_RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION(Message * message);
};

#endif /* _SpuriousSeedAnnihilator_h */
