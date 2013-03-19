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
#include "AnnihilationWorker.h"

/*
 * Methods to implement for the TaskCreator interface.
 *
 * The TaskCreator stack is used in the handler
 * RAY_SLAVE_MODE_FILTER_SEEDS.
 */

/** initialize the whole thing */
void SeedFilteringWorkflow::initializeMethod(){

	m_seedIndex=0;
}

/** finalize the whole thing */
void SeedFilteringWorkflow::finalizeMethod(){

	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
}

/** has an unassigned task left to compute */
bool SeedFilteringWorkflow::hasUnassignedTask(){

	return m_seedIndex < (int) m_seeds->size () ;
}

/** assign the next task to a worker and return this worker */
Worker*SeedFilteringWorkflow::assignNextTask(){

#ifdef ASSERT
	assert(m_seedIndex < (int)m_seeds->size());
#endif

	AnnihilationWorker*worker=new AnnihilationWorker();
	worker->initialize(m_seedIndex, &((*m_seeds)[m_seedIndex]), m_parameters,
		m_virtualCommunicator, RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
		m_core->getOutboxAllocator());

	m_seedIndex++;

	return worker;
}

/** get the result of a worker */
void SeedFilteringWorkflow::processWorkerResult(Worker*worker){

	/* TODO: do something with the result */
}

/** destroy a worker */
void SeedFilteringWorkflow::destroyWorker(Worker*worker){

	delete worker;
	worker = NULL;
}

void SeedFilteringWorkflow::initialize(vector<GraphPath>*seeds, VirtualCommunicator*virtualCommunicator,
	VirtualProcessor * virtualProcessor,ComputeCore * core, Parameters * parameters,
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT){

	m_seeds = seeds;
	m_virtualCommunicator = virtualCommunicator;
	m_virtualProcessor = virtualProcessor;
	m_core = core;
	m_parameters = parameters;

	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
}
