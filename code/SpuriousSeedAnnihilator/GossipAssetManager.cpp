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

// this code is developped with defensive programming. (assertions)
// -Sébastien Boisvert

// #define CONFIG_DEBUG_GOSSIP_ASSET_MANAGER "Yes !!!"

#include "GossipAssetManager.h"

#include <code/Mock/common_functions.h>

#include <iostream>
using namespace std;

void GossipAssetManager::addGossip(GraphSearchResult & gossip) {

	//------------------------------------------------------------//
	// step add the asset and get a handle for it (an index)
	m_gossips.push_back(gossip);

	string key = gossip.toString();

#ifdef CONFIG_ASSERT
	assert(m_gossipIndex.count(key) == 0);
#endif

	int index = m_gossips.size() - 1;
	m_gossipIndex[key] = index;

	classifyGossip(gossip);
}

void GossipAssetManager::classifyGossip(GraphSearchResult & gossip) {

	string key = gossip.toString();

	int index = m_gossipIndex[key];

	bool mergedTwoClusters = false;

	//------------------------------------------------------------//
	// give a better name to the index for the rest of this method.
	int & newGossipIndex = index;

	PathHandle & path1 = gossip.getPathHandles()[0];
	PathHandle & path2 = gossip.getPathHandles()[1];

	//------------------------------------------------------------//
	// Now, we add the gossip to an existing cluster or to a new one.
	//
	// step 3: try to merge the newCluster to existing clusters
	// step 3.1 -- path1 and path2 in the gossip in newCluster have 0 match
	if(m_pathToClusterTable.count(path1) == 0 && m_pathToClusterTable.count(path2) == 0) {
		set<int> newCluster;
		newCluster.insert(index);
		m_gossipClusters.push_back(newCluster);

		int clusterIndex = m_gossipClusters.size() - 1;

		m_pathToClusterTable[path1] = clusterIndex;
		m_pathToClusterTable[path2] = clusterIndex;

	// step 3.2 -- path1 in the gossip newCluster has 1 match, but path2 in in the gossip in newCluster have 0 match
	} else if(m_pathToClusterTable.count(path1) > 0 && m_pathToClusterTable.count(path2) == 0) {

		// add the gossip in cluster 1 and also
		// propagate the forward index for path2 -> oldCluster

		int clusterIndex = m_pathToClusterTable[path1];

		set<int> & clusterContent = m_gossipClusters[clusterIndex];

		clusterContent.insert(newGossipIndex);
		m_pathToClusterTable[path2] = clusterIndex;

	// step 3.3 path1 has 0 matches but path2 has 1 match
	} else if(m_pathToClusterTable.count(path1) == 0 && m_pathToClusterTable.count(path2) > 0) {

		int clusterIndex = m_pathToClusterTable[path2];

		set<int> & clusterContent = m_gossipClusters[clusterIndex];

		clusterContent.insert(newGossipIndex);

		m_pathToClusterTable[path1] = clusterIndex;

	// case 3.4 both path have match in the same cluster
	} else if(m_pathToClusterTable.count(path1) > 0
			&& m_pathToClusterTable.count(path2) > 0
			&& m_pathToClusterTable[path1] == m_pathToClusterTable[path2]) {

		int clusterIndexForPath1 = m_pathToClusterTable[path1];
		int clusterIndexForPath2 = m_pathToClusterTable[path2];

		// it is the same cluster. (?)
		// check if  clusterIndexForPath1 and clusterIndexForPath2 are the
		// same

#ifdef CONFIG_ASSERT
		assert(clusterIndexForPath1 == clusterIndexForPath2);
#endif // CONFIG_ASSERT

		int clusterIndex = clusterIndexForPath1;

		set<int> & clusterContent = m_gossipClusters[clusterIndex];

		clusterContent.insert(newGossipIndex);

		// no update are necessary for the m_pathToClusterTable index

	// step 3.5: path1 has 1 match and path2 has 1 match
	// this bridges two existing clusters, how exciting !!!
	} else if(m_pathToClusterTable.count(path1) > 0
			&& m_pathToClusterTable.count(path2) > 0) {

		int clusterIndexForPath1 = m_pathToClusterTable[path1];
		int clusterIndexForPath2 = m_pathToClusterTable[path2];

#ifdef CONFIG_ASSERT
		assert(clusterIndexForPath1 != clusterIndexForPath2);
#endif // CONFIG_ASSERT

		// TODO optimization: flip group1 and group2 if group2 is smaller than
		// group1

		mergedTwoClusters = true;

		// add everything in the cluster of path1



		set<int> & clusterContentForPath1 = m_gossipClusters[clusterIndexForPath1];
		set<int> & clusterContentForPath2 = m_gossipClusters[clusterIndexForPath2];

		for(set<int>::iterator i = clusterContentForPath2.begin() ;
				i != clusterContentForPath2.end() ; ++i) {

			int otherGossipIndex = *i;

			GraphSearchResult & otherGossip = m_gossips[otherGossipIndex];

#ifdef CONFIG_ASSERT
			if(clusterContentForPath1.count(otherGossipIndex) > 0) {
				cout << "Error: otherGossipIndex is in clusterContentForPath1 and clusterContentForPath2 ! ";
				cout << " gossip: ";
				cout << otherGossipIndex << " ";
				otherGossip.print();
				cout << " path1 " << path1;
				cout << " path2 " << path2;
				cout << " clusterIndexForPath1 " << clusterIndexForPath1;
				cout << " clusterIndexForPath2 " << clusterIndexForPath2;
				cout << " clusterContentForPath1 ";

				for(set<int>::iterator j = clusterContentForPath1.begin();
						j != clusterContentForPath1.end() ; ++j) {
					cout << " " << *j;
				}
				cout << " clusterContentForPath2 ";

				for(set<int>::iterator j = clusterContentForPath2.begin();
						j != clusterContentForPath2.end() ; ++j) {
					cout << " " << *j;
				}
				cout << endl;
			}
			assert(clusterContentForPath1.count(otherGossipIndex) == 0);
#endif

			PathHandle & otherPath1 = otherGossip.getPathHandles()[0];
			PathHandle & otherPath2 = otherGossip.getPathHandles()[1];

			// some assertions before changing things.
#ifdef CONFIG_ASSERT
			assert(m_pathToClusterTable.count(otherPath1) > 0);
			assert(m_pathToClusterTable.count(otherPath2) > 0);

			// These 2 assertions are overkill and invalid
			// because otherPath1 (or otherPath2) can appear in more than
			// 1 gossip !!! LOL XD XD
			//assert(m_pathToClusterTable[otherPath1] == clusterIndexForPath1);
			//assert(m_pathToClusterTable[otherPath2] == clusterIndexForPath2);
#endif // CONFIG_ASSERT

			clusterContentForPath1.insert(otherGossipIndex);

			// update the index buckets
			m_pathToClusterTable[otherPath1] = clusterIndexForPath1;
			m_pathToClusterTable[otherPath2] = clusterIndexForPath1;

			// some assertions *after* changing things.
#ifdef CONFIG_ASSERT
			assert(m_pathToClusterTable.count(otherPath1) > 0);
			assert(m_pathToClusterTable.count(otherPath2) > 0);
			assert(m_pathToClusterTable[otherPath1] == clusterIndexForPath1);
			assert(m_pathToClusterTable[otherPath2] == clusterIndexForPath1);
#endif // CONFIG_ASSERT


		}

		// at this point, the cluster for path1 contains everything that was in the old cluster
		// for path1 and everything that was in the old cluster for path2

		clusterContentForPath2.clear();

		clusterContentForPath1.insert(newGossipIndex);

		// we don't need to update m_pathToClusterTable because path1 and path2 are already
		// indexed.
	}


	//------------------------------------------------------------//
	// step 2. mark the gossip for future transportation

	// get the cluster that contains gossip

#ifdef CONFIG_ASSERT
	assert(m_pathToClusterTable.count(path1) > 0);
	assert(m_pathToClusterTable.count(path2) > 0);
#endif

	int finalCluster1 = m_pathToClusterTable[path1];
	int finalCluster2 = m_pathToClusterTable[path2];

#ifdef CONFIG_ASSERT
	assert(finalCluster1 == finalCluster2);
#endif

	set<int> & finalClusterContent = m_gossipClusters[finalCluster1];

#ifdef CONFIG_ASSERT
	assert(finalClusterContent.count(newGossipIndex) > 0);
#endif

	// gather all destination for this cluster

	set<Rank> destinations;

	for(set<int>::iterator i = finalClusterContent.begin() ;
			i != finalClusterContent.end() ; ++i) {

		int theGossipIndex = *i;
		GraphSearchResult & entry = m_gossips[theGossipIndex];

		Rank rank1 = getRankFromPathUniqueId(entry.getPathHandles()[0]);
		Rank rank2 = getRankFromPathUniqueId(entry.getPathHandles()[1]);

		destinations.insert(rank1);
		destinations.insert(rank2);
	}

	// here, we have a list of destinations to which we must send the gossip
	// if mergedTwoClusters is false, we only need to send gossip to the destinations
	//
	// if mergedTwoClusters is true, however, we need to send every gossip in the cluster to
	// every destination because a merge event occured.

	set<int> dummySet;
	dummySet.insert(newGossipIndex);

	set<int> * gossipsToScheduleForTransportation = & dummySet;

	if(mergedTwoClusters)
		gossipsToScheduleForTransportation = & finalClusterContent;

	int scheduledOperations = 0;

	for(set<int>::iterator gossipIndexIterator = gossipsToScheduleForTransportation->begin();
			gossipIndexIterator != gossipsToScheduleForTransportation->end();
			++gossipIndexIterator) {

		int gossipIndexToSchedule = *gossipIndexIterator;

		for(set<Rank>::iterator destinationIterator = destinations.begin() ;
				destinationIterator != destinations.end();
				++destinationIterator) {

			Rank destination = * destinationIterator;

			if(scheduleTransportation(gossipIndexToSchedule, destination))
				scheduledOperations ++;
		}

	}

#ifdef CONFIG_DEBUG_GOSSIP_ASSET_MANAGER
	cout << "DEBUG GossipAssetManager::classifyGossip classification of gossip ";
	cout << newGossipIndex << " triggered " << scheduledOperations << " scheduling";
	cout << " operations" << endl;
#endif
}

