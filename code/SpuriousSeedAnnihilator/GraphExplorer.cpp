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

#define DEBUG_EXPLORATION_123

void GraphExplorer::start(WorkerHandle key, Kmer * start, GraphPath * seed, int direction, Parameters * parameters,
	VirtualCommunicator * virtualCommunicator,
	RingAllocator * outboxAllocator,
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH,
	PathHandle seedName
) {

	m_seed = seed;
	m_start = *start;

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

	while(!m_verticesToVisit.empty())
		m_verticesToVisit.pop();

	while(!m_depths.empty())
		m_depths.pop();

	m_verticesToVisit.push(*start);

	int depth = 0;
	m_depths.push(depth);

	m_haveAttributes = false;
	m_haveAnnotations = false;
	m_haveAnnotationsReverse = false;

	WorkerHandle identifier = m_key;

	m_attributeFetcher.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);

	m_annotationFetcher.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
			RAY_MPI_TAG_ASK_VERTEX_PATH);

	m_annotationFetcherReverse.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
			RAY_MPI_TAG_ASK_VERTEX_PATH);


	m_stopAtFirstHit = true;

	m_seedName = seedName;
	m_visitedVertices = 0;
	m_maximumVisitedDepth = 0;
	m_searchDepthForFirstResult = -1;

	m_vertexDepths.clear();
	m_parents.clear();
	m_searchResults.clear();

	//cout << "[DEBUG] explorer is ready" << endl;

	m_debug = false;

#ifdef DEBUG_EXPLORATION_123
	bool tryToDebug = true;

	/**
	 * DEBUG URI http://genome.ulaval.ca:10241/client/?map=3&section=0&region=254&location=1734&depth=10
	 */

	//const char * key1 = "GGAATCATGAGAAGTCAGCCG";
	//const char * key1 = "AAATCCCTCTTTTTACAATTG";
	const char * key1 = "TTTCGTGAAAAAAGTTAACAA";
	if(tryToDebug && start->idToWord(m_parameters->getWordSize(), m_parameters->getColorSpaceMode()) == key1) {
		m_debug = true;
		cout << "[DEBUG] 8d97f6e851 BEGIN" << endl;
	}
#endif


}

// TODO: use the coverage value instead of depth
bool GraphExplorer::getBestParent(Kmer * parent, Kmer kmer) {

	if(m_parents.count(kmer) == 0)
		return false;

	int bestDepth = 99999;
	Kmer bestKmer;

	for(vector<Kmer>::iterator i = m_parents[kmer].begin();
			i != m_parents[kmer].end();
			i++) {

		Kmer theParent = *i;

		int parentDepth = m_vertexDepths[theParent];

		if(parentDepth < bestDepth) {
			bestKmer = theParent;
			bestDepth = parentDepth;
		}
	}

	*parent = bestKmer;

	return true;
}

/**
 * Here, the word "parent" is context-specific. It means the parent in the
 * course. If we use EXPLORER_LEFT, then parents are truly parents.
 * But with EXPLORER_RIGHT, parents are in fact children.
 */
void GraphExplorer::backtrackPath(vector<Kmer> * path, Kmer * vertex) {
	Kmer item = *vertex;

	set<Kmer> visited;

	vector<Kmer> aPath;

	while(1) {

		if(visited.count(item) > 0)
			break;

		aPath.push_back(item);
		visited.insert(item);

		if(item == m_start)
			break;

		Kmer parent;

		if(!getBestParent(&parent, item))
			break;

		item = parent;
	}

	// now the path include the source and the sink and also
	// is in reverse order...

#ifdef CONFIG_ASSERT
	assert(aPath.size() >= 3); // source + sink + at least one stranger.
#endif

	// remove first and last
	int position = 1;
	while(position <= (int)aPath.size() -2) {
		path->push_back(aPath[position++]);
	}

	aPath.clear();

	// reverse to enforce the de Bruijn property
	if(m_direction == EXPLORER_RIGHT) {
		int firstPosition = 0;
		int lastPosition = path->size()-1;

		// while(firstPosition < lastPosition) {  error: stray ‘\302’ in program
		while(firstPosition < lastPosition) {
			Kmer holder = (*path)[firstPosition];
			(*path)[firstPosition] = (*path)[lastPosition];
			(*path)[lastPosition] = holder;
			firstPosition ++;
			lastPosition --;
		}
	}
}

bool GraphExplorer::processAnnotations(AnnotationFetcher & annotationFetcher, int currentDepth, Kmer & object) {

	bool foundSomething = false;
	for(int i=0;i< (int) annotationFetcher.getDirections()->size(); i++){

		Direction & direction = annotationFetcher.getDirections()->at(i);

		PathHandle pathName = direction.getPathHandle();
		int position = direction.getPosition();
		bool pathStrand = false;
		if(position != 0)
			pathStrand = true;

		// the self path will always be found at depth 0
		// Streptococcus pneumoniae has a lot of these loops where a seeds touch itself via
		// a short 2X-coverage region.
		// This was seen in Ray Cloud Browser.

		if(currentDepth != 0) {
#ifdef INTERNET_EXPLORER_DEBUG_PATHS
			cout << "[DEBUG] GraphExplorer found path " << pathName << " during graph search";
			cout << ", visited " << m_visitedVertices << ", started from " << m_seedName;

			cout << " direction ";

			if(m_direction == EXPLORER_LEFT)
				cout << "EXPLORER_LEFT";
			else if(m_direction == EXPLORER_RIGHT)
				cout << "EXPLORER_RIGHT";

			cout << " depth " << currentDepth;
#endif

			foundSomething = true;


			// here we can not use GraphPath directly because the de Bruijn property
			// is hardly enforced in both directions
			vector<Kmer> pathToOrigin;

			backtrackPath(&pathToOrigin, &object);

#ifdef INTERNET_EXPLORER_DEBUG_PATHS
			cout << " path has length " << pathToOrigin.size() << endl;
#endif

			GraphPath aPath;
			aPath.setKmerLength(m_parameters->getWordSize());
			for(int i = 0 ; i < (int)pathToOrigin.size() ; i++) {
				Kmer kmer = pathToOrigin[i];
				aPath.push_back(&kmer);
			}

			GraphSearchResult result;

			if(m_direction == EXPLORER_RIGHT) {
				result.addPathHandle(m_seedName, false);
				result.addPath(aPath);
				result.addPathHandle(pathName, pathStrand);
			} else if(m_direction == EXPLORER_LEFT) {
				result.addPathHandle(pathName, pathStrand);
				result.addPath(aPath);
				result.addPathHandle(m_seedName, false);
			}

			m_searchResults.push_back(result);
		}
	}

	return foundSomething;
}

