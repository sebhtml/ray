/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#include<ExtensionData.h>
#include <string.h>

void ExtensionData::constructor(){
	int chunkSize=4194304;
	m_allocator.constructor(chunkSize);
	createStructures();
}

void ExtensionData::createStructures(){
	m_EXTENSION_extension=new vector<uint64_t>;
	m_extensionCoverageValues=new vector<int>;
	m_EXTENSION_coverages=new vector<int>;
	m_EXTENSION_readsInRange=new set<uint64_t>;

	m_database.constructor(&m_allocator);
}

void ExtensionData::destroyStructures(){
	delete m_EXTENSION_extension;
	delete m_extensionCoverageValues;
	delete m_EXTENSION_coverages;
	delete m_EXTENSION_readsInRange;

	m_database.clear();
}

void ExtensionData::resetStructures(){
	destroyStructures();
	createStructures();
	m_allocator.resetMemory();
}

void ExtensionData::destructor(){
	destroyStructures();
	m_allocator.clear();
}

bool ExtensionData::isUsedRead(uint64_t a){
	return m_database.find(a)!=NULL;
}

void ExtensionData::addUsedRead(uint64_t a){
	m_database.insert(a);
}

bool ExtensionData::hasPairedRead(uint64_t a){
	return m_database.find(a)->getValue()->m_hasPairedRead;
}

PairedRead ExtensionData::getPairedRead(uint64_t a){
	return m_database.find(a)->getValue()->m_pairedRead;
}

void ExtensionData::removePairedRead(uint64_t a){
}

void ExtensionData::setPairedRead(uint64_t a,PairedRead b){
	m_database.find(a)->getValue()->m_pairedRead=b;
	m_database.find(a)->getValue()->m_hasPairedRead=true;
}

void ExtensionData::setStrand(uint64_t a,char b){
	m_database.find(a)->getValue()->m_strand=b;
}

void ExtensionData::setStartingPosition(uint64_t a, int b){
	m_database.find(a)->getValue()->m_position=b;
}

void ExtensionData::setSequence(uint64_t a,string b){
	char*seq=(char*)m_allocator.allocate(b.length()*sizeof(char));
	strcpy(seq,b.c_str());
	m_database.find(a)->getValue()->m_readSequence=seq;
	m_database.find(a)->getValue()->m_hasPairedRead=false;
}

int ExtensionData::getStartingPosition(uint64_t a){
	return m_database.find(a)->getValue()->m_position;
}

void ExtensionData::removeSequence(uint64_t a){
}

char*ExtensionData::getSequence(uint64_t a){
	return m_database.find(a)->getValue()->m_readSequence;
}

char ExtensionData::getStrand(uint64_t a){
	return m_database.find(a)->getValue()->m_strand;
}
