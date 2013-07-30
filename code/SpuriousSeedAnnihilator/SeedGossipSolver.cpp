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

void SeedGossipSolver::compute() {
	map<int, bool> processedEntries;


}

vector<GraphSearchResult> & SeedGossipSolver::getSolution() {
	return m_solution;
}

void SeedGossipSolver::destroy() {
	m_solution.clear();
	m_index.clear();
	m_gossips = NULL;
}

bool SeedGossipSolver::hasPathHandle(PathHandle & pathHandle) {
	return m_index.count(pathHandle) > 0;
}
