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

*/

#ifndef _VerticesExtractor
#define _VerticesExtractor

#include <structures/StaticVector.h>
#include <communication/BufferedData.h>
#include <core/Parameters.h>
#include <core/common_functions.h>
#include <structures/ArrayOfReads.h>
#include <communication/Message.h>
#include <assembler/MemoryConsumptionReducer.h>
#include <structures/MyForest.h>
#include <memory/RingAllocator.h>
#include <structures/Read.h>
#include <set>
#include <vector>
using namespace std;

/*
 * Any MPI rank has some reads to process.
 * VerticesExtractor extracts k-mers from these reads.
 * It also computes arcs between k-mers.
 * These bits are then sent (buffered) to
 * their respective owners.
 */
class VerticesExtractor{
	char m_readSequence[4000];
	bool m_distributionIsCompleted;
	Parameters*m_parameters;

	int m_rank;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	int*m_mode;
	int m_mode_send_vertices_sequence_id_position;

	bool m_deletionsInitiated;
	uint64_t m_deletionIterator;

	bool m_hasPreviousVertex;
	Kmer  m_previousVertex;
	Kmer m_previousVertexRC;

	BufferedData m_bufferedDataForOutgoingEdges;
	BufferedData m_bufferedDataForIngoingEdges;

	int m_pendingMessages;

	BufferedData m_bufferedData;
	set<int> m_ranksThatMustRunReducer;
	int m_size;

	int m_ranksReadyForReduction;
	int m_ranksDoneWithReduction;

	uint64_t m_thresholdForReduction;
	uint64_t m_reductionPeriod;

	bool m_triggered;

	bool m_finished;

	bool m_mustTriggerReduction;


	void checkPendingMessagesForReduction(StaticVector*outbox,int rank);
public:

	BufferedData m_buffersForIngoingEdgesToDelete;
	BufferedData m_buffersForOutgoingEdgesToDelete;


	void constructor(int size,Parameters*parameters);
	void process(int*m_mode_send_vertices_sequence_id,
				ArrayOfReads*m_myReads,
				bool*m_reverseComplementVertex,
				int rank,
				StaticVector*m_outbox,
				bool*m_mode_send_vertices,
				int m_wordSize,
				int size,
				RingAllocator*m_outboxAllocator,
				int*m_mode
			);
	void setReadiness();
	bool mustRunReducer();
	void addRankForReduction(int a);
	void resetRanksForReduction();
	
	void incrementRanksReadyForReduction();
	bool readyForReduction();

	void incrementRanksDoneWithReduction();
	bool reductionIsDone();

	void resetRanksReadyForReduction();
	void resetRanksDoneForReduction();

	uint64_t getThreshold();
	void updateThreshold(GridTable*a);
	bool isTriggered();
	void trigger();
	void removeTrigger();
	bool finished();
	void flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int rank);
	void assertBuffersAreEmpty();
	bool mustTriggerReduction();
	void scheduleReduction(StaticVector*outbox,int rank);

	bool deleteVertices(vector<Kmer>*verticesToRemove,GridTable*subgraph,
Parameters*parameters,RingAllocator*m_outboxAllocator,
	StaticVector*m_outbox,map<Kmer,vector<Kmer> >*ingoingEdges,map<Kmer,vector<Kmer> >*outgoingEdges
);
	void prepareDeletions();

	void incrementPendingMessages();

	void showBuffers();
	void enableReducer();

	bool isDistributionCompleted();
	void setDistributionAsCompleted();
};

#endif

