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

#ifndef _GraphImplementationKautz_h
#define _GraphImplementationKautz_h

#include <routing/GraphImplementation.h>
#include <vector>
#include <stdint.h>
using namespace std;

/**
 * Kautz graph
 * n must be a power of something
 * \see http://www.sciencedirect.com/science/article/pii/S0140366497000741
 *
 * \see http://pl.atyp.us/wordpress/index.php/2007/12/the-kautz-graph/
 *
 * \see http://planetmath.org/encyclopedia/KautzGraph.html
 *
 * \see http://www.sciencedirect.com/science/article/pii/089812219500146P
 *
 */
class GraphImplementationKautz: public GraphImplementation{

	vector<Tuple> m_graphToKautz;

	map<int,int> m_kautzToGraph;

	int m_degree;

	int m_base;

	int m_diameter;

	void configureGraph(int n);

	int getPower(int base,int exponent);

/** convert a number to another base */
	void convertToBase(int i,Tuple*tuple);

/** convert a vertex to base 10 */
	int convertToBase10(Tuple*vertex);

	void printVertex(Tuple*a);
	bool isAPowerOf(int n,int base);

	int getMaximumOverlap(Tuple*a,Tuple*b);

	Rank computeNextRankInRoute(Rank source,Rank destination,Rank rank);
	bool computeConnection(Rank source,Rank destination);
	bool isAKautzVertex(Tuple*vertex);

protected:

	void computeRoute(Rank a,Rank b,vector<Rank>*route);
	Rank getNextRankInRoute(Rank source,Rank destination,Rank rank);
	bool isConnected(Rank source,Rank destination);
public:

	void makeConnections(int n);
	void makeRoutes();

	bool isValid(int n);
};

#endif
