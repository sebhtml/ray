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

/**
 * route outcoming messages
 */
void MessageRouter::routeOutcomingMessages(){
	int numberOfMessages=m_outbox->size();

	if(numberOfMessages==0)
		return;

	for(int i=0;i<numberOfMessages;i++){

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

		// for the first 2 cases, nothing is required...
		//
		// - source and destination are the same (1 hop, no routing required)
		// - source and destination are allowed to communicate (1 hop, no routing required)
		//   happens when 
		//       - source is the intermediate rank of the destination
		//       or
		//       - destination is the intermediate rank of the source

		int intermediateSource=getIntermediateRank(trueSource);

		if(isReachable(trueSource,trueDestination)){
			#ifdef CONFIG_ROUTER_VERBOSITY
			cout<<__func__<<" Rank "<<trueSource<<" can reach "<<trueDestination<<" without routing"<<endl;
			#endif
			continue;
		}

		// for the 2 other cases, routing is required.
		//
		// The 2 cases are:
		//
		// - source and destination share the same intermediate rank (2 hops, some routing)
		// - source and destination don't share the same intermediate rank (3 hops, full routing)
		//
		// Regardless of which case we have, we have to send to intermediateSource anyway.

		int routingTag=getRoutingTag(communicationTag,trueSource,trueDestination);

		// re-route the message
		aMessage->setTag(routingTag);
		aMessage->setDestination(intermediateSource);

		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" rerouted message (trueSource="<<trueSource<<" trueDestination="<<trueDestination<<" to intermediateSource "<<intermediateSource<<endl;
		#endif
	}

	// check that all messages are routable
	#ifdef ASSERT

	for(int i=0;i<numberOfMessages;i++){
		Message*aMessage=m_outbox->at(i);
		assert(isReachable(aMessage->getSource(),aMessage->getDestination()));
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
	uint64_t routingTag=tag;
	
	// set the routing tag to 1
	uint64_t routingEnabled=(1<<30);
	routingTag|=routingEnabled;

	uint64_t largeSource=source;
	largeSource<<=8;
	routingTag|=largeSource;

	uint64_t largeDestination=destination;
	largeDestination<<=(8+11);
	routingTag|=largeDestination;

	// should be alright because we use 31 bits only.
	int result=routingTag;

	return result;
}

/**
 * a rank can only speak to itself and to its intermediate rank 
 */
bool MessageRouter::isReachable(int source,int destination){
	/* a rank can communicate with itself, obviously */
	if(source==destination)
		return true;

	/* compute intermediate peers */
	int intermediateSource=getIntermediateRank(source);
	int intermediateDestination=getIntermediateRank(destination);

	/* if the intermediate rank for the source is the destination */
	if(intermediateSource==destination)
		return true;

	/* if the source is the intermediate rank for the destination */
	if(source==intermediateDestination)
		return true;

	/* an intermediate node can communicate with any other intermediate node */
	if(intermediateDestination==destination && intermediateSource==source)
		return true;
	
	/* otherwise it is not allowed*/
	return false;
}

int MessageRouter::getIntermediateRank(int rank){
	int groupSize=8;

	return rank-rank % groupSize;
}

bool MessageRouter::isRoutingTag(int tag){
	uint64_t data=tag;

	int bitNumber=30;

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
	string prefix,int numberOfRanks){
	cout<<endl;

	cout<<"Enabled message routing"<<endl;

	m_inbox=inbox;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_rank=rank;
	m_enabled=true;

	generateRoutes(numberOfRanks);

	ostringstream file;
	file<<prefix<<"Rank"<<rank<<".Connections.txt";

	ofstream f(file.str().c_str());
	
	vector<int> peers;
	for(int i=0;i<numberOfRanks;i++){
		if(isReachable(rank,i))
			peers.push_back(i);
	}

	f<<peers.size()<<"	";

	for(int i=0;i<(int)peers.size();i++){
		if(i!=0)
			f<<" ";
		f<<peers[i];
	}

	f.close();

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
 * route incoming messages */
void MessageRouter::routeIncomingMessages(){
	int numberOfMessages=m_inbox->size();

	// we have no message
	if(numberOfMessages==0)
		return;

	// otherwise, we have exactly one precious message.
	
	// see Documentation/Message-Routing.txt
	//
	//             1	                 2                              3
	// trueSource -> sourceIntermediateRank -> destinationIntermediateRank -> trueDestination
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


	// there are these cases when we receive a message
	//
	// case 1: the tag is not a routing tag
	// 	then we have nothing to do.
	// case 2: the tag is a routing tag, but the trueSource is m_rank 
	// 	in this case, we have to stript the tag to remove routing data
	// case 3: the tag is a routing tag and 1 more hop is required
	// case 4: the tag is a routing tag and 2 more hop is required
	
	Message*aMessage=m_inbox->at(0);

	int tag=aMessage->getTag();

	if(!isRoutingTag(tag)){
		// nothing to do
		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" message has no routing tag, nothing to do"<<endl;
		#endif

		return;
	}

	int trueSource=getSource(tag);
	int trueDestination=getDestination(tag);

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

	//             1	                 2                              3
	// trueSource -> sourceIntermediateRank -> destinationIntermediateRank -> trueDestination

	int sourceIntermediateRank=getIntermediateRank(trueSource);
	int destinationIntermediateRank=getIntermediateRank(trueDestination);
	
	// this condition will process the following cases:
	//
	// ** trueSource -> destinationIntermediateRank -> trueDestination
	//	
	//	(sourceIntermediateRank and destinationIntermediateRank are the same)
	//
	// ** trueSource -> sourceIntermediateRank -> destinationIntermediateRank -> trueDestination
	//
	if(m_rank==destinationIntermediateRank){
		// here, we need to send a message onto the network.

		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" message has 1 more hop (last destination) to do, trueSource="<<trueSource<<" trueDestination= "<<trueDestination<<endl;
		#endif
		
		rerouteMessage(aMessage,trueDestination);

	}else if(m_rank== sourceIntermediateRank){

		// here, we need to send a message onto the network.
		
		#ifdef ASSERT
		assert(m_rank!=destinationIntermediateRank);
		#endif

		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" message has 2 more hops to do, trueSource="<<trueSource<<" trueDestination= "<<trueDestination<<" sourceIntermediateRank="<<sourceIntermediateRank<<" (*)destinationIntermediateRank="<<destinationIntermediateRank<<endl;
		#endif

		rerouteMessage(aMessage,destinationIntermediateRank);
	}
}

void MessageRouter::rerouteMessage(Message*message,int destination){
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
	assert(isReachable(m_rank,destination));
	#endif

	m_outbox->push_back(*message);
}

/**
 * * bits 0 to 7: tag (8 bits, values from 0 to 255, 256 possible values)
 */
int MessageRouter::getTag(int tag){
	uint64_t data=tag;
	data<<=(64-8);
	data>>=(64-8);
	return data;
}

/**
 * * bits 8 to 18: true source (11 bits, values from 0 to 2047, 2048 possible values)
 */
int MessageRouter::getSource(int tag){
	uint64_t data=tag;
	data<<=(64-(8+11));
	data>>=(64-11);
	return data;

}

/**
 * * bits 19 to 29: true destination (11 bits, values from 0 to 2047, 2048 possible values)
 */
int MessageRouter::getDestination(int tag){
	uint64_t data=tag;
	data<<=(64-(8+11+11));
	data>>=(64-11);
	return data;

}

void MessageRouter::getRoute(int source,int destination,vector<int>*route){
	int currentVertex=source;
	route->push_back(currentVertex);

	while(currentVertex!=destination){
		currentVertex=m_routes[source][destination][currentVertex];
		route->push_back(currentVertex);
	}
}

void MessageRouter::generateRoutes(int n){
	generateRoutesByGroups(n);
}

void MessageRouter::generateRoutesByGroups(int n){
	for(int source=0;source<n;source++){
		int intermediateSource=getIntermediateRank(source);
		for(int destination=0;destination<n;destination++){
			int intermediateDestination=getIntermediateRank(destination);
			
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
			// source -> intermediateSource -> destination
			}else if(intermediateSource==intermediateDestination){
				m_routes[source][destination][source]=intermediateSource;
				m_routes[source][destination][intermediateSource]=destination;
			
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
