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

#include <structures/Kmer.h>
#include <memory/malloc_types.h>
#include <assert.h>
#include <graph/GridTable.h>
#include <core/common_functions.h>
#include <cryptography/crypto.h>
#include <stdlib.h>
#include <stdio.h>

void GridTable::constructor(int rank,Parameters*parameters){
	m_parameters=parameters;
	m_kmerAcademy.constructor(rank,m_parameters);
	m_size=0;
	int initialHashSize=4194304;
	m_hashTable.constructor(initialHashSize,RAY_MALLOC_TYPE_GRID_TABLE,
		m_parameters->showMemoryAllocations(),m_parameters->getRank());

	m_inserted=false;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(rank);
	}
}

uint64_t GridTable::size(){
	return m_size;
}

Vertex*GridTable::find(Kmer*key){
	Kmer lowerKey=complementVertex(key,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	return m_hashTable.find(&lowerKey);
}

KmerCandidate*GridTable::insertInAcademy(Kmer*key){
	return m_kmerAcademy.insert(key);
}

Vertex*GridTable::insert(Kmer*key){
	Kmer lowerKey=complementVertex(key,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	uint64_t sizeBefore=m_hashTable.size();
	Vertex*entry=m_hashTable.insert(&lowerKey);
	m_inserted=m_hashTable.size()>sizeBefore;
	if(m_inserted)
		m_size+=2;
	return entry;
}

bool GridTable::insertedInAcademy(){
	return m_kmerAcademy.inserted();
}

bool GridTable::inserted(){
	return m_inserted;
}

bool GridTable::isAssembled(Kmer*a){
	Kmer reverse=complementVertex(a,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	return getDirections(a).size()>0||getDirections(&reverse).size()>0;
}

KmerAcademy*GridTable::getKmerAcademy(){
	return &m_kmerAcademy;
}

void GridTable::addRead(Kmer*a,ReadAnnotation*e){
	Vertex*i=insert(a);
	i->addRead(a,e);
	#ifdef ASSERT
	ReadAnnotation*reads=i->getReads(a);
	assert(reads!=NULL);
	#endif
}

ReadAnnotation*GridTable::getReads(Kmer*a){
	Vertex*i=find(a);
	if(i==NULL){
		return NULL;
	}
	ReadAnnotation*reads=i->getReads(a);
	return reads;
}

void GridTable::addDirection(Kmer*a,Direction*d){
	Vertex*i=insert(a);
	i->addDirection(a,d);
}

vector<Direction> GridTable::getDirections(Kmer*a){
	Vertex*i=find(a);
	if(i==NULL){
		vector<Direction> p;
		return p;
	}
	return i->getDirections(a);
}

void GridTable::clearDirections(Kmer*a){
	Vertex*i=find(a);
	if(i!=NULL){
		i->clearDirections(a);
	}
}

MyHashTable<Kmer,Vertex>*GridTable::getHashTable(){
	return &m_hashTable;
}

void GridTable::printStatistics(){
	m_hashTable.printStatistics();
}
