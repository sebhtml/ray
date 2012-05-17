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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <structures/SplayTreeIterator.h>
#include <plugin_SeedExtender/ExtensionData.h>
#include <string.h>
#include <sstream>
#include <cryptography/crypto.h>
using namespace std;

void ExtensionData::constructor(Parameters*parameters){
	m_parameters=parameters;
	m_numberOfBins=1;
	m_database=(SplayTree<ReadHandle,ExtensionElement>*)__Malloc(m_numberOfBins*sizeof(SplayTree<ReadHandle,ExtensionElement>),
		"RAY_MALLOC_TYPE_EXTENSION_DATA_TREES",m_parameters->showMemoryAllocations());
	createStructures();

	ostringstream prefixFull;
	prefixFull<<m_parameters->getMemoryPrefix()<<"_ExtensionData";
	m_allocator.constructor(4194304,"RAY_MALLOC_TYPE_EXTENSION_DATA_ALLOCATOR",m_parameters->showMemoryAllocations());
}

void ExtensionData::createStructures(){
	m_EXTENSION_extension.clear();
	m_repeatedValues.clear();
	m_extensionCoverageValues.clear();
	m_EXTENSION_coverages.clear();
	m_EXTENSION_readsInRange.clear();
	m_expirations.clear();
	m_pairedReadsWithoutMate.clear();

	for(int i=0;i<m_numberOfBins;i++){
		m_database[i].constructor();
	}
}

void ExtensionData::destroyStructures(Profiler*m_profiler){

	MACRO_COLLECT_PROFILING_INFORMATION();

	m_EXTENSION_extension.clear();
	m_extensionCoverageValues.clear();
	m_repeatedValues.clear();

	MACRO_COLLECT_PROFILING_INFORMATION();

	m_EXTENSION_coverages.clear();
	m_EXTENSION_readsInRange.clear();

	MACRO_COLLECT_PROFILING_INFORMATION();

	//cout<<"m_pairedReadsWithoutMate= "<<m_pairedReadsWithoutMate.size()<<endl;
	m_pairedReadsWithoutMate.clear();

	MACRO_COLLECT_PROFILING_INFORMATION();

	//cout<<"m_expirations.size= "<<m_expirations.size()<<endl;

	m_expirations.clear();

	MACRO_COLLECT_PROFILING_INFORMATION();

	for(int i=0;i<m_numberOfBins;i++){
		m_database[i].clear();
	}

	MACRO_COLLECT_PROFILING_INFORMATION();
}

void ExtensionData::resetStructures(Profiler*m_profiler){
	MACRO_COLLECT_PROFILING_INFORMATION();

	m_allocator.reset();

	MACRO_COLLECT_PROFILING_INFORMATION();

	destroyStructures(m_profiler);

	MACRO_COLLECT_PROFILING_INFORMATION();
}

void ExtensionData::destructor(){
	destroyStructures(NULL);
	__Free(m_database,"RAY_MALLOC_TYPE_EXTENSION_DATA_TREES",m_parameters->showMemoryAllocations());
}

ExtensionElement*ExtensionData::getUsedRead(ReadHandle a){
	int bin=0;//uniform_hashing_function_1_64_64(a)%m_numberOfBins;
	SplayNode<ReadHandle,ExtensionElement>*node=m_database[bin].find(a,true);
	if(node!=NULL && node->getValue()->m_activated){
		return node->getValue();
	}
	return NULL;
}

ExtensionElement*ExtensionData::addUsedRead(ReadHandle a){
	bool val;
	int bin=0;
	ExtensionElement*element=m_database[bin].insert(a,&m_allocator,&val)->getValue();
	element->constructor();
	element->m_activated=true;
	return element;
}

void ExtensionData::removeSequence(ReadHandle a){
	int bin=0;
	m_database[bin].remove(a,false,&m_allocator);
}

MyAllocator*ExtensionData::getAllocator(){
	return &m_allocator;
}

void ExtensionData::lazyDestructor(){
	SplayTreeIterator<ReadHandle,ExtensionElement> i;
	i.constructor(&(m_database[0]));
	while(i.hasNext()){
		ExtensionElement*element=i.next()->getValue();
		element->m_activated=false;
	}
}
