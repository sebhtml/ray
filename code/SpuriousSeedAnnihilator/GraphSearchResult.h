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

#ifndef GraphSearchResult_Header
#define GraphSearchResult_Header

#include <code/SeedingData/PathHandle.h>
#include <code/Mock/CarriageableItem.h>
#include <code/SeedingData/GraphPath.h>

#include <vector>
using namespace std;

/**
 * This stores a graph search result.
 *
 *     Path0  ---------->  Path1  ------> Path2   ------->  Path3
 *
 *     start              start         start               start
 *
 *        end                end             end                end
 *
 *
 * There are:
 *
 * * N m_pathNames
 * * N-1 m_computedPaths
 * * N m_pathStarts
 * * N m_pathEnds
 *
 * if start > end, it means that we must use the other DNA strand.
 *
 * \author Sébastien Boisvert
 */
class GraphSearchResult: public CarriageableItem {

	vector<PathHandle> m_pathHandles;
	vector<bool> m_pathOrientations; // false is normal, true is reverse
	vector<GraphPath> m_computedPaths;

public:

	bool addPathHandle(PathHandle handle, bool orientation);
	bool addPath(GraphPath & path);

	void print();

	int load(const uint8_t * buffer);
	int dump(uint8_t * buffer) const;

	vector<PathHandle> & getPathHandles();
	vector<bool> & getPathOrientations();
	vector<GraphPath> & getComputedPaths();

	void addPathOnLeftSide(PathHandle & handle, bool strand, GraphPath & path);
	void addPathOnRightSide(PathHandle & handle, bool strand, GraphPath & path);

	string toString() const;

	/**
	 * Rotate the object to obtain a reverse-complement.
	 */
	void reverseContent();

	bool hasData() const;
};

#endif /* GraphSearchResult_Header */
