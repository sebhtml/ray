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

#ifndef GossipAssetManagerHeader
#define GossipAssetManagerHeader

#include "GraphSearchResult.h"

#include <vector>
#include <set>
using namespace std;

/**
 * This class manages the assets (which are gossips).
 *
 * It also knows which gossip needs to be sent somewhere.
 *
 * \author Sébastien Boisvert
 */
class GossipAssetManager {
	vector<GraphSearchResult> m_gossips;

	set<string> m_gossipIndex;

public:
	void addGossip(GraphSearchResult & gossip);

	bool hasGossipToShare() const;

	void getGossipToShare(GraphSearchResult & gossip, Rank & destination);

	vector<GraphSearchResult> & getAssets();

	bool hasAsset(const string & key) const;
};

#endif
