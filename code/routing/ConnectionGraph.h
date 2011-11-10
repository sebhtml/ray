/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _ConnectionGraph
#define _ConnectionGraph

#include <vector>
#include <set>
#include <map>
#include <core/statistics.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h> /* for log */
#include <algorithm> /* random_shuffle */

#include <string>
#include <core/types.h>
using namespace std;

/** a graph for connections between compute cores */
class ConnectionGraph{

	/** the type of the graph */
	string m_type;

/**
 * The number of ranks
 */
	int m_size;

/**
 * Number of cores for the group sub-command.
 * Only useful with -route-messages -connection-type group -cores-per-node X
 */
	int m_coresPerNode;

/** 
 * routes contained in the route tables
 *
 * data:
 *
 * source
 * 	destination
 * 		vertex1
 * 			vertex2
 */
	vector<vector<map<Rank,Rank> > > m_routes;

/**
 * connections
 */
	vector<set<Rank> > m_connections;

/**
 * number of relays
 */
	vector<Tag> m_relayEvents;

/**
 * Number of relay events between any destination and source 0
 */
	vector<Tag> m_relayEventsTo0;

/**
 * Number of relay events between source 0 and any destination
 */
	vector<Tag> m_relayEventsFrom0;

/** remove useless connections */
	void removeUnusedConnections();

/**
 * Get a route between a source and a destination
 */
	void getRoute(Rank source,Rank destination,vector<Rank>*route);

	/************************************************/
	/** methods to build connections */

	/** general method to make connections */
	void makeConnections(string type);

	/** random connections */
	void makeConnections_random();

	/** grouped connections */
	void makeConnections_group();
	int getIntermediateRank(Rank rank);

	/** complete connections */
	void makeConnections_complete();

	/** find shortest paths between all pairs */
	void makeRoutes();

	/** find the shortest path between a source and a destination */
	void findShortestPath(Rank source,Rank destination,vector<Rank>*route);

	/** print a route */
	void printRoute(Rank source,Rank destination);

public:

/**
 * Get the next rank to relay the message after <rank> for the route between
 * source and destination
 */
	Rank getNextRankInRoute(Rank source,Rank destination,Rank rank);

/**
 * Is there a up-link between a source and a destination
 */
	bool isConnected(Rank source,Rank destination);

	void writeFiles(string prefix);

/** build the graph. */
	void buildGraph(int numberOfVertices,string method,int groupSize);


/**
 * Get the connections for a source
 */
	void getConnections(Rank source,vector<Rank>*connections);


	/** get the number of paths that contain rank from 0 to any vertex */
	int getRelaysFrom0(Rank rank);

	/** get the number of paths that contain rank from any vertex to vertex 0  */
	int getRelaysTo0(Rank rank);

};

#endif
