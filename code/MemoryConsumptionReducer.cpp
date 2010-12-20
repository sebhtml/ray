/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<MemoryConsumptionReducer.h>

MemoryConsumptionReducer::MemoryConsumptionReducer(){
	m_initiated=false;
}

bool MemoryConsumptionReducer::reduce(MyForest*a){
	if(!m_initiated){
		m_iterator.constructor(a);
		m_initiated=true;
		m_removedVertices=0;
		return false;
	}else if(m_iterator.hasNext()){
		m_iterator.next();
		return false;
	}else{
		m_initiated=false;
		return true;
	}
}

int MemoryConsumptionReducer::getNumberOfRemovedVertices(){
	return m_removedVertices;
}
