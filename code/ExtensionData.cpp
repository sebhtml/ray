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

#include <ExtensionData.h>
#include <string.h>
#include <crypto.h>

void ExtensionData::constructor(){
	m_numberOfBins=4096;
	m_database=(SplayTree<uint64_t,ExtensionElement>*)__Malloc(m_numberOfBins*sizeof(SplayTree<uint64_t,ExtensionElement>));
	createStructures();
	int chunkSize=4194304;
	m_allocator.constructor(chunkSize);
}

void ExtensionData::createStructures(){
	m_EXTENSION_extension=new vector<uint64_t>;
	m_repeatedValues=new vector<int>;
	m_extensionCoverageValues=new vector<int>;
	m_EXTENSION_coverages=new vector<int>;
	m_EXTENSION_readsInRange=new set<uint64_t>;
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

	for(int i=0;i<m_numberOfBins;i++){
		m_database[i].clear();
	}
}

void ExtensionData::resetStructures(){
	m_allocator.reset();
	//m_allocator.clear();
	//int chunkSize=4194304;
	//m_allocator.constructor(chunkSize);

	destroyStructures();
	createStructures();
}

void ExtensionData::destructor(){
	destroyStructures();
	__Free(m_database);
}

ExtensionElement*ExtensionData::getUsedRead(uint64_t a){
	SplayNode<uint64_t,ExtensionElement>*node=m_database[uniform_hashing_function_1_64_64(a)%m_numberOfBins].find(a,true);
	if(node!=NULL){
		return node->getValue();
	}
	return NULL;
}

ExtensionElement*ExtensionData::addUsedRead(uint64_t a){
	bool val;
	return m_database[uniform_hashing_function_1_64_64(a)%m_numberOfBins].insert(a,&m_allocator,&val)->getValue();
}

void ExtensionData::removeSequence(uint64_t a){
	m_database[uniform_hashing_function_1_64_64(a)%m_numberOfBins].remove(a,false,&m_allocator);
}

MyAllocator*ExtensionData::getAllocator(){
	return &m_allocator;
}