// TODO: query also the other DNA strand for annotations
bool GraphExplorer::work() {

	if(m_verticesToVisit.empty())
		m_done = true;

	if(m_done) {

#ifdef DEBUG_EXPLORATION_123
		if(m_debug) {
			cout << "[DEBUG] 8d97f6e851 completed, m_visitedVertices " << m_visitedVertices;
			cout << " path " << m_seedName;
			cout << " m_searchDepthForFirstResult " << m_searchDepthForFirstResult;
			cout << " m_maximumVisitedDepth " << m_maximumVisitedDepth;
			cout << " lengthInKmers " << m_seed->size();
			cout << " direction ";


			if(m_direction == EXPLORER_LEFT)
				cout << "EXPLORER_LEFT";
			else
				cout << "EXPLORER_RIGHT";

			cout << " search results: " << m_searchResults.size();
			cout << endl;
		}
#endif

		return m_done;
	}

#ifdef ASSERT
	assert(!m_verticesToVisit.empty());
	assert(!m_depths.empty());
#endif

	Kmer object = m_verticesToVisit.top();
	Kmer reverseObject = object.complementVertex(m_parameters->getWordSize(), m_parameters->getColorSpaceMode());

	if(!m_haveAttributes && m_attributeFetcher.fetchObjectMetaData(&object)) {

		m_haveAttributes = true;
		m_haveAnnotations = false;
		//cout << "[DEBUG] have attributes" << endl;

	} else if(m_haveAttributes && !m_haveAnnotations && m_annotationFetcher.fetchDirections(&object)) {

		m_haveAnnotations = true;
		m_haveAnnotationsReverse = false;

	} else if(m_haveAttributes && m_haveAnnotations && !m_haveAnnotationsReverse
		&& m_annotationFetcherReverse.fetchDirections(&reverseObject)) {

		m_haveAnnotationsReverse = true;
		//cout << "[DEBUG] have annotations" << endl;

	} else if(m_haveAttributes && m_haveAnnotations && m_haveAnnotationsReverse) {

#ifdef DEBUG_EXPLORATION_123
		if(m_debug) {
			cout << "[DEBUG] 8d97f6e851 vertex " << object.idToWord(m_parameters->getWordSize(), m_parameters->getColorSpaceMode());
			cout << " depth " << m_attributeFetcher.getDepth() << " children [ ";

			for(int i = 0 ; i < (int) m_attributeFetcher.getChildren()->size() ; ++i) {

				cout << " " << m_attributeFetcher.getChildren()->at(i).idToWord(m_parameters->getWordSize(), m_parameters->getColorSpaceMode());
			}
			cout << " ]";

			cout << " forward annotations: " << m_annotationFetcher.getDirections()->size();
			cout << " reverse annotations: " << m_annotationFetcherReverse.getDirections()->size();
			cout << endl;
		}
#endif

		int currentDepth = m_depths.top();

		m_vertexDepths[object] = currentDepth;

		bool foundSomething = false;

		if(currentDepth > m_maximumVisitedDepth)
			m_maximumVisitedDepth = currentDepth;

		//cout << "[DEBUG] processing object now depth=" << currentDepth << " visited= " << m_visitedVertices << endl;

		if(processAnnotations(m_annotationFetcher, currentDepth, object))
			foundSomething = true;
		if(processAnnotations(m_annotationFetcherReverse, currentDepth, object))
			foundSomething = true;

		if(foundSomething && m_searchDepthForFirstResult < 0)
			m_searchDepthForFirstResult = currentDepth;

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

				if(!foundSomething) {
					Kmer nextKmer = links->at(i);

					// implemented already: check if there is not already another parent.
					// if it is the case, select the path with the coverage
					// that is the nearest to the one of both paths
					m_parents[nextKmer].push_back(object);

					m_verticesToVisit.push(nextKmer);
					m_depths.push(newDepth);
				}
			}
		}

		m_annotationFetcher.reset();
		m_annotationFetcherReverse.reset();
		m_attributeFetcher.reset();

		m_visitedVertices ++;

		m_haveAttributes = false;
	}

	return m_done;
}

vector<GraphSearchResult> & GraphExplorer::getSearchResults() {
	return m_searchResults;
}

/**
 * \see http://stackoverflow.com/questions/13639535/what-are-the-naming-conventions-of-functions-that-return-boolean
 */
bool GraphExplorer::isValid() const {

	if(m_searchResults.size() != 1)
		return false;

	if(m_visitedVertices >= m_maximumVisitedVertices)
		return false;

	if(m_maximumVisitedDepth >= m_maximumDepth)
		return false;

	return true;
}
