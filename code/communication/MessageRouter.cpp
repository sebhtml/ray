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

// #define CONFIG_ROUTER_VERBOSITY

/**
 * \brief Message router implementation
 *
 * \author Sébastien Boisvert
 * \reviewedBy __
*/

#include <communication/MessageRouter.h>
#include <string.h> /* for memcpy */
#include <assert.h>
#include <core/constants.h>
#include <core/common_functions.h>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#define RAY_ROUTING_TAG_TAG_OFFSET 0
#define RAY_ROUTING_TAG_TAG_SIZE 8
#define RAY_ROUTING_TAG_SOURCE_OFFSET RAY_ROUTING_TAG_TAG_SIZE
#define RAY_ROUTING_TAG_SOURCE_SIZE 11
#define RAY_ROUTING_TAG_DESTINATION_OFFSET (RAY_ROUTING_TAG_TAG_SIZE+RAY_ROUTING_TAG_SOURCE_SIZE)
#define RAY_ROUTING_TAG_DESTINATION_SIZE 11
#define RAY_ROUTING_TAG_BIT_OFFSET (RAY_ROUTING_TAG_TAG_SIZE+RAY_ROUTING_TAG_SOURCE_SIZE+RAY_ROUTING_TAG_DESTINATION_SIZE)

/**
 * route outcoming messages
 */
void MessageRouter::routeOutcomingMessages(){
	int numberOfMessages=m_outbox->size();

	for(int i=0;i<numberOfMessages;i++){
		Message*aMessage=m_outbox->at(i);

		int communicationTag=aMessage->getTag();

		// - first, the message may have been already routed when it was received (also
		// in a routed version). In this case, nothing must be done.
		if(isRoutingTag(communicationTag)){
			#ifdef CONFIG_ROUTER_VERBOSITY
			cout<<__func__<<" Message has already a routing tag."<<endl;
			#endif
			continue;
		}

		// at this point, the message has no routing information yet.
		int trueSource=aMessage->getSource();
		int trueDestination=aMessage->getDestination();

		// if it is reachable, no further routing is required
		if(isConnected(trueSource,trueDestination)){
			#ifdef CONFIG_ROUTER_VERBOSITY
			cout<<__func__<<" Rank "<<trueSource<<" can reach "<<trueDestination<<" without routing"<<endl;
			#endif
			continue;
		}
	
		// re-route the message by re-writing the tag
		int routingTag=getRoutingTag(communicationTag,trueSource,trueDestination);
		aMessage->setTag(routingTag);

		int nextRank=getNextRankInRoute(trueSource,trueDestination,m_rank);
		aMessage->setDestination(nextRank);

		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" rerouted message (trueSource="<<trueSource<<" trueDestination="<<trueDestination<<" to intermediateSource "<<intermediateSource<<endl;
		#endif
	}

	// check that all messages are routable
	#ifdef ASSERT
	for(int i=0;i<numberOfMessages;i++){
		Message*aMessage=m_outbox->at(i);
		if(!isConnected(aMessage->getSource(),aMessage->getDestination()))
			cout<<aMessage->getSource()<<" and "<<aMessage->getDestination()<<" are not connected !"<<endl;
		assert(isConnected(aMessage->getSource(),aMessage->getDestination()));
	}
	#endif
}

/*
 * To do so, the tag attribute of a message is converted to 
 * a composite tag which contains:
 *
 * int tag
 *
 * bits 0 to 7: tag (8 bits, values from 0 to 255, 256 possible values)
 * bits 8 to 18: true source (11 bits, values from 0 to 2047, 2048 possible values)
 * bits 19 to 29: true destination (11 bits, values from 0 to 2047, 2048 possible values)
 * bit 30: 1 = tag is a routing tag, 0 = tag is not a routing tag
 *
 * 8+11+11+1 = 31
 */
int MessageRouter::getRoutingTag(int tag,int source,int destination){
	uint64_t routingTag=0;

	uint64_t largeTag=tag;
	largeTag<<=RAY_ROUTING_TAG_TAG_OFFSET;
	routingTag|=largeTag;
	
	// set the routing tag to 1
	uint64_t routingEnabled=1;
	routingEnabled<<=RAY_ROUTING_TAG_BIT_OFFSET;
	routingTag|=routingEnabled;

	uint64_t largeSource=source;
	largeSource<<=RAY_ROUTING_TAG_SOURCE_OFFSET;
	routingTag|=largeSource;

	uint64_t largeDestination=destination;
	largeDestination<<=RAY_ROUTING_TAG_DESTINATION_OFFSET;
	routingTag|=largeDestination;

	// should be alright because we use 31 bits only.
	int result=routingTag;

	return result;
}

/**
 * a rank can only speak to things listed in connections
 */
bool MessageRouter::isConnected(int source,int destination){
	// check that a connection exists
	return m_connections[source].count(destination)>0;
}

