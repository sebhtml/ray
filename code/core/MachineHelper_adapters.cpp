/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

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

#include <core/MachineHelper_adapters.h>
#include <core/MachineHelper.h>

void Adapter_RAY_MASTER_MODE_LOAD_CONFIG::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_LOAD_CONFIG::call(){
	m_object->call_RAY_MASTER_MODE_LOAD_CONFIG();
}

void Adapter_RAY_MASTER_MODE_SEND_COVERAGE_VALUES::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_SEND_COVERAGE_VALUES::call(){
	m_object->call_RAY_MASTER_MODE_SEND_COVERAGE_VALUES();
}

void Adapter_RAY_MASTER_MODE_WRITE_KMERS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_WRITE_KMERS::call(){
	m_object->call_RAY_MASTER_MODE_WRITE_KMERS();
}

void Adapter_RAY_MASTER_MODE_LOAD_SEQUENCES::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_LOAD_SEQUENCES::call(){
	m_object->call_RAY_MASTER_MODE_LOAD_SEQUENCES();
}

void Adapter_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION::call(){
	m_object->call_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION();
}

void Adapter_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING::call(){
	m_object->call_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING();
}

void Adapter_RAY_MASTER_MODE_PURGE_NULL_EDGES::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_PURGE_NULL_EDGES::call(){
	m_object->call_RAY_MASTER_MODE_PURGE_NULL_EDGES();
}

void Adapter_RAY_MASTER_MODE_TRIGGER_INDEXING::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_TRIGGER_INDEXING::call(){
	m_object->call_RAY_MASTER_MODE_TRIGGER_INDEXING();
}

void Adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS::call(){
	m_object->call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS();
}

void Adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS::call(){
	m_object->call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS();
}

void Adapter_RAY_MASTER_MODE_PREPARE_SEEDING::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_PREPARE_SEEDING::call(){
	m_object->call_RAY_MASTER_MODE_PREPARE_SEEDING();
}

void Adapter_RAY_MASTER_MODE_TRIGGER_SEEDING::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_TRIGGER_SEEDING::call(){
	m_object->call_RAY_MASTER_MODE_TRIGGER_SEEDING();
}

void Adapter_RAY_MASTER_MODE_TRIGGER_DETECTION::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_TRIGGER_DETECTION::call(){
	m_object->call_RAY_MASTER_MODE_TRIGGER_DETECTION();
}

void Adapter_RAY_MASTER_MODE_ASK_DISTANCES::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_ASK_DISTANCES::call(){
	m_object->call_RAY_MASTER_MODE_ASK_DISTANCES();
}

void Adapter_RAY_MASTER_MODE_START_UPDATING_DISTANCES::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_START_UPDATING_DISTANCES::call(){
	m_object->call_RAY_MASTER_MODE_START_UPDATING_DISTANCES();
}

void Adapter_RAY_MASTER_MODE_TRIGGER_EXTENSIONS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_TRIGGER_EXTENSIONS::call(){
	m_object->call_RAY_MASTER_MODE_TRIGGER_EXTENSIONS();
}

void Adapter_RAY_MASTER_MODE_TRIGGER_FUSIONS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_TRIGGER_FUSIONS::call(){
	m_object->call_RAY_MASTER_MODE_TRIGGER_FUSIONS();
}

void Adapter_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS::call(){
	m_object->call_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS();
}

void Adapter_RAY_MASTER_MODE_START_FUSION_CYCLE::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_START_FUSION_CYCLE::call(){
	m_object->call_RAY_MASTER_MODE_START_FUSION_CYCLE();
}

void Adapter_RAY_MASTER_MODE_ASK_EXTENSIONS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_ASK_EXTENSIONS::call(){
	m_object->call_RAY_MASTER_MODE_ASK_EXTENSIONS();
}

void Adapter_RAY_MASTER_MODE_SCAFFOLDER::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_SCAFFOLDER::call(){
	m_object->call_RAY_MASTER_MODE_SCAFFOLDER();
}

void Adapter_RAY_MASTER_MODE_KILL_RANKS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_KILL_RANKS::call(){
	m_object->call_RAY_MASTER_MODE_KILL_RANKS();
}

void Adapter_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS::call(){
	m_object->call_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS();
}

void Adapter_RAY_SLAVE_MODE_WRITE_KMERS::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_WRITE_KMERS::call(){
	m_object->call_RAY_SLAVE_MODE_WRITE_KMERS();
}

void Adapter_RAY_SLAVE_MODE_ASSEMBLE_WAVES::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_ASSEMBLE_WAVES::call(){
	m_object->call_RAY_SLAVE_MODE_ASSEMBLE_WAVES();
}

void Adapter_RAY_SLAVE_MODE_SEND_EXTENSION_DATA::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_SEND_EXTENSION_DATA::call(){
	m_object->call_RAY_SLAVE_MODE_SEND_EXTENSION_DATA();
}

void Adapter_RAY_SLAVE_MODE_DIE::setObject(MachineHelper*object){
	m_object=object;
}

void Adapter_RAY_SLAVE_MODE_DIE::call(){
	m_object->call_RAY_SLAVE_MODE_DIE();
}