bool GossipAssetManager::scheduleGossipTransportation(GraphSearchResult & gossip, Rank & destination) {

	string key = gossip.toString();

#ifdef CONFIG_ASSERT
	assert(m_gossipIndex.count(key) > 0);
#endif

	int index = m_gossipIndex[key];

	return scheduleTransportation(index, destination);
}

bool GossipAssetManager::scheduleTransportation(int gossipIndex, Rank destination) {

	// the gossip is already on this destination
	if(m_remoteGossipOwners.count(gossipIndex) > 0 && m_remoteGossipOwners[gossipIndex].count(destination) > 0)
		return false;

	m_futureRemoteGossipOwners[gossipIndex].insert(destination);

#ifdef CONFIG_DEBUG_GOSSIP_ASSET_MANAGER
	cout << "DEBUG /GossipAssetManager::scheduleTransportation scheduled gossip " << gossipIndex;
	cout << " for immediate delivery to endpoint " << destination << endl;
#endif

	return true;
}

bool GossipAssetManager::hasGossipToShare() const {

	return m_futureRemoteGossipOwners.size() > 0;
}

void GossipAssetManager::getGossipToShare(GraphSearchResult & gossip, Rank & destination) {

#ifdef CONFIG_ASSERT
	assert(hasGossipToShare());
#endif

	// return the first gossip to share
	// and its first corresponding destination

	map<int, set<Rank> >::iterator gossipIterator = m_futureRemoteGossipOwners.begin();

	set<Rank> & destinations = gossipIterator->second;

	const int & gossipIndex = gossipIterator->first;

#ifdef CONFIG_ASSERT
	assert(destinations.size() > 0);
#endif

	set<Rank>::iterator destinationIterator = destinations.begin();

	const Rank & resultingDestination = *destinationIterator;

	GraphSearchResult & resultingGossip = m_gossips[gossipIndex];

	// give the objects to the caller

	gossip = resultingGossip;
	destination = resultingDestination;
}

