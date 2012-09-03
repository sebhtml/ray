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

#ifndef _KmerAcademyBuilder
#define _KmerAcademyBuilder

#include <structures/StaticVector.h>
#include <communication/BufferedData.h>
#include <plugin_VerticesExtractor/GridTable.h>
#include <profiling/Derivative.h>
#include <application_core/Parameters.h>
#include <application_core/common_functions.h>
#include <plugin_SequencesLoader/ArrayOfReads.h>
#include <communication/Message.h>
#include <profiling/Profiler.h>
#include <memory/RingAllocator.h>
#include <plugin_SequencesLoader/Read.h>
#include <core/ComputeCore.h>

#include <set>
#include <vector>
using namespace std;


/*
 * Any MPI rank has some reads to process.
 * KmerAcademyBuilder extracts k-mers from these reads.
 * It also computes arcs between k-mers.
 * These bits are then sent (buffered) to
 * their respective owners.
 * \author Sébastien Boisvert
 */
class KmerAcademyBuilder : public CorePlugin{

	MessageTag RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED;
	MessageTag RAY_MPI_TAG_VERTICES_DATA_REPLY;
	MessageTag RAY_MPI_TAG_VERTICES_DATA;

	SlaveMode RAY_SLAVE_MODE_ADD_VERTICES;

	Profiler*m_profiler;

	Derivative m_derivative;

	/** this we check the checkpoint ? */
	bool m_checkedCheckpoint;

	char m_readSequence[RAY_MAXIMUM_READ_LENGTH];
	bool m_distributionIsCompleted;
	Parameters*m_parameters;

	bool m_initialised;

	int m_rank;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	int*m_mode;

/** this attribute is not used, it is an artefact. **/
	bool m_reverseComplementVertex;

	int m_mode_send_vertices_sequence_id;
	int m_mode_send_vertices_sequence_id_position;

	int m_pendingMessages;

	ArrayOfReads*m_myReads;
	BufferedData m_bufferedData;
	int m_size;

	bool m_finished;
	GridTable*m_subgraph;
public:

	BufferedData m_buffersForIngoingEdgesToDelete;
	BufferedData m_buffersForOutgoingEdgesToDelete;

	void constructor(int size,Parameters*parameters,GridTable*graph,
		ArrayOfReads*myReads,StaticVector*inbox,StaticVector*outbox,
SlaveMode*mode,RingAllocator*outboxAllocator
);

	void setProfiler(Profiler*profiler);

	void call_RAY_SLAVE_MODE_ADD_VERTICES();

	void setReadiness();

	bool finished();
	void flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int rank);
	void assertBuffersAreEmpty();

	void incrementPendingMessages();

	void showBuffers();
	bool isDistributionCompleted();
	void setDistributionAsCompleted();

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif

