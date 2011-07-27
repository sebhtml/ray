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

#include <assembler/ExtensionData.h>
#include <memory/malloc_types.h>
#include <string.h>
#include <sstream>
#include <cryptography/crypto.h>
using namespace std;

void ExtensionData::constructor(Parameters*parameters){
	m_parameters=parameters;
	m_numberOfBins=1;
	m_database=(SplayTree<uint64_t,ExtensionElement>*)__Malloc(m_numberOfBins*sizeof(SplayTree<uint64_t,ExtensionElement>),
		RAY_MALLOC_TYPE_EXTENSION_DATA_TREES,m_parameters->showMemoryAllocations());
	createStructures();

	ostringstream prefixFull;
	prefixFull<<m_parameters->getMemoryPrefix()<<"_ExtensionData";
	m_allocator.constructor(4194304,RAY_MALLOC_TYPE_EXTENSION_DATA_ALLOCATOR,m_parameters->showMemoryAllocations());
}

void ExtensionData::createStructures(){
	m_EXTENSION_extension=new vector<Kmer>;
	m_repeatedValues=new vector<int>;
	m_extensionCoverageValues=new vector<int>;
	m_EXTENSION_coverages=new vector<int>;
	m_EXTENSION_readsInRange=new set<uint64_t>;
	m_expirations=new map<int,vector<uint64_t> >;
	m_pairedReadsWithoutMate=new set<uint64_t>;
	for(int i=0;i<m_numberOfBins;i++){
		m_database[i].constructor();
	}
}

void ExtensionData::destroyStructures(){
	delete m_EXTENSION_extension;
	delete m_extensionCoverageValues;
	delete m_repeatedValues;
	delete m_EXTENSION_coverages;
	delete m_EXTENSION_readsInRange;
	delete m_pairedReadsWithoutMate;
	delete m_expirations;

	for(int i=0;i<m_numberOfBins;i++){
		m_database[i].clear();
	}
}

void ExtensionData::resetStructures(){
	m_allocator.reset();

	destroyStructures();
	createStructures();
}

void ExtensionData::destructor(){
	destroyStructures();
	__Free(m_database,RAY_MALLOC_TYPE_EXTENSION_DATA_TREES,m_parameters->showMemoryAllocations());
}

ExtensionElement*ExtensionData::getUsedRead(uint64_t a){
	int bin=0;//uniform_hashing_function_1_64_64(a)%m_numberOfBins;
	SplayNode<uint64_t,ExtensionElement>*node=m_database[bin].find(a,true);
	if(node!=NULL){
		return node->getValue();
	}
	return NULL;
}

ExtensionElement*ExtensionData::addUsedRead(uint64_t a){
	bool val;
	int bin=0;
	ExtensionElement*element=m_database[bin].insert(a,&m_allocator,&val)->getValue();
	element->constructor();
	return element;
}

void ExtensionData::removeSequence(uint64_t a){
	int bin=0;
	m_database[bin].remove(a,false,&m_allocator);
}

MyAllocator*ExtensionData::getAllocator(){
	return &m_allocator;
}


