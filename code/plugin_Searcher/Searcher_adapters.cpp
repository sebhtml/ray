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

#include <plugin_Searcher/Searcher_adapters.h>
#include <plugin_Searcher/Searcher.h>

void Adapter_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS::call(){
	m_object->call_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS();
}

void Adapter_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES::call(){
	m_object->call_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES();
}

void Adapter_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES::call(){
	m_object->call_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES();
}

void Adapter_RAY_SLAVE_MODE_ADD_COLORS::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_ADD_COLORS::call(){
	m_object->call_RAY_SLAVE_MODE_ADD_COLORS();
}

void Adapter_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS::call(){
	m_object->call_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS();
}

void Adapter_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES::call(){
	m_object->call_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES();
}

void Adapter_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES::call(){
	m_object->call_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES();
}

void Adapter_RAY_MASTER_MODE_ADD_COLORS::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_ADD_COLORS::call(){
	m_object->call_RAY_MASTER_MODE_ADD_COLORS();
}

void Adapter_RAY_MPI_TAG_ADD_KMER_COLOR::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ADD_KMER_COLOR::call(Message*message){
	m_object->call_RAY_MPI_TAG_ADD_KMER_COLOR(message);
}

void Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS::setObject(Searcher*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS(message);
}


