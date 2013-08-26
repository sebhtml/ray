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

#include "GossipAssetManager.h"

void GossipAssetManager::addGossip(GraphSearchResult & gossip) {

	m_gossips.push_back(gossip);

	string key = gossip.toString();

	m_gossipIndex.insert(key);
}

bool GossipAssetManager::hasGossipToShare() const {

	return false;
}

void GossipAssetManager::getGossipToShare(GraphSearchResult & gossip, Rank & destination) {

}

vector<GraphSearchResult> & GossipAssetManager::getAssets() {
	return m_gossips;
}

bool GossipAssetManager::hasAsset(const string & key) const {
	return m_gossipIndex.count(key) > 0;
}
