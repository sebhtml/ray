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

#ifndef _NanoMerger_h
#define _NanoMerger_h

#include "GraphExplorer.h"
#include "GraphSearchResult.h"

#include <code/SeedingData/GraphPath.h>
#include <code/Mock/Parameters.h>
#include <code/SeedExtender/Direction.h>

#include <RayPlatform/scheduling/Worker.h>

#include <stack>
#include <vector>
#include <set>
using namespace std;

#include <stdint.h>

/**
 * This is a worker that analyze a seed.
 *
 * This class can not use ./SeedExtender/DepthFirstSearchData.h because
 * ./SeedExtender/DepthFirstSearchData.h does not fetch annotations
 * and is not 100% compatible with small workers.
 *
 * \author Sébastien Boisvert
 */
class NanoMerger: public Worker{

	bool m_startedFirst;
	bool m_startedLast;
	PathHandle m_seedName;
	GraphSearchResult m_firstGraphSearchResult;
	GraphSearchResult m_lastGraphSearchResult;

	GraphExplorer m_explorer;

	Rank m_rank;
	RingAllocator*m_outboxAllocator;

	uint64_t m_identifier;       // TODO this should be in Worker because it's always there anyway
	bool m_done;          // TODO this should be in Worker because it's always there anyway
	GraphPath * m_seed;

	VirtualCommunicator * m_virtualCommunicator; // TODO this should be in Worker because it's always there anyway
	Parameters * m_parameters;

	int m_step;

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;

/*
	stack<int> m_depths;
	stack<Kmer> m_vertices;
	int m_maximumAllowedDepth;
	int m_actualMaximumDepth;
	int m_maximumDepthForExploration;

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
	bool m_isPerfectBubble;
*/
// private methods

	/*
	bool checkDeadEndOnTheLeft();
	bool checkDeadEndOnTheRight();
	bool searchGraphForNiceThings(int direction);
	bool checkBubblePatterns();

	bool getOtherBestParent(Kmer*kmer);
	bool getOtherBestChild(Kmer*kmer);
	bool getBestChild(Kmer*kmer);
	bool getBestParent(Kmer*kmer);
*/
	Kmer m_first;
	Kmer m_last;
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