/**
 * a tag is a routing tag is its routing bit is set to 1
 */
bool MessageRouter::isRoutingTag(int tag){
	uint64_t data=tag;
	int bitNumber=RAY_ROUTING_TAG_BIT_OFFSET;
	int bitValue=(data<<(63-bitNumber)) >> 63;
	
	return bitValue==1;
}

bool MessageRouter::isEnabled(){
	return m_enabled;
}

MessageRouter::MessageRouter(){
	m_enabled=false;
}

void MessageRouter::enable(StaticVector*inbox,StaticVector*outbox,RingAllocator*outboxAllocator,int rank,
	string prefix,int numberOfRanks,int coresPerNode){

	m_coresPerNode=coresPerNode;
	cout<<endl;

	cout<<"[MessageRouter] Enabled message routing"<<endl;

	m_inbox=inbox;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_rank=rank;
	m_enabled=true;

	// generate the routes
	generateRoutes(numberOfRanks);

	// dump the connections in a file
	ostringstream file;
	file<<prefix<<"Rank"<<rank<<".Connections.txt";
	ofstream f(file.str().c_str());
	f<<m_connections[m_rank].size()<<"	";

	for(set<int>::iterator i=m_connections[m_rank].begin();
		i!=m_connections[m_rank].end();i++){
		if(i!=m_connections[m_rank].begin())
			f<<" ";
		f<<*i;
	}

	f.close();

	// dump the routes in a file
	ostringstream file2;
	file2<<prefix<<"Rank"<<rank<<".Routes.txt";
	ofstream f2(file2.str().c_str());
	f2<<"#Source	Destination	Hops	Route"<<endl;

	for(int i=0;i<numberOfRanks;i++){
		vector<int> route;
		getRoute(rank,i,&route);
		f2<<rank<<"	"<<i<<"	"<<route.size()-1<<"	";

		for(int i=0;i<(int)route.size();i++){
			if(i!=0)
				f2<<" ";
			f2<<route[i];
		}

		f2<<endl;
	}

	f2.close();
}

/**
 * route incoming messages 
 */
void MessageRouter::routeIncomingMessages(){
	int numberOfMessages=m_inbox->size();

	// we have no message
	if(numberOfMessages==0)
		return;

	// otherwise, we have exactly one precious message.
	
	Message*aMessage=m_inbox->at(0);
	int tag=aMessage->getTag();

	// if the message has no routing tag, then we can sefely receive it as is
	if(!isRoutingTag(tag)){
		// nothing to do
		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" message has no routing tag, nothing to do"<<endl;
		#endif

		return;
	}

	int trueSource=getSource(tag);
	int trueDestination=getDestination(tag);

	// this is the final destination
	// we have received the message
	// we need to restore the original information now.
	if(trueDestination==m_rank){
		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" message has reached destination, must strip routing information"<<endl;
		#endif

		// we must update the original source and original tag
		aMessage->setSource(trueSource);
		
		int trueTag=getTag(tag);
		aMessage->setTag(trueTag);

		return;
	}

	#ifdef ASSERT
	assert(m_rank!=trueDestination);
	#endif

	// at this point, we know that we need to forward
	// the message to another peer
	int nextRank=getNextRankInRoute(trueSource,trueDestination,m_rank);

	#ifdef CONFIG_ROUTER_VERBOSITY
	cout<<__func__<<" message has been sent to the next one, trueSource="<<trueSource<<" trueDestination= "<<trueDestination<<endl;
	#endif
		
	// we forward the message
	forwardMessage(aMessage,nextRank);
}

/**
 * forward a message to follow a route
 */
void MessageRouter::forwardMessage(Message*message,int destination){
	int count=message->getCount();

	// allocate a buffer from the ring
	if(count>0){
		uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		// copy the data into the new buffer
		memcpy(outgoingMessage,message->getBuffer(),count*sizeof(uint64_t));
		message->setBuffer(outgoingMessage);
	}

	// re-route the message
	message->setSource(m_rank);
	message->setDestination(destination);

	#ifdef ASSERT
	assert(isConnected(m_rank,destination));
	#endif

	m_outbox->push_back(*message);
}

/**
 * * bits 0 to 7: tag (8 bits, values from 0 to 255, 256 possible values)
 */
int MessageRouter::getTag(int tag){
	uint64_t data=tag;
	data<<=(sizeof(uint64_t)*8-(RAY_ROUTING_TAG_TAG_OFFSET+RAY_ROUTING_TAG_TAG_SIZE));
	data>>=(sizeof(uint64_t)*8-RAY_ROUTING_TAG_TAG_SIZE);
	return data;
}

/**
 * * bits 8 to 18: true source (11 bits, values from 0 to 2047, 2048 possible values)
 */