vector<GraphSearchResult> & GossipAssetManager::getGossips() {
	return m_gossips;
}

bool GossipAssetManager::hasGossip(const GraphSearchResult & gossip) const {

	string key = gossip.toString();

	return m_gossipIndex.count(key) > 0;
}

void GossipAssetManager::registerRemoteGossip(GraphSearchResult & gossip, Rank & destination) {

	string key = gossip.toString();

#ifdef CONFIG_ASSERT
	assert(m_gossipIndex.count(key) > 0);
#endif

	int index = m_gossipIndex[key];

#if 0
	// check that the asset is registered to be sent to this remote
	// Actually, we may want to register remote copies even when there is no scheduled
	// operations.
	assert(m_futureRemoteGossipOwners.count(index) > 0 && m_futureRemoteGossipOwners[index].count(destination) > 0);
#endif

	// remove any scheduled operation for this gossip and this destination
	if(m_futureRemoteGossipOwners.count(index) > 0
			&& m_futureRemoteGossipOwners[index].count(destination) > 0) {

		// at this point, everything looks OK.
		m_futureRemoteGossipOwners[index].erase(destination);

		if(m_futureRemoteGossipOwners[index].size() == 0) {
			m_futureRemoteGossipOwners.erase(index);
		}
	}


	/// this assertion is not valid
#if 0
#ifdef CONFIG_ASSERT
	// the asset must not be already registered...
	assert(m_remoteGossipOwners.count(index) == 0 || m_remoteGossipOwners[index].count(destination) == 0);
#endif
#endif


	m_remoteGossipOwners[index].insert(destination);

#if CONFIG_DEBUG_GOSSIP_ASSET_MANAGER
	cout << "DEBUG GossipAssetManager::registerRemoteGossip" << index << " was successfully";
	cout << " delivered to endpoint " << destination << endl;
#endif
}
