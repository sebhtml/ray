/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood_adapters.h>
#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood.h>

void Adapter_RAY_SLAVE_MODE_NEIGHBOURHOOD::setObject(GenomeNeighbourhood*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_NEIGHBOURHOOD::call(){
	m_object->call_RAY_SLAVE_MODE_NEIGHBOURHOOD();
}

void Adapter_RAY_MASTER_MODE_NEIGHBOURHOOD::setObject(GenomeNeighbourhood*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_NEIGHBOURHOOD::call(){
	m_object->call_RAY_MASTER_MODE_NEIGHBOURHOOD();
}

void Adapter_RAY_MPI_TAG_NEIGHBOURHOOD_DATA::setObject(GenomeNeighbourhood*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_NEIGHBOURHOOD_DATA::call(Message*message){
	m_object->call_RAY_MPI_TAG_NEIGHBOURHOOD_DATA(message);
}



#undef CLASS_FOR_ADAPTERS


