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

#ifndef _GraphImplementationRandom_h
#define _GraphImplementationRandom_h

#include <routing/GraphImplementation.h>

/**
 * random graph with n*log(n)/2 edges
 */
class GraphImplementationRandom : public GraphImplementation{

protected:
	void computeRoute(Rank a,Rank b,vector<Rank>*route);

	Rank getNextRankInRoute(Rank source,Rank destination,Rank rank);
	bool isConnected(Rank source,Rank destination);
public:

	void makeConnections(int n);
	void makeRoutes();

};

#endif
