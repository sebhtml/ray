/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _GraphImplementation_h
#define _GraphImplementation_h

#include <stdint.h>

/**
 * a tuple for message routing
 * */
class Tuple{
public:
	uint8_t m_digits[16];
};

#include <vector>
#include <set>
#include <map>
#include <core/types.h>
using namespace std;

/** virtual interface for graphs */
class GraphImplementation{

protected:
	bool m_verbose;

	int m_size;

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
 * number of relays
 */
	vector<int> m_relayEvents;

/**
 * Number of relay events between any destination and source 0
 */
	vector<int> m_relayEventsTo0;

/**
 * Number of relay events between source 0 and any destination
 */
	vector<int> m_relayEventsFrom0;


/**
 * outcoming connections
 */
	vector<set<Rank> > m_outcomingConnections;

/**
 * incoming connections
 */
	vector<set<Rank> > m_incomingConnections;

	void computeRoutes();

	/** find the shortest path between a source and a destination */
	void findShortestPath(Rank source,Rank destination,vector<Rank>*route);

	void computeRelayEvents();

	virtual void computeRoute(Rank a,Rank b,vector<Rank>*route) = 0;
	
public:

	virtual void makeConnections(int n) =0;
	
	virtual void makeRoutes() = 0;

/** get the next rank in a route */
	virtual Rank getNextRankInRoute(Rank source,Rank destination,Rank rank) = 0;
	
	virtual bool isConnected(Rank i,Rank j) = 0;

	virtual ~GraphImplementation(){ /* nothing */} /* and no trailing ; */

	void getRoute(Rank source,Rank destination,vector<Rank>*route);

/**
 * Get the connections for a source
 */
	void getOutcomingConnections(Rank source,vector<Rank>*connections);

	void getIncomingConnections(Rank rank,vector<Rank>*connections);

	void setVerbosity(bool verbosity);

	int getRelays(Rank rank);
	int getRelaysTo0(Rank rank);
	int getRelaysFrom0(Rank rank);
};

#endif

