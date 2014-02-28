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

#include <code/Mock/Logger.h>

#ifdef CONFIG_ASSERT
#include <assert.h>
#endif

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
	m_verticesToVisit = stack<Kmer>();
	m_depths = stack<int>();

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

	if(!m_doChildren && !m_doParents) {
		return;
	}

	if(isDone())
		return;

	Kmer kmer = m_verticesToVisit.top();
	int depth = m_depths.top();

	if(!m_attributeFetcher.fetchObjectMetaData(&kmer)) {

		// work a bit

	} else {

		if(depth > m_actualMaximumDepth)
			m_actualMaximumDepth = depth;

		vector<Kmer> * parents = m_attributeFetcher.getParents();
		vector<Kmer> * children = m_attributeFetcher.getChildren();
		//CoverageDepth coverageDepth = m_attributeFetcher.getDepth();

		m_verticesToVisit.pop();
		m_depths.pop();

		int newDepth = depth + 1;

		// insert visited vertex that before processing edges
		// in the case it is its own parent or child
		m_visitedVertices.insert(kmer);

		if(m_doChildren) {

			processEdges(children, newDepth);
		}

		if(m_doParents) {

			processEdges(parents, newDepth);
		}

		m_attributeFetcher.reset();
	}
}

void DepthFirstSearch::processEdges(vector<Kmer> * vertices, int depth) {

	if(depth > m_maximumDepth)
		return;

	if((int)m_visitedVertices.size() >= m_maximumNumberOfVisitedVertices)
		return;
	for(vector<Kmer>::iterator i = vertices->begin();
			i != vertices->end();
			++i) {

		Kmer kmer = *i;

		if(m_visitedVertices.count(kmer) > 0)
			continue;

		m_verticesToVisit.push(kmer);
		m_depths.push(depth);
	}
}

bool DepthFirstSearch::isDone() const {

#ifdef CONFIG_ASSERT
	assert(m_verticesToVisit.size() ==  m_depths.size());
#endif

	bool result = m_verticesToVisit.size() == 0;

	return result;
}

bool DepthFirstSearch::hasReachedMaximumDepth() const {

#if 0
	bool result = true;
	if(result) {
		cout << "DEBUG ";
		cout << " parents " << m_doParents;
		cout << " children " << m_doChildren;
		cout << " DepthFirstSearch visited " << m_visitedVertices.size();
		cout << "/" << m_maximumNumberOfVisitedVertices << " ";
		cout << " " << " depth " << m_actualMaximumDepth << "/";
		cout << m_maximumDepth << endl;
	}
#endif

	return m_actualMaximumDepth >= m_maximumDepth;
}
