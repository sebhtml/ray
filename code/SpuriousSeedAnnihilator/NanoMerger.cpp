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
 *
 * \author Sébastien Boisvert
 */
void NanoMerger::work(){

	m_done = true;
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
void NanoMerger::initialize(uint64_t identifier,GraphPath*seed, Parameters * parameters,
	VirtualCommunicator * virtualCommunicator, RingAllocator*outboxAllocator,
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH
	){

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

	m_attributeFetcher.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);

	m_annotationFetcher.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
			RAY_MPI_TAG_ASK_VERTEX_PATH);

}


