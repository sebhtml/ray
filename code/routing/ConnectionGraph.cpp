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

#include <routing/ConnectionGraph.h>

/**
 * Make connections with a given type
 */
void ConnectionGraph::makeConnections(string type){
	if(m_verbose)
		cout<<"[ConnectionGraph::makeConnections] type: "<<type<<endl;

	// append empty sets
	for(Rank i=0;i<m_size;i++){
		set<Rank> a;
		m_connections.push_back(a);
	}

	// insert self
	for(Rank i=0;i<m_size;i++)
		m_connections[i].insert(i);

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
void ConnectionGraph::makeConnections_complete(){
	for(Rank i=0;i<m_size;i++){
		for(Rank j=0;j<m_size;j++){
			m_connections[i].insert(j);
		}
	}
}

/**
 * create random connections
 */
void ConnectionGraph::makeConnections_random(){
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
	int connectionsPerVertex=log(m_size)/log(2);
	int numberOfEdgesToAdd=m_size*connectionsPerVertex/2;

	// the first numberOfEdgesToAdd edges
	for(int i=0;i<numberOfEdgesToAdd;i++){
		int identifier=identifiers[i];
		Rank source=edges[identifier][0];
		Rank destination=edges[identifier][1];

		// add the edge in both directions
		m_connections[source].insert(destination);
		m_connections[destination].insert(source);
	}
}

/**
 * Get an intermediate for the type group
 */
int ConnectionGraph::getIntermediateRank(Rank rank){
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
void ConnectionGraph::makeConnections_group(){
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
void ConnectionGraph::findShortestPath(Rank source,Rank destination,vector<Rank>*route){

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

/**
 * Print a route
 */
void ConnectionGraph::printRoute(Rank source,Rank destination){
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
void ConnectionGraph::makeRoutes(){

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
			if(source==0)
				m_relayEventsFrom0[relayRank]++;

			// relay data to 0
			if(destination==0)
				m_relayEventsTo0[relayRank]++;
		}

		done++;
	}
}

/**
 * a rank can only speak to things listed in connections
 */
bool ConnectionGraph::isConnected(Rank source,Rank destination){
	// check that a connection exists
	return m_connections[source].count(destination)>0;
}

/**
 * Write files.
 */
void ConnectionGraph::writeFiles(string prefix){
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

	// dump the routes in a file
	ostringstream file4;
	file4<<prefix<<"Routing.Summary.txt";
	ofstream f4(file4.str().c_str());

	int numberOfVertices=m_size;

	int numberOfEdges=0;

	vector<int> connectivities;

	for(Rank i=0;i<m_size;i++){
		vector<Rank> connections;

		getConnections(i,&connections);

		// remove the self edge
		connectivities.push_back(connections.size()-1);

		for(Rank j=0;j<m_size;j++){
			// we only count the edges with i >= j
			// because (i,j) and (j,i) are the same
			if(i<j)
				continue;
	
			if(isConnected(i,j))
				numberOfEdges++;
		}
	}

	f4<<"Type: "<<m_type<<endl;
	f4<<endl;

	f4<<"NumberOfVertices: "<<numberOfVertices<<endl;
	f4<<"NumberOfEdges: "<<numberOfEdges-m_size<<endl;
	f4<<"NumberOfEdgesInCompleteGraph: "<<numberOfVertices*(numberOfVertices-1)/2<<endl;
	f4<<endl;
	f4<<"NumberOfConnectionsPerVertex"<<endl;
	f4<<"   Frequencies:"<<endl;

	map<int,int> connectionFrequencies;
	for(Rank i=0;i<m_size;i++){
		vector<Rank> connections;
		getConnections(i,&connections);
		connectionFrequencies[connections.size()-1]++;
	}

	int totalForEdges=0;

	for(map<int,int>::iterator i=connectionFrequencies.begin();
		i!=connectionFrequencies.end();i++){
		totalForEdges+=i->second;
	}

	for(map<int,int>::iterator i=connectionFrequencies.begin();
		i!=connectionFrequencies.end();i++){
		f4<<"        "<<i->first<<"    "<<i->second<<"    "<<i->second*100.0/totalForEdges<<"%"<<endl;
	}
	
	f4<<"        "<<"Total"<<"    "<<totalForEdges<<"    100.00%"<<endl;

	f4<<"   Average: "<<getAverage(&connectivities)<<endl;
	f4<<"   StandardDeviation: "<<getStandardDeviation(&connectivities)<<endl;

	f4<<endl;
	f4<<"NumberOfRelayEventsPerVertex"<<endl;

	f4<<"   Average: "<<getAverage(&m_relayEvents)<<endl;
	f4<<"   StandardDeviation: "<<getStandardDeviation(&m_relayEvents)<<endl;

	f4<<endl;
	f4<<"RouteLength"<<endl;

	f4<<"   Frequencies:"<<endl;
	map<int,int> pathLengths;

	for(Rank i=0;i<m_size;i++){
		for(Rank j=0;j<m_size;j++){
			vector<Rank> route;
			getRoute(i,j,&route);

			// we remove the source vertex
			pathLengths[route.size()-1]++;
		}
	}

	int totalForPaths=0;
	for(map<int,int>::iterator i=pathLengths.begin();
		i!=pathLengths.end();i++){
		totalForPaths+=i->second;
	}

	for(map<int,int>::iterator i=pathLengths.begin();
		i!=pathLengths.end();i++){
		f4<<"        "<<i->first<<"    "<<i->second<<"    "<<i->second*100.0/totalForPaths<<"%"<<endl;
	}
	f4<<"        "<<"Total"<<"    "<<totalForPaths<<"    100.00%"<<endl;

	f4.close();
}

/**
 * get the route between two points
 */
void ConnectionGraph::getRoute(Rank source,Rank destination,vector<int>*route){
	int currentVertex=source;
	route->push_back(currentVertex);

	while(currentVertex!=destination){
		currentVertex=getNextRankInRoute(source,destination,currentVertex);
		route->push_back(currentVertex);
	}
}

int ConnectionGraph::getNextRankInRoute(Rank source,Rank destination,Rank rank){
	#ifdef ASSERT
	assert(m_routes[source][destination].count(rank)==1);
	#endif

	return m_routes[source][destination][rank];
}

void ConnectionGraph::getConnections(Rank source,vector<Rank>*connections){
	for(set<Rank>::iterator i=m_connections[source].begin();
		i!=m_connections[source].end();i++){
		connections->push_back(*i);
	}
}

void ConnectionGraph::removeUnusedConnections(){
	// clear connections
	for(Rank source=0;source<m_size;source++){
		m_connections[source].clear();

		// add self
		m_connections[source].insert(source);
	}

	// generate connections using the routes
	for(Rank source=0;source<m_size;source++){
		for(Rank destination=0;destination<m_size;destination++){
			vector<Rank> route;
			getRoute(source,destination,&route);
			for(int i=0;i<(int)route.size()-1;i++){
				Rank rank1=route[i];
				Rank rank2=route[i+1];
	
				// add the connections
				m_connections[rank1].insert(rank2);
				m_connections[rank2].insert(rank1);
			}
		}
	}
}

void ConnectionGraph::buildGraph(int numberOfRanks,string type,int groupSize,bool verbosity){

	m_verbose=verbosity;

	m_size=numberOfRanks;

	// only used for type 'group'
	m_coresPerNode=groupSize;

	if(type=="")
		type="random";

	// generate the connections
	makeConnections(type);

	// generate the routes
	makeRoutes();

	removeUnusedConnections();
}

int ConnectionGraph::getRelaysFrom0(Rank rank){
	return m_relayEventsFrom0[rank];
}

int ConnectionGraph::getRelaysTo0(Rank rank){
	return m_relayEventsTo0[rank];
}
