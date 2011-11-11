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

#ifndef _GraphImplementation_h
#define _GraphImplementation_h

#include <vector>
#include <set>
#include <core/types.h>
using namespace std;

/** virtual interface for graphs */
class GraphImplementation{

protected:
	bool m_verbose;

	int m_size;

/**
 * connections
 */
	vector<set<Rank> > m_connections;

public:

	virtual void makeConnections(int n) =0;

/**
 * Get the connections for a source
 */
	void getConnections(Rank source,vector<Rank>*connections);

	bool isConnected(Rank i,Rank j);

	virtual ~GraphImplementation(){ /* nothing */} /* and no trailing ; */

};

#endif

