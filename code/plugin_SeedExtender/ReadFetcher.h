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

#ifndef _ReadFetcher
#define _ReadFetcher

#include <vector>
#include <plugin_SequencesIndexer/ReadAnnotation.h>
#include <structures/StaticVector.h>
#include <application_core/Parameters.h>
#include <stdint.h>
#include <memory/RingAllocator.h>
#include <communication/VirtualCommunicator.h>
using namespace std;

/**
 * A class that fetches reads for a Kmer
 * Pretty simple stuff.
 *
 * \author Sébastien Boisvert
 */
class ReadFetcher{
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS;

	WorkerHandle m_workerId;
	VirtualCommunicator*m_virtualCommunicator;
	Parameters*m_parameters;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	vector<ReadAnnotation> m_reads;
	Kmer m_vertex;
	bool m_readsRequested;
	void*m_pointer;
	bool m_done;
public:
	/**
 *	Initiate the object with a Kmer
 */
	void constructor(Kmer*vertex,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,VirtualCommunicator*vc,WorkerHandle workerId,
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS

);

/**
 * Returns true if done
 */
	bool isDone();
/**
 * Advance the work to be done
 */
	void work();

/**
 * Get the requested information.
 */
	vector<ReadAnnotation>*getResult();
};

#endif
