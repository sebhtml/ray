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

#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <assert.h>
#include <plugin_VerticesExtractor/GridTable.h>
#include <core/OperatingSystem.h>
#include <application_core/common_functions.h>
#include <cryptography/crypto.h>
#include <stdlib.h>
#include <stdio.h>

void GridTable::constructor(int rank,Parameters*parameters){
	m_parameters=parameters;
	m_size=0;

	showMemoryUsage(rank);

	uint64_t buckets=m_parameters->getNumberOfBuckets();
	int bucketsPerGroup=m_parameters->getNumberOfBucketsPerGroup();
	double loadFactorThreshold=m_parameters->getLoadFactorThreshold();

	cout<<"[GridTable] buckets="<<buckets<<" bucketsPerGroup="<<bucketsPerGroup;
	cout<<" loadFactorThreshold="<<loadFactorThreshold<<endl;

	m_hashTable.constructor(buckets,"RAY_MALLOC_TYPE_GRID_TABLE",
		m_parameters->showMemoryAllocations(),m_parameters->getRank(),
		bucketsPerGroup,loadFactorThreshold
		);

	if(m_parameters->hasOption("-hash-table-verbosity"))
		m_hashTable.toggleVerbosity();

	m_inserted=false;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(rank);
	}

	m_findOperations=0;

	m_verbose=false;
}

LargeCount GridTable::size(){
	return m_size;
}

Vertex*GridTable::find(Kmer*key){
	#ifdef ASSERT
	assert(key!=NULL);
	#endif

	Kmer lowerKey=key->complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}

	m_findOperations++;

	// show some love on screen
	if(m_verbose && m_findOperations%100000==0){
		m_hashTable.toggleVerbosity();
	}

	Vertex*vertex= m_hashTable.find(&lowerKey);

	// turns off verbosity
	if(m_verbose && m_findOperations%100000==0){
		m_hashTable.toggleVerbosity();
	}

	return vertex;
}

Vertex*GridTable::insert(Kmer*key){
	#ifdef ASSERT
	assert(key!=NULL);
	#endif

	Kmer lowerKey=key->complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}

	LargeCount sizeBefore=m_hashTable.size();
	Vertex*entry=m_hashTable.insert(&lowerKey);
	m_inserted=m_hashTable.size()>sizeBefore;

	if(m_inserted){
		m_size+=2;
	}

	return entry;
}

bool GridTable::inserted(){
	return m_inserted;
}

bool GridTable::isAssembledByGreaterRank(Kmer*a,Rank origin){
	#ifdef ASSERT
	assert(a!=NULL);
	#endif

	Vertex*entry=find(a);
	
	#ifdef ASSERT
	assert(entry!=NULL);
	#endif

	return entry->isAssembledByGreaterRank(origin);
}

bool GridTable::isAssembled(Kmer*a){
	#ifdef ASSERT
	assert(a!=NULL);
	#endif

	Vertex*entry=find(a);
	
	#ifdef ASSERT
	assert(entry!=NULL);
	#endif

	return entry->isAssembled();
}

void GridTable::addRead(Kmer*a,ReadAnnotation*e){

	#ifdef ASSERT
	assert(a!=NULL);
	assert(e!=NULL);
	#endif

	Vertex*i=find(a);
	i->addRead(a,e);

	#ifdef ASSERT
	ReadAnnotation*reads=i->getReads(a);
	assert(reads!=NULL);
	#endif
}

ReadAnnotation*GridTable::getReads(Kmer*a){

	#ifdef ASSERT
	assert(a!=NULL);
	#endif

	Vertex*i=find(a);
	if(i==NULL){
		return NULL;
	}

	ReadAnnotation*reads=i->getReads(a);
	return reads;
}

void GridTable::addDirection(Kmer*a,Direction*d){
	#ifdef ASSERT
	assert(a!=NULL);
	assert(d!=NULL);

	// the commented code was used when non-NULL pointers were invalid
	// according to /proc/self/maps
	//
	//Kmer copy1=*a;
	//Direction copy2=*d;
	#endif

	Vertex*i=find(a);

	#ifdef ASSERT
	assert(i!=NULL);
	#endif

	i->addDirection(a,d);
}

vector<Direction> GridTable::getDirections(Kmer*a){
	#ifdef ASSERT
	assert(a!=NULL);
	#endif

	Vertex*i=find(a);

	if(i==NULL){
		vector<Direction> p;
		return p;
	}

	#ifdef ASSERT
	assert(i!=NULL);

	/* do a copy to track to check for a segmentation fault */
	Vertex copy=*i;
	assert(copy.m_coverage_lower >= 1);
	#endif

	return i->getDirections(a);
}

void GridTable::clearDirections(Kmer*a){
	#ifdef ASSERT
	assert(a!=NULL);
	#endif

	Vertex*i=find(a);

	#ifdef ASSERT
	assert(i!=NULL);
	#endif
	
	i->clearDirections(a);
}

MyHashTable<Kmer,Vertex>*GridTable::getHashTable(){
	return &m_hashTable;
}

void GridTable::printStatistics(){
	m_hashTable.printProbeStatistics();
}

void GridTable::completeResizing(){
	m_hashTable.completeResizing();
}
