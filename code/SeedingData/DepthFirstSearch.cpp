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

#include "DepthFirstSearch.h"

DepthFirstSearch::DepthFirstSearch() {

}

DepthFirstSearch::~DepthFirstSearch() {

}

void DepthFirstSearch::initialize(Parameters*parameters, VirtualCommunicator*virtualCommunicator,
		WorkerHandle identifier, RingAllocator * outboxAllocator,
		MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT) {


	m_attributeFetcher.initialize(parameters, virtualCommunicator, identifier, outboxAllocator,
			RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);

	reset();
}

void DepthFirstSearch::reset() {

	m_visitedVertices.clear();

	// use copy constructor instead of clear because
	// class stack has no clear method
	// which is strange
	// \see http://www.gamedev.net/topic/527439-c-stdstack--why-no-clear-method/
	stack<Kmer> empty1;
	m_verticesToVisit = empty1;
	stack<int> empty2;
	m_depths = empty2;

	m_attributeFetcher.reset();

	m_actualMaximumDepth = 0;
}

void DepthFirstSearch::start(Kmer & kmer, bool checkParents, bool checkChildren, int maximumDepth,
		int maximumNumberOfVertices) {

	reset();

	int depth = 0;  // 0 edges

	m_verticesToVisit.push(kmer);
	m_depths.push( depth );

	m_doParents = checkParents;
	m_doChildren = checkChildren;

	m_maximumDepth = maximumDepth;
	m_maximumNumberOfVisitedVertices = maximumNumberOfVertices;
}

void DepthFirstSearch::work() {

	return;
}

bool DepthFirstSearch::isDone() const {

	return true;
}

bool DepthFirstSearch::hasReachedMaximumDepth() const {

	return true;


	return m_actualMaximumDepth >= m_maximumDepth;
}
