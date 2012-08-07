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

#include <core/OperatingSystem.h>
#include <assert.h>
#include <plugin_KmerAcademyBuilder/KmerAcademy.h>
#include <application_core/common_functions.h>
#include <iostream>
#include <cryptography/crypto.h>
#include <stdlib.h>
#include <stdio.h>
#include <application_core/Parameters.h>
using namespace std;

void KmerAcademy::constructor(int rank,Parameters*parameters){
	m_parameters=parameters;
	m_size=0;

	int buckets=m_parameters->getNumberOfBuckets();
	int bucketsPerGroup=m_parameters->getNumberOfBucketsPerGroup();
	double loadFactorThreshold=m_parameters->getLoadFactorThreshold();

	cout<<"[KmerAcademy] buckets="<<buckets<<" bucketsPerGroup="<<bucketsPerGroup;
	cout<<" loadFactorThreshold="<<loadFactorThreshold<<endl;

	m_hashTable.constructor(buckets,"RAY_MALLOC_TYPE_KMER_ACADEMY",
		m_parameters->showMemoryAllocations(),m_parameters->getRank(),
		bucketsPerGroup,loadFactorThreshold
		);

	if(m_parameters->hasOption("-hash-table-verbosity"))
		m_hashTable.toggleVerbosity();

	m_inserted=false;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(rank);
	}
}

LargeCount KmerAcademy::size(){
	return m_size;
}

/**
 * What is this sorcery ?
 * Actually, Ray just stores one Kmer of any pair of reverse-complement
 * Kmer objects.
 */
KmerCandidate*KmerAcademy::find(Kmer*key){
	Kmer lowerKey=key->complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	return m_hashTable.find(&lowerKey);
}

/**
 * Ray just stores one k-mer for any pair of k-mers
 */
KmerCandidate*KmerAcademy::insert(Kmer*key){
	Kmer lowerKey=key->complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	if(key->isLower(&lowerKey)){
		lowerKey=*key;
	}
	LargeCount sizeBefore=m_hashTable.size();
	KmerCandidate*entry=m_hashTable.insert(&lowerKey);
	m_inserted=m_hashTable.size()>sizeBefore;

	/* for any pair of reverse-complement k-mers
 * 	we only insert the lowest so when we do so we virtually insert 2 k-mers
 */
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

void KmerAcademy::completeResizing(){
	m_hashTable.completeResizing();
}
