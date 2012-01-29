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

//#define CONFIG_ROUTING_VERBOSITY

/**
 * \brief Message router implementation
 *
 * \author Sébastien Boisvert 2011-11-04
 * \reviewedBy Elénie Godzaridis 2011-11-05
*/

#include <communication/MessageRouter.h>
#include <string.h> /* for memcpy */
#include <assert.h>
#include <core/OperatingSystem.h>
#include <core/ComputeCore.h>
using namespace std;

/*
#define CONFIG_ROUTING_VERBOSITY
#define ASSERT
*/

/**
 * route outcoming messages
 */
void MessageRouter::routeOutcomingMessages(){
	int numberOfMessages=m_outbox->size();

	for(int i=0;i<numberOfMessages;i++){
		Message*aMessage=m_outbox->at(i);

		MessageTag communicationTag=aMessage->getTag();

		#ifdef CONFIG_ROUTING_VERBOSITY
		uint8_t printableTag=communicationTag;
		cout<<"routeOutcomingMessages tag= "<<MESSAGE_TAGS[printableTag]<<endl;
		#endif

		// - first, the message may have been already routed when it was received (also
		// in a routed version). In this case, nothing must be done.
		if(isRoutingTag(communicationTag)){
			#ifdef CONFIG_ROUTING_VERBOSITY
			cout<<__func__<<" Message has already a routing tag."<<endl;
			#endif
			continue;
		}

		// at this point, the message has no routing information yet.
		Rank trueSource=aMessage->getSource();
		Rank trueDestination=aMessage->getDestination();

		// if it is reachable, no further routing is required
		if(m_graph.isConnected(trueSource,trueDestination)){
			#ifdef CONFIG_ROUTING_VERBOSITY
			cout<<__func__<<" Rank "<<trueSource<<" can reach "<<trueDestination<<" without routing"<<endl;
			#endif
			continue;
		}
	
		// re-route the message by re-writing the tag
		RoutingTag routingTag=getRoutingTag(communicationTag,trueSource,trueDestination);
		aMessage->setTag(routingTag);

		Rank nextRank=m_graph.getNextRankInRoute(trueSource,trueDestination,m_rank);
		aMessage->setDestination(nextRank);

		#ifdef CONFIG_ROUTING_VERBOSITY
		cout<<__func__<<" relayed message (trueSource="<<trueSource<<" trueDestination="<<trueDestination<<" to intermediateSource "<<nextRank<<endl;
		#endif
	}

	// check that all messages are routable
	#ifdef ASSERT
	for(int i=0;i<numberOfMessages;i++){
		Message*aMessage=m_outbox->at(i);
		if(!m_graph.isConnected(aMessage->getSource(),aMessage->getDestination()))
			cout<<aMessage->getSource()<<" and "<<aMessage->getDestination()<<" are not connected !"<<endl;
		assert(m_graph.isConnected(aMessage->getSource(),aMessage->getDestination()));
	}
	#endif
}

/**
 * route incoming messages 
 * \returns true if rerouted something.
 */
bool MessageRouter::routeIncomingMessages(){
	int numberOfMessages=m_inbox->size();

	// we have no message
	if(numberOfMessages==0)
		return false;

	// otherwise, we have exactly one precious message.
	
	Message*aMessage=m_inbox->at(0);
	MessageTag tag=aMessage->getTag();

	// if the message has no routing tag, then we can sefely receive it as is
	if(!isRoutingTag(tag)){
		// nothing to do
		#ifdef CONFIG_ROUTING_VERBOSITY
		cout<<__func__<<" message has no routing tag, nothing to do"<<endl;
		#endif

		return false;
	}

	// we have a routing tag
	RoutingTag routingTag=tag;

	Rank trueSource=getSource(routingTag);
	Rank trueDestination=getDestination(routingTag);

	// this is the final destination
	// we have received the message
	// we need to restore the original information now.
	if(trueDestination==m_rank){
		#ifdef CONFIG_ROUTING_VERBOSITY
		cout<<__func__<<" message has reached destination, must strip routing information"<<endl;
		#endif

		// we must update the original source and original tag
		aMessage->setSource(trueSource);
		
		MessageTag trueTag=getTag(routingTag);
		aMessage->setTag(trueTag);

		return false;
	}

	#ifdef ASSERT
	assert(m_rank!=trueDestination);
	#endif

	// at this point, we know that we need to forward
	// the message to another peer
	int nextRank=m_graph.getNextRankInRoute(trueSource,trueDestination,m_rank);

	#ifdef CONFIG_ROUTING_VERBOSITY
	cout<<__func__<<" message has been sent to the next one, trueSource="<<trueSource<<" trueDestination= "<<trueDestination<<endl;
	#endif
		
	// process the relay event if necessary
	if(m_relayCheckerActivated){
		MessageTag trueTag=getTag(routingTag);

		if(trueSource==MASTER_RANK){
			m_relayedMessagesFrom0[trueTag]++;
		}

		if(trueDestination==MASTER_RANK){
			m_relayedMessagesTo0[trueTag]++;
		}
	}

	// we forward the message
	relayMessage(aMessage,nextRank);

	// propagate the information about routing
	return true;
}

/**
 * forward a message to follow a route
 */
void MessageRouter::relayMessage(Message*message,Rank destination){
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
	assert(m_graph.isConnected(m_rank,destination));
	#endif

	m_outbox->push_back(*message);
}


/**
 * a tag is a routing tag is its routing bit is set to 1
 */
bool MessageRouter::isEnabled(){
	return m_enabled;
}

MessageRouter::MessageRouter(){
	m_enabled=false;
}

