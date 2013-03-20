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

#ifndef _AnnihilationWorker_h
#define _AnnihilationWorker_h

#include <code/plugin_SeedingData/GraphPath.h>
#include <code/plugin_Mock/Parameters.h>

#include <RayPlatform/scheduling/Worker.h>

#include <stdint.h>

/**
 * This is a worker that analyze a seed.
 *
 * \author Sébastien Boisvert
 */
class AnnihilationWorker: public Worker{

	uint64_t m_identifier;       // TODO this should be in Worker because it's always there anyway
	bool m_done;          // TODO this should be in Worker because it's always there anyway
	GraphPath * m_seed;

	VirtualCommunicator * m_virtualCommunicator; // TODO this should be in Worker because it's always there anyway
	Parameters * m_parameters;

	Kmer m_parent;
	Kmer m_grandparent;

	int m_step;

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;

	int STEP_CHECK_DEAD_END_ON_THE_LEFT;
	int STEP_CHECK_DEAD_END_ON_THE_RIGHT;
	int STEP_FETCH_FIRST_PARENT;
	int STEP_FETCH_SECOND_PARENT;
	int STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS;
	int STEP_GET_SEED_SEQUENCE_NOW;

	bool m_queryWasSent;
	Rank m_rank;
	RingAllocator*m_outboxAllocator;

public:
	void work();

	bool isDone();

	WorkerHandle getWorkerIdentifier();

	void initialize(uint64_t identifier, GraphPath*seed, Parameters * parameters,
		VirtualCommunicator * virtualCommunicator, MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
		RingAllocator * outboxAllocator);
};

#endif
