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
#include <map>
#include <set>
using namespace std;

/**
 * This class manages the assets (which are gossips).
 *
 * It also knows which gossip needs to be sent somewhere.
 *
 * This class dynamically computes the preference list for a gossip
 * (the preference list of a gossip contains its primary actors).
 *
 *
 * \author Sébastien Boisvert
 */
class GossipAssetManager {
	vector<GraphSearchResult> m_gossips;

	map<string, int> m_gossipIndex;

	map<int, set<Rank> > m_remoteGossipOwners;
	map<int, set<Rank> > m_futureRemoteGossipOwners;

	vector<set<int> > m_gossipClusters;

	map<PathHandle, int> m_pathToClusterTable;


	void classifyGossip(GraphSearchResult & gossip);
	bool scheduleTransportation(int gossipIndex, Rank destination);

public:
	void addGossip(GraphSearchResult & gossip);

	bool hasGossipToShare() const;

	void getGossipToShare(GraphSearchResult & gossip, Rank & destination);

	vector<GraphSearchResult> & getGossips();

	bool hasGossip(const GraphSearchResult & gossip) const;

	void registerRemoteGossip(GraphSearchResult & gossip, Rank & destination);
	bool scheduleGossipTransportation(GraphSearchResult & gossip, Rank & destination);
};

#endif
