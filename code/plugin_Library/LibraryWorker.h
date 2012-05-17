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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _LibraryWorker
#define _LibraryWorker

#include <plugin_SeedingData/SeedingData.h>
#include <plugin_SeedExtender/ExtensionData.h>
#include <communication/VirtualCommunicator.h>
#include <memory/RingAllocator.h>
#include <scheduling/Worker.h>
#include <map>
#include <stdint.h>
using namespace std;

class LibraryElement{
public:
	int m_readPosition;
	uint16_t m_strandPosition;
	char m_readStrand;
} ATTRIBUTE_PACKED;

/*
 * Computes average outer distances 
 * \author Sébastien Boisvert
 */
class LibraryWorker : public Worker {

	MessageTag RAY_MPI_TAG_GET_READ_MATE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS;

	bool m_done;
	ReadFetcher m_readFetcher;
	map<int,map<int,int> >*m_libraryDistances;
	ExtensionData*m_ed;

	SplayTree<ReadHandle,LibraryElement> m_database;

	MyAllocator*m_allocator;
	VirtualCommunicator*m_virtualCommunicator;
	PathHandle m_SEEDING_i;
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

	void constructor(WorkerHandle id,SeedingData*seedingData,VirtualCommunicator*virtualCommunicator,RingAllocator*outboxAllocator,
	Parameters*parameters,StaticVector*inbox,StaticVector*outbox,map<int,map<int,int> >*libraryDistances,int*detectedDistances,
		MyAllocator*allocator,
MessageTag RAY_MPI_TAG_GET_READ_MATE,
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS
);

	/** work a little bit 
	 * the class Worker provides no implementation for that 
	*/
	void work();

	/** is the worker done doing its things */
	bool isDone();

	/** get the worker number */
	WorkerHandle getWorkerIdentifier();

};

#endif