int MessageRouter::getSource(int tag){
	uint64_t data=tag;
	data<<=(sizeof(uint64_t)*8-(RAY_ROUTING_TAG_SOURCE_OFFSET+RAY_ROUTING_TAG_SOURCE_SIZE));
	data>>=(sizeof(uint64_t)*8-RAY_ROUTING_TAG_SOURCE_SIZE);
	return data;
}

/**
 * * bits 19 to 29: true destination (11 bits, values from 0 to 2047, 2048 possible values)
 */
int MessageRouter::getDestination(int tag){
	uint64_t data=tag;
	data<<=(sizeof(uint64_t)*8-(RAY_ROUTING_TAG_DESTINATION_OFFSET+RAY_ROUTING_TAG_DESTINATION_SIZE));
	data>>=(sizeof(uint64_t)*8-RAY_ROUTING_TAG_DESTINATION_SIZE);
	return data;
}

/**
 * get the route between two points
 */
void MessageRouter::getRoute(int source,int destination,vector<int>*route){
	int currentVertex=source;
	route->push_back(currentVertex);

	while(currentVertex!=destination){
		currentVertex=getNextRankInRoute(source,destination,currentVertex);
		route->push_back(currentVertex);
	}
}

/**
 * generate routes and connections
 */
void MessageRouter::generateRoutes(int n){
	generateRoutesByGroups(n);
}

int MessageRouter::getIntermediateRank(int rank){
	return rank-rank % m_coresPerNode;
}

/**
 * given n ranks, they are grouped in groups.
 * in each group, only one rank is allowed to communicate with the reprentative rank of
 * other groups.
 *
 * a rank can communicate with itself and with its intermediate rank
 *
 * if a rank is intermediate, it can reach any intermediate rank too.
 *
 * This maps well on super-computers with the same number of cores on each node
 *
 * For instance, if a node has 8 cores, then 8 ranks per group is correct.
 *
 * this method populates these attributes:
 *
 * 	- m_connections
 * 	- m_routes
 */
void MessageRouter::generateRoutesByGroups(int n){
// the general idea of routing a message:
//
//
// Cases: (starting with simpler cases)
//
//
// case 1: source and destination are the same (1 hop, no routing required)
// case 2:  source and destination are allowed to communicate (1 hop, no routing required)
//   happens when 
//       - source is the intermediate rank of the destination
//       or
//       - destination is the intermediate rank of the source
// case 3:  source and destination share the same intermediate rank (2 hops, some routing)
// case 4:  source and destination don't share the same intermediate rank (3 hops, full routing)
//
//
// see Documentation/Message-Routing.txt
//
//             1	                 2                              3
// trueSource -> sourceIntermediateRank -> destinationIntermediateRank -> trueDestination

// the message has no routing tag
// we must check that the channel is authorized.

	for(int source=0;source<n;source++){
		int intermediateSource=getIntermediateRank(source);
	
		// can connect with self.
		m_connections[source].insert(source);

		// can connect with the intermediate source
		m_connections[source].insert(intermediateSource);

		for(int destination=0;destination<n;destination++){
			int intermediateDestination=getIntermediateRank(destination);

			// an intermediate node can connect with any intermediate node
			if(destination==intermediateDestination && source==intermediateSource)
				m_connections[source].insert(intermediateDestination);
			
			// if the source is the intermediate destination, add a link
			// this is within the same group
			if(source==intermediateDestination)
				m_connections[source].insert(destination);

			// peers in the same group are allowed to connect
			if(intermediateSource==intermediateDestination)
				m_connections[source].insert(destination);

			// same rank
			// source -> destination
			if(source==destination){
				m_routes[source][destination][source]=destination;

			// direct communication is allowed
			// source -> destination
			}else if(source==intermediateSource && destination==intermediateDestination){
				m_routes[source][destination][source]=destination;

			// destination is the intermediate source
			// source -> destination
			}else if(intermediateSource==destination){
				m_routes[source][destination][source]=destination;
			
			// source is the intermediate destination
			// source -> destination
			}else if(source==intermediateDestination){
				m_routes[source][destination][source]=destination;
			// source and destination have the same intermediate
			// source -> destination
			// it is faster like this
			}else if(intermediateSource==intermediateDestination){
				m_routes[source][destination][source]=destination;
			
			// source and destination have a different intermediate
			// source -> intermediateSource -> intermediateDestination -> destination
			}else{
				m_routes[source][destination][source]=intermediateSource;
				m_routes[source][destination][intermediateSource]=intermediateDestination;
				m_routes[source][destination][intermediateDestination]=destination;
			}
		}
	}
}

int MessageRouter::getNextRankInRoute(int source,int destination,int rank){
	#ifdef ASSERT
	assert(m_routes[source][destination].count(rank)==1);
	#endif

	return m_routes[source][destination][rank];
}

void MessageRouter::getConnections(int source,vector<int>*connections){
	for(set<int>::iterator i=m_connections[m_rank].begin();
		i!=m_connections[m_rank].end();i++){
		connections->push_back(*i);
	}
}
