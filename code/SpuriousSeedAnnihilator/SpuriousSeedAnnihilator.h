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

#include "SeedFilteringWorkflow.h"

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

__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_MERGE_SEEDS);

__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_MERGE_SEEDS);

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
 *          |
 *          |
 *          |
 *    SeedFilteringWorkflow
 *                   |
 *                   |
 *                   |
 *              AnnihilationWorker
 *              |               |
 *              |               |
 *              |               |
 *     AttributeFetcher         |
 *                              |
 *                      AnnotationFetcher
 *
 *
 * \author Sébastien Boisvert
 *
 * \see TODO put github issues hereS
 *
 */
class SpuriousSeedAnnihilator: public CorePlugin {

	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_MERGE_SEEDS);

	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_MERGE_SEEDS);

	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_MERGE_SEEDS);

	MasterMode RAY_MASTER_MODE_REGISTER_SEEDS;
	MasterMode RAY_MASTER_MODE_FILTER_SEEDS;
	MasterMode RAY_MASTER_MODE_CLEAN_SEEDS;
	MasterMode RAY_MASTER_MODE_PUSH_SEED_LENGTHS;
	MasterMode RAY_MASTER_MODE_TRIGGER_DETECTION;
	MasterMode RAY_MASTER_MODE_MERGE_SEEDS;

	SlaveMode RAY_SLAVE_MODE_REGISTER_SEEDS;
	SlaveMode RAY_SLAVE_MODE_FILTER_SEEDS;
	SlaveMode RAY_SLAVE_MODE_CLEAN_SEEDS;
	SlaveMode RAY_SLAVE_MODE_PUSH_SEED_LENGTHS;
	SlaveMode RAY_SLAVE_MODE_MERGE_SEEDS;

	MessageTag RAY_MESSAGE_TAG_REGISTER_SEEDS;
	MessageTag RAY_MESSAGE_TAG_FILTER_SEEDS;
	MessageTag RAY_MESSAGE_TAG_CLEAN_SEEDS;
	MessageTag RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS;
	MessageTag RAY_MESSAGE_TAG_MERGE_SEEDS;

	MessageTag RAY_MESSAGE_TAG_PUSH_SEEDS;
	MessageTag RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY;
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;

	MessageTag RAY_MESSAGE_TAG_SEND_SEED_LENGTHS;
	MessageTag RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY;
	MessageTag RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS;

	bool m_skip;
	bool m_distributionIsStarted;
	bool m_filteringIsStarted;
	bool m_cleaningIsStarted;
	bool m_gatheringHasStarted;

	map<int,int> m_masterSeedLengths;
	map<int,int> m_slaveSeedLengths;
	map<int,int>::iterator m_iterator;

/*
 * The workflow implements the TaskCreator interface so that it's possible for a 
 * single CorePlugin to implement several workflows that use VirtualCommunicator and
 * VirtualProcessor via TaskCreator.
 */
	SeedFilteringWorkflow m_workflow;

	GridTable*m_subgraph;
	MyAllocator*m_directionsAllocator;

	vector<GraphPath>*m_seeds;
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

	bool m_hasMergedSeeds;

	ComputeCore * getCore();

	/* some useless methods. */
	SpuriousSeedAnnihilator * getThis();
	SpuriousSeedAnnihilator * getThat();

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

	void call_RAY_SLAVE_MODE_REGISTER_SEEDS();
	void call_RAY_SLAVE_MODE_FILTER_SEEDS();
	void call_RAY_SLAVE_MODE_CLEAN_SEEDS();
	void call_RAY_SLAVE_MODE_PUSH_SEED_LENGTHS();
	void call_RAY_SLAVE_MODE_MERGE_SEEDS();

	void call_RAY_MESSAGE_TAG_REGISTER_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_FILTER_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_CLEAN_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS(Message*message);
	void call_RAY_MESSAGE_TAG_SEND_SEED_LENGTHS(Message*message);
	void call_RAY_MESSAGE_TAG_MERGE_SEEDS(Message*message);
};

#endif /* _SpuriousSeedAnnihilator_h */
