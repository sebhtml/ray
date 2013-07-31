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

#include <iostream>
using namespace std;

SeedGossipSolver::SeedGossipSolver() {

}

void SeedGossipSolver::setInput(vector<GraphSearchResult> * gossips) {
	this->m_gossips = gossips;

	for(int i = 0 ; i < (int)m_gossips->size() ; ++i) {
		GraphSearchResult & element = m_gossips->at(i);
		PathHandle & path1 = element.getPathHandles()[0];
		PathHandle & path2 = element.getPathHandles()[1];

		m_index[path1].push_back(i);

		if(path1 != path2)
			m_index[path2].push_back(i);

#ifdef CONFIG_ASSERT
		assert(path1 != path2);
#endif ///////////// CONFIG_ASSERT

	}
}

/**
 * scoping is everything
 */
void SeedGossipSolver::compute() {

	cout << "[DEBUG] computing solution..." << endl;
	cout << "[DEBUG] index size " << m_index.size() << endl;

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
	
	vector<PathHandle> & handles = partialSolution.getPathHandles();
	vector<bool> & orientations = partialSolution.getPathOrientations();

#ifdef CONFIG_ASSERT
	assert(handles.size() >= 2);
#endif

	int leftIndex = 0;
	PathHandle & leftPath = handles[leftIndex];
	bool leftStrand = orientations[leftIndex];

	/**
	 *
	 *     ???    *********--*********--**********
	 *
	 */
	if(m_index.count(leftPath) > 0) {
		vector<int> & positions = m_index[leftPath];
		
		for(int i = 0 ; i < (int) positions.size() ; ++i) {
			int index = positions[i];
			if(m_processedEntries.count(index) > 0)
				continue;

			cout << "[DEBUG] found a edge on the left" << endl;

			GraphSearchResult & edge = m_gossips->at(index);

			PathHandle & leftGossipPath = edge.getPathHandles()[0];
			PathHandle & rightGossipPath = edge.getPathHandles()[1];
			GraphPath & path = edge.getComputedPaths()[0];

			bool leftGossipStrand = edge.getPathOrientations()[0];
			bool rightGossipStrand = edge.getPathOrientations()[1];

			// Case 1.1
			// easy match
			//
			//            edge
			//
			//  ******---*******
			//             pathx
			//             strandx
			//
			//             partialSolution
			//           ********---********---**********
			//             pathx
			//             strandx
			if(rightGossipStrand == leftStrand
					&& rightGossipPath == leftPath) {

				cout << "[DEBUG] Case 1.1 direct match on the left" << endl;

				partialSolution.addPathOnLeftSide(leftGossipPath, leftGossipStrand, path);
				m_processedEntries.insert(index);

				return true;

			// Same as Case 1.1, but we need to rotate the edge.
			} else if(leftGossipStrand == !leftStrand
					&& leftGossipPath == leftPath) {

				GraphSearchResult rotatedEdge = edge;
				rotatedEdge.reverseContent();

#ifdef CONFIG_ASSERT
				assert(rotatedEdge.getPathOrientations()[1] == leftStrand);
#endif

				cout << "[DEBUG] Case 1.2 reverse match on the left" << endl;

				partialSolution.addPathOnLeftSide(rotatedEdge.getPathHandles()[0],
						rotatedEdge.getPathOrientations()[0],
						rotatedEdge.getComputedPaths()[0]);
				m_processedEntries.insert(index);

				return true;
			}
		}

	}

	int rightIndex = handles.size() - 1;
	PathHandle & rightPath = handles[rightIndex];

	if(m_index.count(rightPath) > 0) {
		//vector<int> & positions = m_index[rightPath];

	}

	return false;
}

bool SeedGossipSolver::hasFreeGossip() const {
	return m_processedEntries.size() != m_gossips->size();
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
