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

#ifndef _SeedMergingWorkflow_h
#define _SeedMergingWorkflow_h

#include "GraphSearchResult.h"

#include <code/SeedingData/GraphPath.h>
#include <code/Mock/Parameters.h>

#include <RayPlatform/core/ComputeCore.h>
#include <RayPlatform/scheduling/TaskCreator.h>
#include <RayPlatform/communication/VirtualCommunicator.h>

#include <vector>
using namespace std;

/**
 *
 * This is used to merge seeds.
 *
 * This class is derived directly from the SeedFilteringWorkflow.
 *
 * \author Sébastien Boisvert
 */
class SeedMergingWorkflow: public TaskCreator {

	vector<GraphSearchResult> m_searchResults;
	vector<GraphSearchResult> m_remoteResults;

	ComputeCore*m_core;
	int m_seedIndex;
	Parameters * m_parameters;
	vector<GraphPath>*m_seeds;
	vector<bool> m_states;

	int m_finished;
	int m_rank;
	int m_period;

/* TODO: maybe this should be in the TaskCreator */
	VirtualCommunicator * m_virtualCommunicator;

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;

public:

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

	void initialize(vector<GraphPath>*seeds, VirtualCommunicator*virtualCommunicator,
		VirtualProcessor * virtualProcessor, ComputeCore * core,
		Parameters * parameters, MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
		MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
		MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH
	);

	vector<GraphSearchResult> & getResults();
};

#endif
