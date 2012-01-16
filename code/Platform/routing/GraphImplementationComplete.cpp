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

#include <routing/GraphImplementationComplete.h>

/**
 * complete graph
 */
void GraphImplementationComplete::makeConnections(int n){
	m_size=n;

	for(int i=0;i<m_size;i++){
		set<Rank> b;
		m_incomingConnections.push_back(b);
		m_outcomingConnections.push_back(b);
	}

	for(Rank i=0;i<m_size;i++){
		for(Rank j=0;j<m_size;j++){
			if(i==j)
				continue;

			m_outcomingConnections[i].insert(j);
			m_incomingConnections[j].insert(i);
		}
	}
}

void GraphImplementationComplete::computeRoute(Rank a,Rank b,vector<Rank>*route){
	/* not necessary */
}

void GraphImplementationComplete::makeRoutes(){
	/* no routes are computed */

	computeRelayEvents();
}

Rank GraphImplementationComplete::getNextRankInRoute(Rank source,Rank destination,Rank rank){
	return destination;
}

bool GraphImplementationComplete::isConnected(Rank source,Rank destination){
	return true;
}


