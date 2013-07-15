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

#include "NanoMerger.h"

#include <stack>
using namespace std;

/**
 * Here, we want to do a depth first search
 * on the left and on the right to find
 * nearby paths.
 *
 * Local arbitration will deal with ownership.
 *
 * \author Sébastien Boisvert
 */
void NanoMerger::work(){

	//m_done = true;

	if(!m_startedFirst) {

		m_explorer.start(m_identifier, &m_first, EXPLORER_LEFT, m_parameters,
			m_virtualCommunicator,
			m_outboxAllocator,
			RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
			RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, RAY_MPI_TAG_ASK_VERTEX_PATH
		);

		m_startedFirst = true;

	} else if(m_startedFirst && !m_startedLast && m_explorer.work()) {

		cout << "[DEBUG] NanoMerger processed first, seed length is " << m_seed->size() << endl;

		// now do the last one.
		m_explorer.start(m_identifier, &m_last, EXPLORER_RIGHT, m_parameters,
			m_virtualCommunicator,
			m_outboxAllocator,
			RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
			RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, RAY_MPI_TAG_ASK_VERTEX_PATH
		);

		m_startedLast = true;

	} else if(m_startedLast && m_explorer.work()) {

		cout << "[DEBUG] NanoMerger processed last, seed length is " << m_seed->size() << endl;
		m_done = true;
	}
}

bool NanoMerger::isDone(){

	return m_done;
}

WorkerHandle NanoMerger::getWorkerIdentifier(){

	return m_identifier;
}

/**
 * RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE
 * RAY_MPI_TAG_ASK_VERTEX_PATH
 */
void NanoMerger::initialize(WorkerHandle identifier,GraphPath*seed, Parameters * parameters,
	VirtualCommunicator * virtualCommunicator, RingAllocator*outboxAllocator,
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH
	){

	//cout << "[DEBUG] configuring nano merger now." << endl;

	m_identifier = identifier;
	m_done = false;

	m_seed = seed;

	m_virtualCommunicator = virtualCommunicator;
	m_parameters = parameters;

	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;

	this->RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE = RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	this->RAY_MPI_TAG_ASK_VERTEX_PATH = RAY_MPI_TAG_ASK_VERTEX_PATH;

	m_rank = m_parameters->getRank();
	m_outboxAllocator = outboxAllocator;

	int index1 = 0;
	m_seed->at(index1, &m_first);

	int index2 = m_seed->size() - 1;
	m_seed->at(index2, &m_last);

#ifdef CONFIG_ASSERT
	assert(index1 == 0);
	assert(index2 >= 0);
#endif
	//cout << "[DEBUG] achieved with RayPlatform." << endl;

	m_startedFirst = false;
	m_startedLast = false;
}


