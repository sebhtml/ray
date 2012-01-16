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

#ifndef _ConnectionGraph
#define _ConnectionGraph

#include <vector>
#include <set>
#include <map>
#include <core/statistics.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <routing/GraphImplementation.h>
#include <routing/GraphImplementationRandom.h>
#include <routing/GraphImplementationGroup.h>
#include <routing/GraphImplementationDeBruijn.h>
#include <routing/GraphImplementationKautz.h>
#include <routing/GraphImplementationExperimental.h>
#include <routing/GraphImplementationComplete.h>
#include <string>
#include <core/types.h>
using namespace std;

/** a graph for connections between compute cores */
class ConnectionGraph{

	string m_type;

	GraphImplementation*m_implementation;

	GraphImplementationRandom m_random;
	GraphImplementationComplete m_complete;
	GraphImplementationDeBruijn m_deBruijn;
	GraphImplementationKautz m_kautz;
	GraphImplementationExperimental m_experimental;
	GraphImplementationGroup m_group;

/** verbosity */
	bool m_verbose;

/**
 * The number of ranks
 */
	int m_size;

	/************************************************/
	/** methods to build connections */

	/** print a route */
	void printRoute(Rank source,Rank destination);

public:

/**
 * Get the next rank to relay the message after <rank> for the route between
 * source and destination
 */
	Rank getNextRankInRoute(Rank source,Rank destination,Rank rank);

/**
 * Is there a up-link between a source and a destination
 */
	bool isConnected(Rank source,Rank destination);

	void writeFiles(string prefix);

/** build the graph. */
	void buildGraph(int numberOfVertices,string method,bool verbosity,int degree);


	/** get the number of paths that contain rank from 0 to any vertex */
	int getRelaysFrom0(Rank rank);

	/** get the number of paths that contain rank from any vertex to vertex 0  */
	int getRelaysTo0(Rank rank);

	void getIncomingConnections(Rank i,vector<Rank>*connections);

};

#endif
