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

#include "SeedGossipSolver.h"


SeedGossipSolver::SeedGossipSolver() {

}

void SeedGossipSolver::setInput(vector<GraphSearchResult> * gossips) {
	this->m_gossips = gossips;

	for(int i = 0 ; i < (int)m_gossips->size() ; ++i) {
		GraphSearchResult & element = m_gossips->at(i);
		PathHandle & path1 = element.getPathHandles()[0];
		PathHandle & path2 = element.getPathHandles()[1];

		m_index[path1].push_back(&element);

		if(path1 != path2)
			m_index[path2].push_back(&element);

#ifdef CONFIG_ASSERT
		assert(path1 != path2);
#endif ///////////// CONFIG_ASSERT

	}
}

/**
 * scoping is everything
 */
void SeedGossipSolver::compute() {

	while(hasFreeGossip()) {

		GraphSearchResult & entry = getFreeGossip();

		GraphSearchResult workingCopy = entry;

		while(expand(workingCopy)) {
		}

		m_solution.push_back(workingCopy);
	}
}

/**
 * TODO implement this method
 */
bool SeedGossipSolver::expand(GraphSearchResult & partialSolution) {

	// take the two paths, and look for them in the index
	return false;
}

bool SeedGossipSolver::hasFreeGossip() const {
	return m_processedEntries.size() == m_gossips->size();
}

GraphSearchResult & SeedGossipSolver::getFreeGossip() {
	for(int i = 0 ; i < (int) m_gossips->size() ; ++i) {
		bool used = m_processedEntries.count(i) > 0;

		if(used)
			continue;

		GraphSearchResult & entry = m_gossips->at(i);

#ifdef CONFIG_ASSERT
		assert(m_processedEntries.count(i) == 0);
#endif ////////// CONFIG_ASSERT

		m_processedEntries.insert(i);

		return entry;
	}

	return m_dummy;
}

vector<GraphSearchResult> & SeedGossipSolver::getSolution() {
	return m_solution;
}

void SeedGossipSolver::destroy() {
	m_solution.clear();
	m_index.clear();
	m_gossips = NULL;
	m_processedEntries.clear();
}

bool SeedGossipSolver::hasPathHandle(PathHandle & pathHandle) {
	return m_index.count(pathHandle) > 0;
}
