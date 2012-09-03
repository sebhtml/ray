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

#ifndef _VerticesExtractor
#define _VerticesExtractor

#include <structures/StaticVector.h>
#include <communication/BufferedData.h>
#include <application_core/Parameters.h>
#include <application_core/common_functions.h>
#include <plugin_SequencesLoader/ArrayOfReads.h>
#include <communication/Message.h>
#include <plugin_VerticesExtractor/GridTable.h>
#include <memory/RingAllocator.h>
#include <plugin_SequencesLoader/Read.h>
#include <profiling/Derivative.h>
#include <profiling/Profiler.h>
#include <core/ComputeCore.h>

#include <set>
#include <vector>
using namespace std;


/*
 * Any MPI rank has some reads to process.
 * VerticesExtractor extracts k-mers from these reads.
 * It also computes arcs between k-mers.
 * These bits are then sent (buffered) to
 * their respective owners.
 * \author Sébastien Boisvert
 */
class VerticesExtractor: public CorePlugin{

	MessageTag RAY_MPI_TAG_VERTEX_INFO_REPLY;
	MessageTag RAY_MPI_TAG_BUILD_GRAPH;
	MessageTag RAY_MPI_TAG_WRITE_KMERS;
	MessageTag RAY_MPI_TAG_WRITE_KMERS_REPLY;

	MessageTag RAY_MPI_TAG_IN_EDGES_DATA;
	MessageTag RAY_MPI_TAG_OUT_EDGES_DATA;
	MessageTag RAY_MPI_TAG_VERTICES_DATA;
	MessageTag RAY_MPI_TAG_VERTICES_DISTRIBUTED;
	
	SlaveMode RAY_SLAVE_MODE_ADD_EDGES;
	SlaveMode RAY_SLAVE_MODE_WRITE_KMERS;


	Derivative m_derivative;

	Profiler*m_profiler;

	/** checkpointing */
	bool m_checkedCheckpoint;

	GridTable*m_subgraph;
	char m_readSequence[RAY_MAXIMUM_READ_LENGTH];
	bool m_distributionIsCompleted;
	Parameters*m_parameters;

	int m_rank;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	int*m_mode;
	int m_mode_send_vertices_sequence_id;
	int m_mode_send_vertices_sequence_id_position;

	bool m_hasPreviousVertex;
	Kmer  m_previousVertex;
	Kmer m_previousVertexRC;

	BufferedData m_bufferedDataForOutgoingEdges;
	BufferedData m_bufferedDataForIngoingEdges;

	int m_pendingMessages;

	ArrayOfReads*m_myReads;
	int m_size;
	
/** useless state, legacy code **/
	bool m_reverseComplementVertex;

	bool m_finished;
public:

	void constructor(int size,Parameters*parameters,GridTable*graph,
StaticVector*outbox,RingAllocator*outboxAllocator,ArrayOfReads*myReads
);
	void call_RAY_SLAVE_MODE_ADD_EDGES();
	void setReadiness();
	
	bool finished();
	void flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int rank);
	void assertBuffersAreEmpty();

	void incrementPendingMessages();

	void showBuffers();
	void enableReducer();

	bool isDistributionCompleted();
	void setDistributionAsCompleted();

	void setProfiler(Profiler*profiler);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif

