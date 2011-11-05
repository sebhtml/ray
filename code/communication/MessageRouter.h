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

#ifndef _MessageRouter_h
#define _MessageRouter_h

#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <communication/Message.h>
#include <map>
#include <core/constants.h>
#include <vector>
#include <string>
using namespace std;

/**
 * \author Sébastien Boisvert 2011-11-04
 * \reviewedBy Elénie Godzaridis 2011-11-05
 *
 * the MessageRouter makes communication more efficient.
 *
 * To do so, the tag attribute of a message is converted to 
 * a composite tag which contains:
 *
 * int tag
 *
 * bits 0 to 7: tag (8 bits, values from 0 to 255, 256 possible values)
 * bits 8 to 19: true source (12 bits, values from 0 to 4095, 4096 possible values)
 * bits 20 to 31: true destination (12 bits, values from 0 to 4095, 4096 possible values)
 */
class MessageRouter{
	int m_coresPerNode;

	bool m_enabled;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	Rank m_rank;
	int m_size;

/** routes
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

	
	void forwardMessage(Message*message,Rank destination);

	void getRoute(Rank source,Rank destination,vector<Rank>*route);

	Rank getNextRankInRoute(Rank source,Rank destination,Rank rank);

	bool isConnected(Rank destination,Rank source);

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

	void findShortestPath(Rank source,Rank destination,vector<Rank>*route);
	void printRoute(Rank source,Rank destination);

	void writeFiles(string prefix);

	/********************************************/
	/* stuff for routing tags */

	/** build a routing tag */
	RoutingTag getRoutingTag(Tag tag,Rank source,Rank destination);

	/** get the source from a routing tag */
	Rank getSource(RoutingTag tag);

	/** get the destination from a routing tag */
	Rank getDestination(RoutingTag tag);

	/** get the tag from a routing tag */
	Tag getTag(RoutingTag tag);

public:
	MessageRouter();

	void routeOutcomingMessages();
	void routeIncomingMessages();

	bool isEnabled();
	void enable(StaticVector*inbox,StaticVector*outbox,RingAllocator*outboxAllocator,Rank rank,
string prefix,int numberOfRanks,int coresPerNode,string type);

	bool isRoutingTag(Tag tag);

	void getConnections(Rank source,vector<Rank>*connections);
};

#endif
