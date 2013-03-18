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

#include <code/plugin_SeedingData/GraphPath.h>
#include <code/plugin_Mock/Parameters.h>
#include <code/plugin_VerticesExtractor/GridTable.h>

#include <RayPlatform/core/ComputeCore.h>
#include <RayPlatform/scheduling/TaskCreator.h>
#include <RayPlatform/communication/VirtualCommunicator.h>
#include <RayPlatform/communication/BufferedData.h>

#include <vector>
using namespace std;

__DeclarePlugin(SpuriousSeedAnnihilator);

__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
__DeclareMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);

__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
__DeclareSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);

__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
__DeclareMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);

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
 * \author Sébastien Boisvert
 * \see TODO put github issues here
 */
class SpuriousSeedAnnihilator: public CorePlugin, public TaskCreator {

	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);

	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);

	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
	__AddAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);

	MasterMode RAY_MASTER_MODE_REGISTER_SEEDS;
	MasterMode RAY_MASTER_MODE_FILTER_SEEDS;
	MasterMode RAY_MASTER_MODE_CLEAN_SEEDS;

	MasterMode RAY_MASTER_MODE_TRIGGER_DETECTION;

	SlaveMode RAY_SLAVE_MODE_REGISTER_SEEDS;
	SlaveMode RAY_SLAVE_MODE_FILTER_SEEDS;
	SlaveMode RAY_SLAVE_MODE_CLEAN_SEEDS;

	MessageTag RAY_MESSAGE_TAG_REGISTER_SEEDS;
	MessageTag RAY_MESSAGE_TAG_FILTER_SEEDS;
	MessageTag RAY_MESSAGE_TAG_CLEAN_SEEDS;

	MessageTag RAY_MESSAGE_TAG_PUSH_SEEDS;
	MessageTag RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY;

	bool m_distributionIsStarted;
	bool m_filteringIsStarted;
	bool m_cleaningIsStarted;

	GridTable*m_subgraph;
	MyAllocator*m_directionsAllocator;

	vector<GraphPath>*m_seeds;
	Parameters*m_parameters;

	int m_seedIndex;
	int m_seedPosition;

	int m_activeQueries;
	BufferedData m_buffers;
	VirtualCommunicator*m_virtualCommunicator;
	Rank m_rank;
	int m_size;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
public:

	SpuriousSeedAnnihilator();

/*
 * Methods to implement for the CorePlugin interface.
 */
	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

/*
 * Methods to implement for the TaskCreator interface.
 *
 * The TaskCreator stack is used in the handler
 * RAY_SLAVE_MODE_FILTER_SEEDS.
 */

	/** initialize the whole thing */
	void initializeMethod();

	/** finalize the whole thing */
	void finalizeMethod();

	/** has an unassigned task left to compute */
	bool hasUnassignedTask();

	/** assign the next task to a worker and return this worker */
	Worker*assignNextTask();

	/** get the result of a worker */
	void processWorkerResult(Worker*worker);

	/** destroy a worker */
	void destroyWorker(Worker*worker);

/*
 * handlers.
 */
	void call_RAY_MASTER_MODE_REGISTER_SEEDS();
	void call_RAY_MASTER_MODE_FILTER_SEEDS();
	void call_RAY_MASTER_MODE_CLEAN_SEEDS();

	void call_RAY_SLAVE_MODE_REGISTER_SEEDS();
	void call_RAY_SLAVE_MODE_FILTER_SEEDS();
	void call_RAY_SLAVE_MODE_CLEAN_SEEDS();

	void call_RAY_MESSAGE_TAG_REGISTER_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_FILTER_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_CLEAN_SEEDS(Message*message);
};

#endif /* _SpuriousSeedAnnihilator_h */
