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

#include <routing/GraphImplementationGroup.h>
#include <assert.h>

/**
 * Get an intermediate for the type group
 */
int GraphImplementationGroup::getIntermediateRank(Rank rank){
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
void GraphImplementationGroup::makeConnections(int n){
	m_coresPerNode=8;

	m_size=n;

	for(int i=0;i<m_size;i++){
		set<Rank> b;
		m_incomingConnections.push_back(b);
		m_outcomingConnections.push_back(b);
	}


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
	
		// can connect with the intermediate source
		m_outcomingConnections[source].insert(intermediateSource);
		m_incomingConnections[intermediateSource].insert(source);

		for(Rank destination=0;destination<m_size;destination++){

			int intermediateDestination=getIntermediateRank(destination);

			// an intermediate node can connect with any intermediate node
			if(destination==intermediateDestination && source==intermediateSource){
				m_outcomingConnections[source].insert(intermediateDestination);
				m_incomingConnections[intermediateDestination].insert(source);
			}
			
			// if the source is the intermediate destination, add a link
			// this is within the same group
			if(source==intermediateDestination){
				m_outcomingConnections[source].insert(destination);
				m_incomingConnections[destination].insert(source);
			}

			// peers in the same group are allowed to connect
			if(intermediateSource==intermediateDestination){
				m_outcomingConnections[source].insert(destination);
				m_incomingConnections[destination].insert(source);
			}
		}
	}
}

void GraphImplementationGroup::computeRoute(Rank a,Rank b,vector<Rank>*route){
	findShortestPath(a,b,route);
}

void GraphImplementationGroup::makeRoutes(){
	computeRoutes();

	computeRelayEvents();
}

Rank GraphImplementationGroup::getNextRankInRoute(Rank source,Rank destination,Rank rank){
	#ifdef ASSERT
	assert(m_routes[source][destination].count(rank)==1);
	#endif

	return m_routes[source][destination][rank];
}

bool GraphImplementationGroup::isConnected(Rank source,Rank destination){
	// communicating with itself is always allowed
	if(source==destination)
		return true;

	// check that a connection exists
	return m_outcomingConnections[source].count(destination)>0;
}


