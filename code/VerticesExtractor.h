/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<StaticVector.h>
#include<BufferedData.h>
#include<vector>
#include<Parameters.h>
#include<common_functions.h>
#include<ArrayOfReads.h>
#include<Message.h>
#include<MyForest.h>
#include<RingAllocator.h>
#include<set>
#include<Read.h>
using namespace std;

class VerticesExtractor{
	bool m_deletionsInitiated;
	uint64_t m_deletionIterator;

	bool m_hasPreviousVertex;
	uint64_t m_previousVertex;
	uint64_t m_previousVertexRC;

	BufferedData m_bufferedDataForOutgoingEdges;
	BufferedData m_bufferedDataForIngoingEdges;

	int m_pendingMessages;

	BufferedData m_bufferedData;
	set<int> m_ranksThatMustRunReducer;
	int m_size;

	int m_ranksReadyForReduction;
	int m_ranksDoneWithReduction;

	uint64_t m_thresholdForReduction;
	int m_reductionPeriod;

	bool m_triggered;

	bool m_finished;

	bool m_mustTriggerReduction;

public:

	BufferedData m_buffersForIngoingEdgesToDelete;
	BufferedData m_buffersForOutgoingEdgesToDelete;


	void constructor(int size);
	void process(int*m_mode_send_vertices_sequence_id,
				ArrayOfReads*m_myReads,
				bool*m_reverseComplementVertex,
				int*m_mode_send_vertices_sequence_id_position,
				int rank,
				StaticVector*m_outbox,
				bool*m_mode_send_vertices,
				int m_wordSize,
				int size,
				RingAllocator*m_outboxAllocator,
				bool m_colorSpaceMode,int*m_mode
			);
	void setReadiness(StaticVector*outbox,int rank);
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
	void updateThreshold(MyForest*a);
	bool isTriggered();
	void trigger();
	void removeTrigger();
	bool finished();
	void flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int rank);
	void assertBuffersAreEmpty();
	bool mustTriggerReduction();
	void scheduleReduction();

	bool deleteVertices(vector<uint64_t>*verticesToRemove,MyForest*subgraph,
Parameters*parameters,RingAllocator*m_outboxAllocator,
	StaticVector*m_outbox
);
	void prepareDeletions();

	void flushBuffers(int rank,StaticVector*m_outbox,RingAllocator*m_outboxAllocator);
	void incrementPendingMessages();

	void showBuffers();
};

#endif

