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
/*
	cout << "[DEBUG] computing solution..." << endl;
	cout << "[DEBUG] index size " << m_index.size() << endl;
	*/


	while(hasFreeGossip()) {

		GraphSearchResult & entry = getFreeGossip();

		GraphSearchResult workingCopy = entry;

		while(expand(workingCopy)) {
		}

		m_solution.push_back(workingCopy);
	}

	printSolution();
}

void SeedGossipSolver::printSolution() {

	cout << "DEBUG [printSolution]" << endl;

#if 0
	cout << "Solution has: " << m_solution.size() << " entries" << endl;
	cout << "Initial gossips: " << m_gossips->size() << endl;
	cout << "Objects: " << m_index.size() << endl;

	cout << "edges: " << endl;
#endif


#if 0
	int iterator = 0;
	for(vector<GraphSearchResult>::iterator i = m_gossips->begin();
			i != m_gossips->end() ; ++i) {
		cout << "[DEBUG] gossip:" << iterator++ << " ";
		(*i).print();

		cout << endl;
	}
#endif

	int objectsInSolution = 0;

	map<PathHandle, vector<int> > summary;

	cout << "solution" << endl;

	for(int i = 0 ; i < (int) m_solution.size() ; ++i) {
		//cout << "[DEBUG] @" << i << " ";

		int total = m_solution[i].getPathHandles().size();
		cout << total << " ... ";
		m_solution[i].print();
		cout << endl;

		objectsInSolution += total;

		vector<PathHandle> & handles = m_solution[i].getPathHandles();

		for(vector<PathHandle>::iterator j = handles.begin() ;
				j!= handles.end() ; ++j) {

			PathHandle & handle = *j;
			summary[handle].push_back(i);
		}
	}

	//cout << "[DEBUG] list of GraphSearchResult entries" << endl;

#if 0
	for(map<PathHandle, vector<int> >::iterator i = summary.begin();
			i!= summary.end() ; ++i) {

		vector<int> & matches = i->second;

		if(matches.size() != 1) {
			cout << "[DEBUG] " << i->first << " is in ";

			for(vector<int>::iterator j = matches.begin();
					j != matches.end() ; ++j){
				cout << " " << *j;
			}
			cout << endl;
			cout << "[DEBUG] first gossip that contains the path: ";
			m_solution.at(matches[0]).print();
			cout << endl;
		}
	}
#endif

	//cout << "[DEBUG] objects found in solution ...... " << endl;
	cout << objectsInSolution << endl;
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

			//cout << "[DEBUG] found a edge on the left" << endl;

			GraphSearchResult & edge = m_gossips->at(index);

			PathHandle & leftGossipPath = edge.getPathHandles()[0];
			PathHandle & rightGossipPath = edge.getPathHandles()[1];

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

				//cout << "[DEBUG] Case 1.1 direct match on the left" << endl;

				GraphPath & path = edge.getComputedPaths()[0];

				partialSolution.addPathOnLeftSide(leftGossipPath, leftGossipStrand, path);
				m_processedEntries.insert(index);

				return true;

			// Case 1.2
			// Same as Case 1.1, but we need to rotate the edge.
			} else if(leftGossipStrand == !leftStrand
					&& leftGossipPath == leftPath) {

				GraphSearchResult rotatedEdge = edge;
				rotatedEdge.reverseContent();

#ifdef CONFIG_ASSERT
				assert(rotatedEdge.getPathOrientations()[1] == leftStrand);
#endif

				//cout << "[DEBUG] Case 1.2 reverse match on the left" << endl;

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
	bool rightStrand = orientations[rightIndex];

	/**
	 *
	 *     *********--*********--**********---  ???????
	 *
	 */
	if(m_index.count(rightPath) > 0) {
		vector<int> & positions = m_index[rightPath];

		for(int i = 0 ; i < (int) positions.size() ; ++i) {
			int index = positions[i];
			if(m_processedEntries.count(index) > 0)
				continue;

			//cout << "[DEBUG] found a edge on the right" << endl;

			GraphSearchResult & edge = m_gossips->at(index);

			PathHandle & leftGossipPath = edge.getPathHandles()[0];
			PathHandle & rightGossipPath = edge.getPathHandles()[1];

			bool leftGossipStrand = edge.getPathOrientations()[0];
			bool rightGossipStrand = edge.getPathOrientations()[1];

			// Case 2.1
			// easy match
			//
			//            edge
			//
			//                                      ******---*******
			//                                       pathx
			//                                      strandx
			//
			//             partialSolution
			//           ********---********---**********
			//                                       pathx
			//                                      strandx
			if(leftGossipStrand == rightStrand 
					&& leftGossipPath == rightPath) {

				//cout << "[DEBUG] Case 2.1 direct match on the right" << endl;

				GraphPath & path = edge.getComputedPaths()[0];
				partialSolution.addPathOnRightSide(rightGossipPath, rightGossipStrand, path);
				m_processedEntries.insert(index);

				return true;

			// Case 2.2
			// Same as Case 2.1, but we need to rotate the edge.
			} else if(rightGossipStrand == !rightStrand
					&& rightGossipPath == rightPath) {

				GraphSearchResult rotatedEdge = edge;
				rotatedEdge.reverseContent();

#ifdef CONFIG_ASSERT
				assert(rotatedEdge.getPathOrientations()[0] == rightStrand);
				assert(rotatedEdge.getPathHandles()[0] == rightPath);
#endif

				//cout << "[DEBUG] Case 2.2 reverse match on the right" << endl;

				partialSolution.addPathOnRightSide(rotatedEdge.getPathHandles()[1],
						rotatedEdge.getPathOrientations()[1],
						rotatedEdge.getComputedPaths()[0]);
				m_processedEntries.insert(index);

				return true;
			}
		}

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
