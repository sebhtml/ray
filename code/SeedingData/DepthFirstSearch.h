/*
 * Ray -- Parallel genome assemblies for parallel DNA sequencing
 * Copyright 2014 Sébastien Boisvert
 *
 * This file is part of Ray.
 *
 * Ray Surveyor is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 *
 * Ray Surveyor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ray Surveyor.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DepthFirstSearch
#define _DepthFirstSearch

#include <code/KmerAcademyBuilder/Kmer.h>
#include <code/Mock/Parameters.h>
#include <code/SpuriousSeedAnnihilator/AttributeFetcher.h>

#include <RayPlatform/communication/VirtualCommunicator.h>
#include <RayPlatform/scheduling/Worker.h>

#include <set>
#include <stack>
using namespace std;

/**
 *
 * Perform a depth first search in a distributed
 * de Bruijn graph.
 *
 * \author Sébastien Boisvert
 *
 */
class DepthFirstSearch {

private:

	AttributeFetcher m_attributeFetcher;

	set<Kmer> m_visitedVertices;
	stack<Kmer> m_verticesToVisit;
	stack<int> m_depths;

	bool m_doParents;
	bool m_doChildren;

	int m_maximumDepth;
	int m_maximumNumberOfVisitedVertices;

	int m_actualMaximumDepth;

	void reset();

public:

	DepthFirstSearch();
	~DepthFirstSearch();

	/**
	 * Initialize component
	 */
	void initialize(Parameters*parameters, VirtualCommunicator*virtualCommunicator,
			WorkerHandle identifier, RingAllocator * outboxAllocator,
			MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);


	/**
	 * Start on a kmer
	 * This initiates the depth first search.
	 */
	void start(Kmer & kmer, bool checkParents, bool checkChildren, int maximumDepth,
			int maximumNumberOfVertices);

	/**
	 * Perform a unit of work.
	 */
	void work();

	/**
	 * Check if the work is completed.
	 */
	bool isDone() const;

	/**
	 * Check if the maximum depth was reached
	 */
	bool hasReachedMaximumDepth() const;
};

#endif
