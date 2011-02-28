/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#ifndef _LibraryWorker
#define _LibraryWorker

#include <map>
#include <stdint.h>
#include <ExtensionData.h>
#include <SeedingData.h>
#include <VirtualCommunicator.h>
#include <RingAllocator.h>
using namespace std;

class LibraryElement{
public:
	int m_readPosition;
	uint16_t m_strandPosition;
	char m_readStrand;
};

class LibraryWorker{
	bool m_done;
	ReadFetcher m_readFetcher;
	map<int,map<int,int> >*m_libraryDistances;
	ExtensionData*m_ed;

	SplayTree<uint64_t,LibraryElement> m_database;

	MyAllocator*m_allocator;
	VirtualCommunicator*m_virtualCommunicator;
	uint64_t m_SEEDING_i;
	RingAllocator*m_outboxAllocator;
	Parameters*m_parameters;
	int m_EXTENSION_currentPosition;
	bool m_EXTENSION_reads_requested;
	SeedingData*m_seedingData;
	int m_EXTENSION_edgeIterator;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	bool m_EXTENSION_hasPairedReadRequested;
	int*m_detectedDistances;
	
public:

	void constructor(uint64_t id,SeedingData*seedingData,VirtualCommunicator*virtualCommunicator,RingAllocator*outboxAllocator,
	Parameters*parameters,StaticVector*inbox,StaticVector*outbox,map<int,map<int,int> >*libraryDistances,int*detectedDistances,
		MyAllocator*allocator);
	bool isDone();
	void work();
};

#endif
