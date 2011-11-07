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
 * \author Sébastien Boisvert 2011-11-04
 * \reviewedBy Elénie Godzaridis 2011-11-05
*/

#include <communication/MessageRouter.h>
#include <string.h> /* for memcpy */
#include <assert.h>
#include <core/common_functions.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h> /* for log */
using namespace std;

/*
#define CONFIG_ROUTER_VERBOSITY
#define ASSERT
*/

/**
 * route outcoming messages
 */
void MessageRouter::routeOutcomingMessages(){
	int numberOfMessages=m_outbox->size();

	for(int i=0;i<numberOfMessages;i++){
		Message*aMessage=m_outbox->at(i);

		Tag communicationTag=aMessage->getTag();

		// - first, the message may have been already routed when it was received (also
		// in a routed version). In this case, nothing must be done.
		if(isRoutingTag(communicationTag)){
			#ifdef CONFIG_ROUTER_VERBOSITY
			cout<<__func__<<" Message has already a routing tag."<<endl;
			#endif
			continue;
		}

		// at this point, the message has no routing information yet.
		Rank trueSource=aMessage->getSource();
		Rank trueDestination=aMessage->getDestination();

		// if it is reachable, no further routing is required
		if(isConnected(trueSource,trueDestination)){
			#ifdef CONFIG_ROUTER_VERBOSITY
			cout<<__func__<<" Rank "<<trueSource<<" can reach "<<trueDestination<<" without routing"<<endl;
			#endif
			continue;
		}
	
		// re-route the message by re-writing the tag
		RoutingTag routingTag=getRoutingTag(communicationTag,trueSource,trueDestination);
		aMessage->setTag(routingTag);

		Rank nextRank=getNextRankInRoute(trueSource,trueDestination,m_rank);
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
	Tag tag=aMessage->getTag();

	// if the message has no routing tag, then we can sefely receive it as is
	if(!isRoutingTag(tag)){
		// nothing to do
		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" message has no routing tag, nothing to do"<<endl;
		#endif

		return;
	}

	// we have a routing tag
	RoutingTag routingTag=tag;

	Rank trueSource=getSource(routingTag);
	Rank trueDestination=getDestination(routingTag);

	// this is the final destination
	// we have received the message
	// we need to restore the original information now.
	if(trueDestination==m_rank){
		#ifdef CONFIG_ROUTER_VERBOSITY
		cout<<__func__<<" message has reached destination, must strip routing information"<<endl;
		#endif

		// we must update the original source and original tag
		aMessage->setSource(trueSource);
		
		Tag trueTag=getTag(routingTag);
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
		
	// process the relay event if necessary
	if(m_relayCheckerActivated){
		Tag trueTag=getTag(routingTag);

		if(trueSource==MASTER_RANK){
			m_relayedMessagesFrom0[trueTag]++;
		}

		if(trueDestination==MASTER_RANK){
			m_relayedMessagesTo0[trueTag]++;
		}
	}

	// we forward the message
	relayMessage(aMessage,nextRank);
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
	assert(isConnected(m_rank,destination));
	#endif

	m_outbox->push_back(*message);
}

/**
 * Make connections with a given type
 */
void MessageRouter::makeConnections(string type){
	cout<<"[MessageRouter::makeConnections] type: "<<type<<endl;

	// append empty sets
	for(Rank i=0;i<m_size;i++){
		set<Rank> a;
		m_connections.push_back(a);
	}

	if(type=="random"){
		makeConnections_random();
	}else if(type=="group"){
		makeConnections_group();
	}else if(type=="complete"){
		makeConnections_complete();
	}else{// the default is random
		makeConnections_random();
	}
}

/**
 * complete graph
 */
void MessageRouter::makeConnections_complete(){
	for(Rank i=0;i<m_size;i++){
		for(Rank j=0;j<m_size;j++){
			m_connections[i].insert(j);
		}
	}
}

/**
 * create random connections
 */
void MessageRouter::makeConnections_random(){
	srand(4);

	// insert self
	for(Rank i=0;i<m_size;i++)
		m_connections[i].insert(i);

	int connectionsPerVertex=log(m_size)/log(2)/2;

	cout<<"[MessageRouter] vertices: "<<m_size<<endl;
	cout<<"[MessageRouter] connectionsPerVertex: "<<connectionsPerVertex<<endl;

	for(int connectionNumber=0;connectionNumber<connectionsPerVertex;connectionNumber++){
		for(Rank source=0;source<m_size;source++){

			// add an edge bool added=false;
			bool added=false;
			while(!added){
				Rank destination=rand()%m_size;

				// if already set, find another one
				if(m_connections[source].count(destination)>0)
					continue;
			
				m_connections[source].insert(destination);
				m_connections[destination].insert(source);
				added=true;
			}
		}
	}
}

/**
 * Get an intermediate for the type group
 */
int MessageRouter::getIntermediateRank(Rank rank){
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
void MessageRouter::makeConnections_group(){
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

	for(Rank source=0;source<m_size;source++){
		int intermediateSource=getIntermediateRank(source);
	
		// can connect with self.
		m_connections[source].insert(source);

		// can connect with the intermediate source
		m_connections[source].insert(intermediateSource);

		for(Rank destination=0;destination<m_size;destination++){

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
		}
	}
}

/**
 * Dijkstra's algorithm
 * All weights are 1
 */
void MessageRouter::findShortestPath(Rank source,Rank destination,vector<Rank>*route){

	// assign tentative distances
	map<Rank,Distance> tentativeDistances;
	
	for(Rank i=0;i<m_size;i++)
		tentativeDistances[i]=9999;

	tentativeDistances[source]=0;

	map<Rank,Rank> previousVertices;

	// create a set of unvisited vertices
	set<Rank> unvisited;

	for(Rank i=0;i<m_size;i++)
		unvisited.insert(i);

	// create a current vertex
	Rank current=source;

	// create an index of distances
	map<Distance,set<Rank> > verticesWithDistance;

	for(map<Rank,Distance>::iterator i=tentativeDistances.begin();i!=tentativeDistances.end();i++){
		verticesWithDistance[i->second].insert(i->first);
	}

	while(!unvisited.empty()){
	
		// calculate the tentative distance
		// of each neighbors of the current
		for(set<Rank>::iterator neighbor=m_connections[current].begin();
			neighbor!=m_connections[current].end();neighbor++){
			Rank theNeighbor=*neighbor;

			// we are only interested in unvisited neighbors
			if(unvisited.count(theNeighbor)>0){
				Distance newDistance=tentativeDistances[current]+1;
				Distance oldDistance=tentativeDistances[theNeighbor];

				// the new distance is better
				// if the oldDistance for theNeighbor and the newDistance
				// for theNeighbor in respect to current are equal, then
				// choose the one having the previousVertex with the least
				// relay events. The previous vertex is current or the one
				// stored in previousVertices
				if(newDistance < oldDistance || 
					// distances are equal and current has less relay events
				(newDistance==oldDistance && previousVertices.count(theNeighbor)>0
				&& m_relayEvents[current] < m_relayEvents[previousVertices[theNeighbor]])){

					tentativeDistances[theNeighbor]=newDistance;
					previousVertices[theNeighbor]=current;

					// update the distance index
					verticesWithDistance[oldDistance].erase(theNeighbor);
					verticesWithDistance[newDistance].insert(theNeighbor);
				}
			}
		}

		// mark the current vertex as not used
		unvisited.erase(current);

		// remove it as well from the index
		Distance theDistance=tentativeDistances[current];
		verticesWithDistance[theDistance].erase(current);

		if(verticesWithDistance[theDistance].size()==0)
			verticesWithDistance.erase(theDistance);

		// the next current is the one in unvisited vertices
		// with the lowest distance
		
		Distance bestDistance=-1;

		// find it using the index
		// the index contains only unvisited vertices
		for(map<Distance,set<Rank> >::iterator myIterator=verticesWithDistance.begin();
			myIterator!=verticesWithDistance.end();myIterator++){

			Distance theDistance=myIterator->first;

			// we are done if all the remaining distances are greater
			if(bestDistance!=-1 && theDistance > bestDistance)
				break;

			// find a vertex with the said distance
			for(set<Rank>::iterator i=myIterator->second.begin();
				i!=myIterator->second.end();i++){
				Rank vertex=*i;

				if(theDistance < bestDistance || bestDistance==-1){
					current=vertex;
					bestDistance=tentativeDistances[vertex];

					// we can break because all the other remaining 
					// for this distance have the same distance (obviously)
					break;
				}
			}
		}
	}

	// generate the route
	current=destination;
	while(current!=source){
		route->push_back(current);
		current=previousVertices[current];
	}

	route->push_back(source);

	// invert the route
	// because the one we have is from destination to source
	int left=0;
	int right=route->size()-1;
	while(left<right){
		Rank t=(*route)[left];
		(*route)[left]=(*route)[right];
		(*route)[right]=t;
		left++;
		right--;
	}

	#ifdef CONFIG_ROUTER_VERBOSITY
	// print the best distance
	cout<<"Shortest path from "<<source<<" to "<<destination<<" is "<<tentativeDistances[destination]<<"	";
	cout<<"Path:	"<<route->size()<<"	";
	for(int i=0;i<(int)route->size();i++){
		cout<<" "<<route->at(i);
	}
	cout<<endl;
	#endif
}

/**
 * Print a route
 */
void MessageRouter::printRoute(Rank source,Rank destination){
	cout<<"[printRoute] Source: "<<source<<"	Destination: "<<destination<<"	";

	vector<Rank> route;
	getRoute(source,destination,&route);

	cout<<"Size: "<<route.size()<<"	Route: ";

	for(int i=0;i<(int)route.size();i++){
		if(i!=0)
			cout<<" ";
		cout<<route[i];
	}
	cout<<"	Hops: "<<route.size()-1<<endl;
}

/**
 * Compute the routing tables.
 * This is done for all pairs of ranks
 */
void MessageRouter::makeRoutes(){

	// initialize the relay events
	for(Rank source=0;source<m_size;source++){
		m_relayEvents.push_back(0);
		m_relayEventsTo0.push_back(0);
		m_relayEventsFrom0.push_back(0);
	}

	// append empty routes
	for(Rank i=0;i<m_size;i++){
		vector<map<Rank,Rank> > a;
		for(Rank j=0;j<m_size;j++){
			map<Rank,Rank> b;
			a.push_back(b);
		}
		m_routes.push_back(a);
	}

	#ifdef CONFIG_ROUTER_VERBOSITY
	int step=m_size/60+1;
	#endif

	for(Rank source=0;source<m_size;source++){

		#ifndef CONFIG_ROUTER_VERBOSITY
		cout<<"[MessageRouter::makeRoutes] "<<source<<" "<<endl;
		#endif

		for(Rank destination=0;destination<m_size;destination++){
			#ifdef CONFIG_ROUTER_VERBOSITY
			if(destination%step==0){
				cout<<"*";
				cout.flush();
			}
			#endif

			vector<Rank> route;
			findShortestPath(source,destination,&route);

			for(int i=0;i<(int)route.size()-1;i++){
				// add the route
				m_routes[source][destination][route[i]]=route[i+1];
			}

			// add the relay information
			// the relay ranks are all the ranks in the route
			// minus the source and minus the destination
			for(int i=1;i<(int)route.size()-1;i++){
				Rank relayRank=route[i];

				// general relay data
				m_relayEvents[relayRank]++;

				// relay data from 0
				if(source==MASTER_RANK)
					m_relayEventsFrom0[relayRank]++;

				// relay data to 0
				if(destination==MASTER_RANK)
					m_relayEventsTo0[relayRank]++;
			}

			#ifdef CONFIG_ROUTER_VERBOSITY
			printRoute(source,destination);

			printRoute(destination,source);
			#endif
		}

		#ifdef CONFIG_ROUTER_VERBOSITY
		double ratio=source*100.0/m_size;
		cout<<" "<<ratio<<"%"<<endl;
		#endif
	}
}

/**
 * a rank can only speak to things listed in connections
 */
bool MessageRouter::isConnected(Rank source,Rank destination){
	// check that a connection exists
	return m_connections[source].count(destination)>0;
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
	string prefix,int numberOfRanks,int coresPerNode,string type){

	m_relayCheckerActivated=false;

	m_coresPerNode=coresPerNode;
	m_size=numberOfRanks;

	cout<<endl;

	cout<<"[MessageRouter] Enabled message routing"<<endl;

	m_inbox=inbox;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_rank=rank;
	m_enabled=true;

	// generate the connections
	makeConnections(type);

	// generate the routes
	makeRoutes();

	if(m_rank==0)
		writeFiles(prefix);
}

/**
 * Write files.
 */
void MessageRouter::writeFiles(string prefix){
	// dump the connections in a file
	ostringstream file;
	file<<prefix<<"Routing.Connections.txt";
	ofstream f(file.str().c_str());

	f<<"#Rank	Count	Connections"<<endl;

	for(Rank rank=0;rank<m_size;rank++){
		f<<rank<<"	"<<m_connections[rank].size()<<"	";

		for(set<Rank>::iterator i=m_connections[rank].begin();
			i!=m_connections[rank].end();i++){
			if(i!=m_connections[rank].begin())
				f<<" ";
			f<<*i;
		}
		f<<endl;
	}

	f.close();

	// dump the routes in a file
	ostringstream file2;
	file2<<prefix<<"Routing.Routes.txt";
	ofstream f2(file2.str().c_str());
	f2<<"#Source	Destination	Hops	Route"<<endl;

	for(Rank rank=0;rank<m_size;rank++){
		for(Rank i=0;i<m_size;i++){
			vector<Rank> route;
			getRoute(rank,i,&route);
			f2<<rank<<"	"<<i<<"	"<<route.size()-1<<"	";

			for(int i=0;i<(int)route.size();i++){
				if(i!=0)
					f2<<" ";
				f2<<route[i];
			}

			f2<<endl;
		}
	}

	f2.close();

	// write relay events
	ostringstream file3;
	file3<<prefix<<"Routing.RelayEvents.txt";
	ofstream f3(file3.str().c_str());
	f3<<"#Source	RelayEvents"<<endl;

	for(Rank rank=0;rank<m_size;rank++){
		f3<<rank<<"	"<<m_relayEvents[rank]<<endl;
	}

	f3.close();
}

/**
 * get the route between two points
 */
void MessageRouter::getRoute(Rank source,Rank destination,vector<int>*route){
	int currentVertex=source;
	route->push_back(currentVertex);

	while(currentVertex!=destination){
		currentVertex=getNextRankInRoute(source,destination,currentVertex);
		route->push_back(currentVertex);
	}
}


int MessageRouter::getNextRankInRoute(Rank source,Rank destination,Rank rank){
	#ifdef ASSERT
	assert(m_routes[source][destination].count(rank)==1);
	#endif

	return m_routes[source][destination][rank];
}

void MessageRouter::getConnections(Rank source,vector<Rank>*connections){
	for(set<Rank>::iterator i=m_connections[m_rank].begin();
		i!=m_connections[m_rank].end();i++){
		connections->push_back(*i);
	}
}

void MessageRouter::activateRelayChecker(){
	m_relayCheckerActivated=true;
}

void MessageRouter::addTagToCheckForRelayFrom0(Tag tag){
	m_tagsToCheckForRelayFrom0.insert(tag);
}

void MessageRouter::addTagToCheckForRelayTo0(Tag tag){
	m_tagsToCheckForRelayTo0.insert(tag);
}

bool MessageRouter::hasCompletedRelayEvents(){
	// check relay events from 0
	int expected=m_relayEventsFrom0[m_rank];

	for(set<Tag>::iterator i=m_tagsToCheckForRelayFrom0.begin();
		i!=m_tagsToCheckForRelayFrom0.end();i++){

		Tag tag=*i;
		int actual=m_relayedMessagesFrom0[tag];

		if(actual!=expected){
			return false;
		}
	}
	
	// check relay events to 0
	expected=m_relayEventsTo0[m_rank];

	for(set<Tag>::iterator i=m_tagsToCheckForRelayTo0.begin();
		i!=m_tagsToCheckForRelayTo0.end();i++){

		Tag tag=*i;
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

bool MessageRouter::isRoutingTag(Tag tag){
	// the only case that could be an issue is sender=0 receiver=0
	// but in this case, no routing is required (self send)
	return getSource(tag)>0||getDestination(tag)>0;
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
RoutingTag MessageRouter::getRoutingTag(Tag tag,Rank source,Rank destination){
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

