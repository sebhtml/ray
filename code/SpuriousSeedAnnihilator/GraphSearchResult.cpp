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

#include "GraphSearchResult.h"

#include <iostream>
#include <sstream>
#include <string>
using namespace std;

#include <string.h>

bool GraphSearchResult::addPathHandle(PathHandle handle, bool orientation) {

	// we need a path after each handle
	if(m_computedPaths.size() != m_pathHandles.size())
		return false;

	m_pathHandles.push_back(handle);
	m_pathOrientations.push_back(orientation);

	return true;
}

bool GraphSearchResult::addPath(GraphPath & path) {

	if(m_pathHandles.size() - 1 != m_computedPaths.size())
		return false;

	m_computedPaths.push_back(path);

	return true;

}

void GraphSearchResult::print() {
	int i = 0;

	while(i < (int)m_pathHandles.size()) {
		cout << " [" << m_pathHandles[i] << ":" << m_pathOrientations[i] << "]";

		if(i != (int)m_pathHandles.size() -1) {

			cout << " " << m_computedPaths[i].size();
		}
		i++;
	}
}

int GraphSearchResult::load(const uint8_t * buffer) {

	int position = 0;

	uint32_t paths = 0;

	int size = sizeof(uint32_t);

	memcpy(&paths, buffer + position, size);
	position += size;

	//cout << "[DEBUG] GraphSearchResult::load paths " << paths << endl;

	for(int i = 0 ; i < (int)paths; i++) {
		PathHandle entry;
		position += entry.load(buffer + position);
		m_pathHandles.push_back(entry);
	}

	for(int i = 0 ; i < (int)paths; i++) {
		size = sizeof(bool);
		bool strand;
		memcpy(&strand, buffer + position, size);
		position += size;

		m_pathOrientations.push_back(strand);
	}

	int computedPaths = paths - 1;

	for(int i = 0 ; i < computedPaths ; i ++) {
		GraphPath aParticularPath;
		size = aParticularPath.load(buffer + position);
		m_computedPaths.push_back(aParticularPath);
		position += size;
	}

	return position;
}

int GraphSearchResult::dump(uint8_t * buffer) const {

	int position = 0;

	uint32_t paths = m_pathHandles.size();
	int size = sizeof(uint32_t);

	//cout << "[DEBUG] GraphSearchResult::dump paths " << paths << endl;

	memcpy(buffer + position, &paths, size);
	position += size;

	for(int i = 0 ; i < (int)paths ; i++) {
		position += m_pathHandles[i].dump(buffer + position);
	}

	for(int i = 0 ; i < (int)paths ; i ++) {
		size = sizeof(bool);
		bool value = m_pathOrientations[i];
		memcpy(buffer + position, &value, size);
		position += size;
	}

	int computedPaths = paths - 1;

	for(int i = 0 ; i < computedPaths ; i ++) {
#ifdef CONFIG_ASSERT
		if(i >= (int)m_computedPaths.size()) {
			cout << "Error i " << i << " m_computedPaths.size() ";
			cout << m_computedPaths.size() << " computedPaths " << computedPaths << endl;
		}

		assert(i >= 0);
		assert(i < (int)m_computedPaths.size());
#endif /* CONFIG_ASSERT */

		position += m_computedPaths[i].dump(buffer + position);
	}

	return position;
}

vector<PathHandle> & GraphSearchResult::getPathHandles() {
	return m_pathHandles;
}

string GraphSearchResult::toString() const {

	ostringstream value;

	if(m_pathHandles.size() != 2)
		return "Error-not-implemented";

	PathHandle path1 = m_pathHandles[0];
	PathHandle path2 = m_pathHandles[1];

	if(path1 < path2)
		value << path1 << "-" << path2;
	else
		value << path2 << "-" << path1;

	return value.str();
}

bool GraphSearchResult::hasData() const {
	return m_pathHandles.size() > 0;
}
