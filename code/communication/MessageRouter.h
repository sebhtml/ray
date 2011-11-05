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
#include <vector>
#include <map>
#include <string>
using namespace std;

/**
 * \author Sébastien Boisvert
 * \reviewedBy __
 *
 * the MessageRouter makes communication more efficient.
 *
 * To do so, the tag attribute of a message is converted to 
 * a composite tag which contains:
 *
 * int tag
 *
 * bits 0 to 7: tag (8 bits, values from 0 to 255, 256 possible values)
 * bits 8 to 18: true source (11 bits, values from 0 to 2047, 2048 possible values)
 * bits 19 to 29: true destination (11 bits, values from 0 to 2047, 2048 possible values)
 * bit 30: 1 = tag is a routing tag, 0 = tag is not a routing tag
 */
class MessageRouter{
	int m_coresPerNode;

	bool m_enabled;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	int m_rank;
	int m_size;

	set<int> m_directTags;

/** routes
 * source
 * 	destination
 * 		vertex1
 * 			vertex2
 */
	map<int,map<int,map<int,int> > > m_routes;

/**
 * connections
 */
	map<int,set<int> > m_connections;

	int getIntermediateRank(int rank);
	int getRoutingTag(int tag,int source,int destination);

	void forwardMessage(Message*message,int destination);

	/** get the source from a routing tag */
	int getSource(int tag);

	/** get the destination from a routing tag */
	int getDestination(int tag);

	/** get the tag from a routing tag */
	int getTag(int tag);

	void getRoute(int source,int destination,vector<int>*route);

	int getNextRankInRoute(int source,int destination,int rank);

	bool isConnected(int destination,int source);

	void makeConnections(string type);
	void makeConnections_randomGraph();
	void makeConnections_withGroups();
	void makeConnections_complete();

	void viewConnections();
	void findShortestPath(int source,int destination,vector<int>*route);
	void printRoute(int source,int destination);
	void makeRoutes();
	void viewRoutes();

	void writeFiles(string prefix);
public:
	MessageRouter();

	void routeOutcomingMessages();
	void routeIncomingMessages();

	bool isEnabled();
	void enable(StaticVector*inbox,StaticVector*outbox,RingAllocator*outboxAllocator,int rank,
string prefix,int numberOfRanks,int coresPerNode,string type);

	bool isRoutingTag(int tag);

	void getConnections(int source,vector<int>*connections);

	void addDirectTag(int tag);
};

#endif
