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

#include <routing/GraphImplementation.h>

bool GraphImplementation::isConnected(Rank source,Rank destination){
	// communicating with itself is always allowed
	if(source==destination)
		return true;

	// check that a connection exists
	return m_connections[source].count(destination)>0;
}

void GraphImplementation::getConnections(Rank source,vector<Rank>*connections){
	for(set<Rank>::iterator i=m_connections[source].begin();
		i!=m_connections[source].end();i++){
		connections->push_back(*i);
	}
}
