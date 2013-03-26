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

#include "AttributeFetcher.h"
#include "AnnotationFetcher.h"

#include <code/plugin_SeedingData/GraphPath.h>
#include <code/plugin_Mock/Parameters.h>
#include <code/plugin_SeedExtender/Direction.h>

#include <RayPlatform/scheduling/Worker.h>

#include <stack>
#include <vector>
#include <set>
using namespace std;

#include <stdint.h>

/**
 * This is a worker that analyze a seed.
 *
 * \author Sébastien Boisvert
 */
class AnnihilationWorker: public Worker{

	AttributeFetcher m_attributeFetcher;
	AnnotationFetcher m_annotationFetcher;

	uint64_t m_identifier;       // TODO this should be in Worker because it's always there anyway
	bool m_done;          // TODO this should be in Worker because it's always there anyway
	GraphPath * m_seed;

	VirtualCommunicator * m_virtualCommunicator; // TODO this should be in Worker because it's always there anyway
	Parameters * m_parameters;

	Kmer m_parent;
	Kmer m_grandparent;
	Kmer m_child;
	Kmer m_grandchild;

	int m_step;

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;

	int STEP_CHECK_LENGTH;
	int STEP_CHECK_DEAD_END_ON_THE_LEFT;
	int STEP_CHECK_DEAD_END_ON_THE_RIGHT;
	int STEP_CHECK_BUBBLE_PATTERNS;
	int STEP_FETCH_FIRST_PARENT;
	int STEP_FETCH_SECOND_PARENT;
	int STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS;
	int STEP_GET_SEED_SEQUENCE_NOW;

	bool m_queryWasSent;
	Rank m_rank;
	RingAllocator*m_outboxAllocator;

	bool m_startedToCheckDeadEndOnTheLeft;
	bool m_startedToCheckDeadEndOnTheRight;


	stack<int> m_depths;
	stack<Kmer> m_vertices;
	int m_maximumAllowedDepth;
	int m_actualMaximumDepth;

	bool m_valid;
	set<Kmer> m_visited;

	bool m_searchIsStarted;

	int DIRECTION_PARENTS;
	int DIRECTION_CHILDREN;

	bool m_fetchedFirstParent;
	bool m_fetchedSecondParent;
	bool m_fetchedFirstChild;
	bool m_fetchedSecondChild;

	vector<Direction> m_leftDirections;
	vector<Direction> m_rightDirections;
	bool m_fetchedGrandparentDirections;
	bool m_fetchedGrandchildDirections;
	bool m_fetchedGrandparentReverseDirections;
	bool m_fetchedGrandchildReverseDirections;

// private methods

	bool checkDeadEndOnTheLeft();
	bool checkDeadEndOnTheRight();
	bool searchGraphForNiceThings(int direction);
	bool checkBubblePatterns();

public:
	void work();

	bool isDone();

	WorkerHandle getWorkerIdentifier();

	void initialize(uint64_t identifier, GraphPath*seed, Parameters * parameters,
		VirtualCommunicator * virtualCommunicator,
		RingAllocator * outboxAllocator,
		MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
		MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH
	);

	bool isValid();
};

#endif
