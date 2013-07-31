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

#ifndef SeedGossipSolverHeader
#define SeedGossipSolverHeader

#include "GraphSearchResult.h"

#include <code/SeedingData/PathHandle.h>

#include <map>
#include <set>
#include <vector>
using namespace std;

/**
 *
 * This is a solver for seed gossips.
 *
 * \author Sébastien Boisvert
 */
class SeedGossipSolver {

	map<PathHandle, vector<GraphSearchResult*> > m_index;
	vector<GraphSearchResult> * m_gossips;
	vector<GraphSearchResult> m_solution;
	set<int> m_processedEntries;
	GraphSearchResult m_dummy;

	bool expand(GraphSearchResult & partialSolution);

	bool hasFreeGossip() const;
	GraphSearchResult & getFreeGossip();

public:

	SeedGossipSolver();
	void setInput(vector<GraphSearchResult> * gossips);
	void compute();
	vector<GraphSearchResult> & getSolution();
	void destroy();
	bool hasPathHandle(PathHandle & pathHandle);
};

#endif ////////////// SeedGossipSolverHeader
