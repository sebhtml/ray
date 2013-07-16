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

#include "GraphExplorer.h"

void GraphExplorer::start(WorkerHandle key, Kmer * start, int direction, Parameters * parameters,
	VirtualCommunicator * virtualCommunicator,
	RingAllocator * outboxAllocator,
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH
) {

	//cout << "[DEBUG] starting graph search with explorer technology" << endl;

	m_key = key;
	m_direction = direction;

	m_parameters = parameters;
	m_virtualCommunicator = virtualCommunicator;
	m_outboxAllocator = outboxAllocator;

	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	this->RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE = RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	this->RAY_MPI_TAG_ASK_VERTEX_PATH = RAY_MPI_TAG_ASK_VERTEX_PATH;

	m_done = false;
	m_maximumDepth = 128;
	m_maximumVisitedVertices = 1024;

	while(!m_currentPath.empty())
		m_currentPath.pop();

	while(!m_verticesToVisit.empty())
		m_verticesToVisit.pop();

	while(!m_depths.empty())
		m_depths.pop();

	m_verticesToVisit.push(*start);

	int depth = 0;
	m_depths.push(depth);

	m_haveAttributes = false;
	m_haveAnnotations = false;

	WorkerHandle identifier = m_key;

	m_attributeFetcher.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);

	m_annotationFetcher.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
			RAY_MPI_TAG_ASK_VERTEX_PATH);

	m_stopAtFirstHit = true;

	int seedIndex = m_key;

	m_seedName = getPathUniqueId(m_parameters->getRank(), seedIndex);
	m_visitedVertices = 0;
	m_maximumVisitedDepth = 0;

	//cout << "[DEBUG] explorer is ready" << endl;
}

// TODO: query also the other DNA strand for annotations
bool GraphExplorer::work() {

	if(m_verticesToVisit.empty())
		m_done = true;

	if(m_done) {
		cout << "[DEBUG] visited " << m_visitedVertices << endl;

		return m_done;
	}

#ifdef ASSERT
	assert(!m_verticesToVisit.empty());
	assert(!m_depths.empty());
#endif

	Kmer object = m_verticesToVisit.top();

	if(!m_haveAttributes && m_attributeFetcher.fetchObjectMetaData(&object)) {

		m_haveAttributes = true;
		m_haveAnnotations = false;
		//cout << "[DEBUG] have attributes" << endl;

	} else if(m_haveAttributes && !m_haveAnnotations && m_annotationFetcher.fetchDirections(&object)) {

		m_haveAnnotations = true;

		//cout << "[DEBUG] have annotations" << endl;

	} else if(m_haveAttributes && m_haveAnnotations) {

		int currentDepth = m_depths.top();

		if(currentDepth > m_maximumVisitedDepth)
			m_maximumVisitedDepth = currentDepth;

		//cout << "[DEBUG] processing object now depth=" << currentDepth << " visited= " << m_visitedVertices << endl;

		for(int i=0;i< (int) m_annotationFetcher.getDirections()->size(); i++){

			PathHandle pathName = m_annotationFetcher.getDirections()->at(i).getWave();

			if(pathName != m_seedName) {
				cout << "[DEBUG] GraphExplorer found path " << pathName << " during graph search";
				cout << ", visited " << m_visitedVertices << ", started from " << m_seedName;
				cout << endl;

				cout << "[DEBUG] direction ";

				if(m_direction == EXPLORER_LEFT)
					cout << "EXPLORER_LEFT";
				else if(m_direction == EXPLORER_RIGHT)
					cout << "EXPLORER_RIGHT";

				cout << " depth " << currentDepth;

				cout << endl;
			}
		}

#ifdef ASSERT
		assert(!m_depths.empty());
#endif

		int newDepth = currentDepth + 1;

		m_depths.pop();
		m_verticesToVisit.pop();

		vector<Kmer> * links = NULL;

		if(m_direction == EXPLORER_LEFT)
			links = m_attributeFetcher.getParents();
		else if(m_direction == EXPLORER_RIGHT)
			links = m_attributeFetcher.getChildren();

#ifdef ASSERT
		assert(links != NULL);
#endif

		if(newDepth <= m_maximumDepth
			&& m_visitedVertices + (int)links->size() <= m_maximumVisitedVertices) {

			for(int i = 0 ; i < (int)links->size() ; i ++) {
				Kmer nextKmer = links->at(i);
				m_verticesToVisit.push(nextKmer);
				m_depths.push(newDepth);
			}
		}

		m_annotationFetcher.reset();
		m_attributeFetcher.reset();

		m_visitedVertices ++;

		m_haveAttributes = false;
	}

	return m_done;
}