void MessageRouter::enable(StaticVector*inbox,StaticVector*outbox,RingAllocator*outboxAllocator,Rank rank,
	string prefix,int numberOfRanks,string type,int degree){

	m_relayCheckerActivated=false;
	
	m_graph.buildGraph(numberOfRanks,type,rank==MASTER_RANK,degree);

	m_size=numberOfRanks;

	cout<<endl;

	cout<<"[MessageRouter] Enabled message routing"<<endl;

	m_inbox=inbox;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_rank=rank;
	m_enabled=true;


	if(m_rank==MASTER_RANK)
		m_graph.writeFiles(prefix);
}

void MessageRouter::activateRelayChecker(){
	m_relayCheckerActivated=true;
}

void MessageRouter::addTagToCheckForRelayFrom0(MessageTag tag){
	m_tagsToCheckForRelayFrom0.insert(tag);
}

void MessageRouter::addTagToCheckForRelayTo0(MessageTag tag){
	m_tagsToCheckForRelayTo0.insert(tag);
}

bool MessageRouter::hasCompletedRelayEvents(){
	// check relay events from 0
	int expected=m_graph.getRelaysFrom0(m_rank);

	for(set<MessageTag>::iterator i=m_tagsToCheckForRelayFrom0.begin();
		i!=m_tagsToCheckForRelayFrom0.end();i++){

		MessageTag tag=*i;
		int actual=m_relayedMessagesFrom0[tag];

		if(actual!=expected){
			return false;
		}
	}
	
	// check relay events to 0
	expected=m_graph.getRelaysTo0(m_rank);

	for(set<MessageTag>::iterator i=m_tagsToCheckForRelayTo0.begin();
		i!=m_tagsToCheckForRelayTo0.end();i++){

		MessageTag tag=*i;
		int actual=m_relayedMessagesTo0[tag];

		if(actual!=expected){
			return false;
		}
	}

	return true;
}

//_-------------------------------------------------
// routing tag stuff
// TODO: should be a class

#define RAY_ROUTING_TAG_TAG_OFFSET 0
#define RAY_ROUTING_TAG_TAG_SIZE 8
#define RAY_ROUTING_TAG_SOURCE_OFFSET RAY_ROUTING_TAG_TAG_SIZE
#define RAY_ROUTING_TAG_SOURCE_SIZE 12
#define RAY_ROUTING_TAG_DESTINATION_OFFSET (RAY_ROUTING_TAG_TAG_SIZE+RAY_ROUTING_TAG_SOURCE_SIZE)
#define RAY_ROUTING_TAG_DESTINATION_SIZE 12

bool MessageRouter::isRoutingTag(MessageTag tag){
	// the only case that could be an issue is sender=0 receiver=0
	// but in this case, no routing is required (self send)
	return getSource(tag)>0||getDestination(tag)>0;
}

/**
 */
int MessageRouter::getTag(int tag){
	uint64_t data=tag;
	data<<=(sizeof(uint64_t)*8-(RAY_ROUTING_TAG_TAG_OFFSET+RAY_ROUTING_TAG_TAG_SIZE));
	data>>=(sizeof(uint64_t)*8-RAY_ROUTING_TAG_TAG_SIZE);
	return data;
}

/**
 */
int MessageRouter::getSource(int tag){
	uint64_t data=tag;
	data<<=(sizeof(uint64_t)*8-(RAY_ROUTING_TAG_SOURCE_OFFSET+RAY_ROUTING_TAG_SOURCE_SIZE));
	data>>=(sizeof(uint64_t)*8-RAY_ROUTING_TAG_SOURCE_SIZE);
	return data;
}

/**
 */
int MessageRouter::getDestination(int tag){
	uint64_t data=tag;
	data<<=(sizeof(uint64_t)*8-(RAY_ROUTING_TAG_DESTINATION_OFFSET+RAY_ROUTING_TAG_DESTINATION_SIZE));
	data>>=(sizeof(uint64_t)*8-RAY_ROUTING_TAG_DESTINATION_SIZE);
	return data;
}

/*
 * To do so, the tag attribute of a message is converted to 
 * a composite tag which contains:
 *
 * int tag
 *
 * bits 0 to 7: tag (8 bits, values from 0 to 255, 256 possible values)
 * bits 8 to 19: true source (12 bits, values from 0 to 4095, 4096 possible values)
 * bits 20 to 31: true destination (12 bits, values from 0 to 4095, 4096 possible values)
 *
 * 8+12+12=32
 */
RoutingTag MessageRouter::getRoutingTag(MessageTag tag,Rank source,Rank destination){
	uint64_t routingTag=0;

	uint64_t largeTag=tag;
	largeTag<<=RAY_ROUTING_TAG_TAG_OFFSET;
	routingTag|=largeTag;
	
	uint64_t largeSource=source;
	largeSource<<=RAY_ROUTING_TAG_SOURCE_OFFSET;
	routingTag|=largeSource;

	uint64_t largeDestination=destination;
	largeDestination<<=RAY_ROUTING_TAG_DESTINATION_OFFSET;
	routingTag|=largeDestination;

	// should be alright because we use 31 bits only.
	RoutingTag result=routingTag;

	return result;
}

ConnectionGraph*MessageRouter::getGraph(){
	return &m_graph;
}

void MessageRouter::registerPlugin(ComputeCore*core){
	m_plugin=core->allocatePluginHandle();
	PluginHandle plugin=m_plugin;

	core->setPluginName(m_plugin,"MessageRouter");
	core->setPluginDescription(m_plugin,"message router for large jobs (bundled with RayPlatform)");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU Lesser General License version 3");

	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY=core->allocateMessageTagHandle(plugin,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY");

}

void MessageRouter::resolveSymbols(ComputeCore*core){
	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON");
	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY");
	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY");

	// terminal control messages can not be routed.
	addTagToCheckForRelayFrom0(RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON);
	addTagToCheckForRelayTo0(RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY);
}
