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

#include "SeedMergingWorkflow.h"
#include "NanoMerger.h"

/*
 * Methods to implement for the TaskCreator interface.
 *
 * The TaskCreator stack is used in the handler
 * RAY_SLAVE_MODE_FILTER_SEEDS.
 */

/** initialize the whole thing */
void SeedMergingWorkflow::initializeMethod(){

	m_seedIndex=0;

	m_finished = 0 ;
}

/** finalize the whole thing */
void SeedMergingWorkflow::finalizeMethod(){

	//cout << "[DEBUG] number of relations: " << m_searchResults.size() << endl;

	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
}

/** has an unassigned task left to compute */
bool SeedMergingWorkflow::hasUnassignedTask(){

	/*
	 * This code is still buggy. I will do the 2.3.0 release
	 * without it.
	 */
	bool enableSeedMerging = m_parameters->hasOption("-merge-seeds");

	//enableSeedMerging = true;

	if(!enableSeedMerging)
		return false;

	return m_seedIndex < (int) m_seeds->size () ;
}

/** assign the next task to a worker and return this worker */
Worker*SeedMergingWorkflow::assignNextTask(){

#ifdef CONFIG_ASSERT
	assert(m_seedIndex < (int)m_seeds->size());
#endif

	if(m_seedIndex % m_period == 0)
		cout<<"Rank "<<m_rank<< " assignNextTask "<<m_seedIndex<< "/" << m_seeds->size()<<endl;

	NanoMerger*worker = new NanoMerger();

	worker->initialize(m_seedIndex, &((*m_seeds)[m_seedIndex]), m_parameters,
		m_virtualCommunicator,
		m_core->getOutboxAllocator(),
		RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
		RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
		RAY_MPI_TAG_ASK_VERTEX_PATH
	);

	m_seedIndex++;

	return worker;
}

/** get the result of a worker */
void SeedMergingWorkflow::processWorkerResult(Worker*worker){

	NanoMerger * theWorker = (NanoMerger*)worker;

	vector<GraphSearchResult> & results = theWorker->getResults();

	bool runTransaction = true;

	if(results.size() == 2) {

		// make sure this is not something like
		//
		// A001 --------B
		// |            /
		// A100 --------
		//
		// with repeats, both ends of A could link to B...

		set<PathHandle> observations;

		for(int i = 0 ; i < (int) results.size() ; i++) {
			GraphSearchResult & result = results[i];

			observations.insert(result.getPathHandles()[0]);
			observations.insert(result.getPathHandles()[1]);
		}

		if(observations.size() != 3)
			runTransaction = false;
	}

	// we must send the results now !!!A
	// we will send at most 2 messages...
	// here we don't send messages right away because these units are
	// not regular.

	for(int i = 0 ; i < (int) results.size() ; i ++) {
		if(!runTransaction)
			break;
		m_searchResults.push_back(results[i]);
	}

	if(m_finished % m_period == 0){
		cout<<"Rank " << m_rank << " processWorkerResult "<<m_finished<<"/" <<m_seeds->size()<<endl;
	}

	m_finished++;
}

/** destroy a worker */
void SeedMergingWorkflow::destroyWorker(Worker*worker){

	NanoMerger * concreteWorker = (NanoMerger*)worker;
	delete concreteWorker;
	worker = NULL; // this does nothing useful
	concreteWorker = NULL;
}

void SeedMergingWorkflow::initialize(vector<GraphPath>*seeds, VirtualCommunicator*virtualCommunicator,
	VirtualProcessor * virtualProcessor,ComputeCore * core, Parameters * parameters,
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH){

	m_rank = core->getRank();
	m_seeds = seeds;
	m_virtualCommunicator = virtualCommunicator;
	m_virtualProcessor = virtualProcessor;
	m_core = core;
	m_parameters = parameters;

	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	this->RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE = RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	this->RAY_MPI_TAG_ASK_VERTEX_PATH = RAY_MPI_TAG_ASK_VERTEX_PATH;

	m_period = 100;

}

vector<GraphSearchResult> & SeedMergingWorkflow::getResults() {
	return m_searchResults;
}
