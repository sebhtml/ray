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

#include <routing/GraphImplementation.h>
#include <iostream>
#include <stdlib.h> /* srand, rand */
#include <algorithm> /* random_shuffle */
using namespace std;

void GraphImplementation::getOutcomingConnections(Rank source,vector<Rank>*connections){
	for(set<Rank>::iterator i=m_outcomingConnections[source].begin();
		i!=m_outcomingConnections[source].end();i++){
		connections->push_back(*i);
	}
}

void GraphImplementation::getIncomingConnections(Rank source,vector<Rank>*connections){
	for(set<Rank>::iterator i=m_incomingConnections[source].begin();
		i!=m_incomingConnections[source].end();i++){
		connections->push_back(*i);
	}
}

/**
 * Dijkstra's algorithm
 * All weights are 1
 *
 * furthermore, we minimize saturation of vertices
 * by minimizing the relay points
 */
void GraphImplementation::findShortestPath(Rank source,Rank destination,vector<Rank>*route){

	// same vertex
	if(source==destination){
		route->push_back(source);
		route->push_back(destination);
		return;
	}

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
		vector<int> connections;

		getOutcomingConnections(current,&connections);

		for(vector<Rank>::iterator neighbor=connections.begin();
			neighbor!=connections.end();neighbor++){
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

		// here we find the next current vertex
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

				// the distance is lower or no distance
				// was processed so far
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

		/** this should not happen... */
		if(previousVertices.count(current)==0){
			cout<<"Error, current has no previous vertex:";
			cout<<" there is no route between "<<source<<" and "<<destination<<endl;
			route->clear();
			return;
		}

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
}

void GraphImplementation::getRoute(Rank source,Rank destination,vector<Rank>*route){

	Rank currentVertex=source;
	route->push_back(currentVertex);

	while(currentVertex!=destination){
		currentVertex=getNextRankInRoute(source,destination,currentVertex);
		route->push_back(currentVertex);
	}
}

/**
 * Compute the routing tables.
 * This is done for all pairs of ranks
 */
void GraphImplementation::computeRoutes(){
	// initialize the relay events
	for(Rank source=0;source<m_size;source++){
		m_relayEvents.push_back(0);
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

	// make a liste of pairs
	vector<vector<Rank> > pairs;

	for(Rank source=0;source<m_size;source++){
		for(Rank destination=0;destination<m_size;destination++){
			vector<Rank> pair;
			pair.push_back(source);
			pair.push_back(destination);
			pairs.push_back(pair);
		}
	}

	// shuffle the list
	// we need the same seed on all ranks
	// we shuffle a lot 
	for(int i=0;i<32;i++){
		srand(i*i + i*i*i);
		std::random_shuffle(pairs.begin(),pairs.end());
	}

	if(m_verbose)
		cout<<"Computing routes, please wait..."<<endl;

	int done=0;

	// compute routes using the random order
	for(int i=0;i<(int)pairs.size();i++){

		if(done%100==0 && m_verbose)
			cout<<"makeRoutes "<<done<<"/"<<pairs.size()<<" "<<done/(0.0+pairs.size())*100<<"%"<<endl;

		Rank source=pairs[i][0];
		Rank destination=pairs[i][1];

		vector<Rank> route;

		computeRoute(source,destination,&route);

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
		}

		done++;
	}

	cout<<"makeRoutes "<<done<<"/"<<pairs.size()<<" "<<done/(0.0+pairs.size())*100<<"%"<<endl;
}

void GraphImplementation::computeRelayEvents(){

	m_relayEvents.clear();
	m_relayEventsFrom0.clear();
	m_relayEventsTo0.clear();

	// initialize the relay events
	for(Rank source=0;source<m_size;source++){
		m_relayEvents.push_back(0);
		m_relayEventsTo0.push_back(0);
		m_relayEventsFrom0.push_back(0);
	}

	for(Rank source=0;source<m_size;source++){
		if(source%100==0)
			cout<<source<<"/"<<m_size<<endl;
		for(Rank destination=0;destination<m_size;destination++){
			vector<Rank> route;
			getRoute(source,destination,&route);

			// add the relay information
			// the relay ranks are all the ranks in the route
			// minus the source and minus the destination
			for(int i=1;i<(int)route.size()-1;i++){
				Rank relayRank=route[i];

				// general relay data
				m_relayEvents[relayRank]++;

				// relay data from 0
				if(source==0)
					m_relayEventsFrom0[relayRank]++;
	
				// relay data to 0
				if(destination==0)
					m_relayEventsTo0[relayRank]++;
			}
	
		}
	}
	
	cout<<m_size<<"/"<<m_size<<endl;
}

int GraphImplementation::getRelaysFrom0(Rank rank){
	return m_relayEventsFrom0[rank];
}

int GraphImplementation::getRelaysTo0(Rank rank){
	return m_relayEventsTo0[rank];
}

int GraphImplementation::getRelays(Rank rank){
	return m_relayEvents[rank];
}

void GraphImplementation::setVerbosity(bool verbosity){
	m_verbose=verbosity;
}

