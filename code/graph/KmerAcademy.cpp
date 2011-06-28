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

#include <memory/malloc_types.h>
#include <assert.h>
#include <graph/KmerAcademy.h>
#include <core/common_functions.h>
#include <iostream>
#include <cryptography/crypto.h>
#include <stdlib.h>
#include <stdio.h>
#include <core/Parameters.h>
using namespace std;

void KmerAcademy::constructor(int rank,Parameters*parameters){
	m_parameters=parameters;
	m_size=0;
	m_hashTable.constructor(RAY_MALLOC_TYPE_KMER_ACADEMY,
		m_parameters->showMemoryAllocations(),m_parameters->getRank());

	m_inserted=false;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(rank);
	}
}

uint64_t KmerAcademy::size(){
	return m_size;
}

/**
 * What is this sorcery ?
 * Actually, Ray just stores one Kmer of any pair of reverse-complement
 * Kmer objects.
 */
KmerCandidate*KmerAcademy::find(Kmer*key){
	Kmer lowerKey=complementVertex(key,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	return m_hashTable.find(&lowerKey);
}

KmerCandidate*KmerAcademy::insert(Kmer*key){
	Kmer lowerKey=complementVertex(key,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	uint64_t sizeBefore=m_hashTable.size();
	KmerCandidate*entry=m_hashTable.insert(key);
	m_inserted=m_hashTable.size()>sizeBefore;
	if(m_inserted)
		m_size+=2;
	return entry;
}

bool KmerAcademy::inserted(){
	return m_inserted;
}

void KmerAcademy::destructor(){
	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
	}

	m_hashTable.destructor();

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
	}
}

MyHashTable<Kmer,KmerCandidate>*KmerAcademy::getHashTable(){
	return &m_hashTable;
}

/** print statistics of the hash table */
void KmerAcademy::printStatistics(){
	m_hashTable.printStatistics();
}
