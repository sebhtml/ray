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
#include <routing/ConnectionGraph.h>
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

/**
 * Number of relayed messages if the relay checker is activated.
 */
	map<Tag,int> m_relayedMessagesFrom0;

	map<Tag,int> m_relayedMessagesTo0;


/**  the connection graph */
	ConnectionGraph m_graph;

/**
 * Is the relay checker activated
 */
	bool m_relayCheckerActivated;

/**
 * A list of tags to check with the relay checker.
 * from 0
 */
	set<Tag> m_tagsToCheckForRelayFrom0;
	
/**
 * Tags to check to 0
 */
	set<Tag> m_tagsToCheckForRelayTo0;

/**
 * Is the router activated at all ?
 */
	bool m_enabled;

/**
 * The message inbox
 */
	StaticVector*m_inbox;

/**
 * The message outbox
 */
	StaticVector*m_outbox;

/**
 * The outbox buffer allocator
 */
	RingAllocator*m_outboxAllocator;

/**
 * The identifier of the current rank
 */
	Rank m_rank;

/**
 * The number of ranks
 */
	int m_size;



/**************** METHODS ***************************/

	void relayMessage(Message*message,Rank destination);

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

/**
 * Callback method to call before sending messages in the outbox onto the network
 */
	void routeOutcomingMessages();

/**
 * Callback method to call after receiving messages from the network into the inbox
 */
	void routeIncomingMessages();

	bool isEnabled();

/**
 * Enable the router
 * This could be a constructor.
 */
	void enable(StaticVector*inbox,StaticVector*outbox,RingAllocator*outboxAllocator,Rank rank,
string prefix,int numberOfRanks,string type);

	bool isRoutingTag(Tag tag);


/**
 * Check if relayed messages have completed their transit.
 * This is required for the messages sent at the end
 * of the computation because we need ranks to shut down 
 * after they have relayed messages for which they act as
 * relay in the routing table.
 */
	bool hasCompletedRelayEvents();

/**
 * We actually only need to monitor a few message tags
 * for relay events. They are added with this method
 * if they are sent from 0
 */
	void addTagToCheckForRelayFrom0(Tag tag);

/**
 * Tags to monitor when sent to 0
 */
	void addTagToCheckForRelayTo0(Tag tag);

/**
 * Unless the relayChecker component is activated with
 * this method, no logic code concerning relays is ever
 * executed whatsoever.
 */
	void activateRelayChecker();

	ConnectionGraph*getGraph();
};

#endif
