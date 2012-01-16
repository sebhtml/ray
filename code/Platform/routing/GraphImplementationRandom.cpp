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

#include <routing/GraphImplementationRandom.h>
#include <math.h> /* for log */
#include <algorithm> /* random_shuffle */
#include <assert.h>

void GraphImplementationRandom::makeConnections(int n){
	m_size=n;

	for(int i=0;i<m_size;i++){
		set<Rank> b;
		m_outcomingConnections.push_back(b);
		m_incomingConnections.push_back(b);
	}

	// create a set of all edges
	vector<vector<Rank> > edges;
	vector<int> identifiers;
	int k=0;
	for(Rank i=0;i<m_size;i++){
		for(Rank j=0;j<m_size;j++){
			// don't generate a pair for (i,i)
			if(i==j)
				continue;
		
			// don't generate a pair for (i,j) if i<j because
			// (j,i) will be processed anyway
			if(i<j)
				continue;

			vector<Rank> pair;
			pair.push_back(i);
			pair.push_back(j);
			edges.push_back(pair);

			identifiers.push_back(k);

			k++;
		}
	}

	// shuffle the edges
	// we shuffle a lot
	for(int i=0;i<32;i++){
		srand(i*i*i+2*i);
		std::random_shuffle(identifiers.begin(),identifiers.end());
	}

	// add the edges
	int connectionsPerVertex=(int) (log(m_size)/log(2));
	int numberOfEdgesToAdd=m_size*connectionsPerVertex/2;

	// the first numberOfEdgesToAdd edges
	for(int i=0;i<numberOfEdgesToAdd;i++){
		int identifier=identifiers[i];
		Rank source=edges[identifier][0];
		Rank destination=edges[identifier][1];

		// add the edge in both directions
		m_outcomingConnections[source].insert(destination);
		m_outcomingConnections[destination].insert(source);

		m_incomingConnections[destination].insert(source);
		m_incomingConnections[source].insert(destination);
	}
}

void GraphImplementationRandom::computeRoute(Rank a,Rank b,vector<Rank>*route){
	findShortestPath(a,b,route);
}

Rank GraphImplementationRandom::getNextRankInRoute(Rank source,Rank destination,Rank rank){
	#ifdef ASSERT
	assert(m_routes[source][destination].count(rank)==1);
	#endif

	return m_routes[source][destination][rank];
}

void GraphImplementationRandom::makeRoutes(){
	computeRoutes();

	computeRelayEvents();
}

bool GraphImplementationRandom::isConnected(Rank source,Rank destination){
	// communicating with itself is always allowed
	if(source==destination)
		return true;

	// check that a connection exists
	return m_outcomingConnections[source].count(destination)>0;
}


