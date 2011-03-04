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

#ifndef _MessageProcessor
#define _MessageProcessor

#include<RingAllocator.h>
#include<Library.h>
#include<SequencesIndexer.h>
#include<SeedingData.h>
#include<OpenAssemblerChooser.h>
#include<ArrayOfReads.h>
#include<Message.h>
#include<vector>
#include<SplayTree.h>
#include<StaticVector.h>
#include<SeedExtender.h>
#include<MessagesHandler.h>
#include<SequencesLoader.h>
#include<FusionData.h>
#include<ReadAnnotation.h>
#include<VerticesExtractor.h>
#include<MyForest.h>
#include <GridTable.h>
#include<Parameters.h>
#include<MemoryConsumptionReducer.h>
#include<BufferedData.h>
#include<MyAllocator.h>
#include<Vertex.h>
using namespace std;


class MessageProcessor;
typedef void (MessageProcessor::*FNMETHOD) (Message*message);

class MessageProcessor{
	int m_count;

	MemoryConsumptionReducer*m_reducer;
	uint64_t m_lastSize;

	SequencesLoader*m_sequencesLoader;

	FNMETHOD m_methods[256];

	uint64_t m_sentinelValue;

	SeedingData*m_seedingData;

	// data for processing
	bool*m_ready;
	int m_consumed;
	time_t m_last;

	Library*m_library;
	bool*m_isFinalFusion;
	int*m_master_mode;
	ExtensionData*m_ed;
	int*m_numberOfRanksDoneDetectingDistances;
	int*m_numberOfRanksDoneSendingDistances;
	Parameters*parameters;

	MessagesHandler*m_messagesHandler;

	//MyForest*m_subgraph;
	GridTable*m_subgraph;

	RingAllocator*m_outboxAllocator;
	int rank;
	int*m_numberOfMachinesDoneSendingEdges;
	FusionData*m_fusionData;
	int*m_wordSize;
	int*m_minimumCoverage;
	int*m_seedCoverage;
	int*m_peakCoverage;
	ArrayOfReads*m_myReads;
	int size;
	SequencesIndexer*m_si;

	RingAllocator*m_inboxAllocator;
	MyAllocator*m_persistentAllocator;
	vector<uint64_t>*m_identifiers;
	bool*m_mode_sendDistribution;
	bool*m_alive;
	bool*m_colorSpaceMode;
	int*m_mode;
	vector<vector<uint64_t> >*m_allPaths;
	int*m_last_value;
	int*m_ranksDoneAttachingReads;
	int*m_DISTRIBUTE_n;
	int*m_numberOfRanksDoneSeeding;
	int*m_CLEAR_n;
	int*m_readyToSeed;
	int*m_FINISH_n;
	bool*m_nextReductionOccured;
	MyAllocator*m_directionsAllocator;
	int*m_mode_send_coverage_iterator;
	map<int,uint64_t>*m_coverageDistribution;
	int*m_sequence_ready_machines;
	int*m_numberOfMachinesReadyForEdgesDistribution;
	int*m_numberOfMachinesReadyToSendDistribution;
	bool*m_mode_send_outgoing_edges;
	int*m_mode_send_vertices_sequence_id;
	bool*m_mode_send_vertices;
	int*m_numberOfMachinesDoneSendingVertices;
	VerticesExtractor*m_verticesExtractor;
	int*m_numberOfMachinesDoneSendingCoverage;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	map<int,int>*m_allIdentifiers;
	OpenAssemblerChooser*m_oa;
	int*m_numberOfRanksWithCoverageData;
	SeedExtender*seedExtender;

	void assignHandlers();

	void call_RAY_MPI_TAG_WELCOME(Message*message);
	void call_RAY_MPI_TAG_SEQUENCES_READY(Message*message);
	void call_RAY_MPI_TAG_VERTICES_DATA(Message*message);
	void call_RAY_MPI_TAG_VERTICES_DISTRIBUTED(Message*message);
	void call_RAY_MPI_TAG_OUT_EDGES_DATA(Message*message);
	void call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION(Message*message);
	void call_RAY_MPI_TAG_EDGES_DISTRIBUTED(Message*message);
	void call_RAY_MPI_TAG_IN_EDGES_DATA(Message*message);
	void call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION(Message*message);
	void call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ASK(Message*message);
	void call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ANSWER(Message*message);
	void call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message);
	void call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER(Message*message);
	void call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION(Message*message);
	void call_RAY_MPI_TAG_COVERAGE_DATA(Message*message);
	void call_RAY_MPI_TAG_COVERAGE_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_COVERAGE_END(Message*message);
	void call_RAY_MPI_TAG_SEND_COVERAGE_VALUES(Message*message);
	void call_RAY_MPI_TAG_READY_TO_SEED(Message*message);
	void call_RAY_MPI_TAG_START_SEEDING(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_SEEDING_IS_OVER(Message*message);
	void call_RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON(Message*message);
	void call_RAY_MPI_TAG_I_GO_NOW(Message*message);
	void call_RAY_MPI_TAG_SET_WORD_SIZE(Message*message);
	void call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS(Message*message);
	void call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_IS_DONE(Message*message);
	void call_RAY_MPI_TAG_ASK_EXTENSION(Message*message);
	void call_RAY_MPI_TAG_ASK_IS_ASSEMBLED(Message*message);
	void call_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY(Message*message);
	void call_RAY_MPI_TAG_MARK_AS_ASSEMBLED(Message*message);
	void call_RAY_MPI_TAG_ASK_EXTENSION_DATA(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_DATA(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_END(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_DATA_END(Message*message);
	void call_RAY_MPI_TAG_ATTACH_SEQUENCE(Message*message);
	void call_RAY_MPI_TAG_REQUEST_READS(Message*message);
	void call_RAY_MPI_TAG_REQUEST_READS_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION(Message*message);
	void call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_READ_LENGTH(Message*message);
	void call_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY(Message*message);
	void call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION(Message*message);
	void call_RAY_MPI_TAG_ASSEMBLE_WAVES(Message*message);
	void call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REVERSE(Message*message);
	void call_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE(Message*message);
	void call_RAY_MPI_TAG_START_FUSION(Message*message);
	void call_RAY_MPI_TAG_FUSION_DONE(Message*message);
	void call_RAY_MPI_TAG_WRITE_AMOS(Message*message);
	void call_RAY_MPI_TAG_WRITE_AMOS_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY(Message*message);
	void call_RAY_MPI_TAG_GET_PATH_LENGTH(Message*message);
	void call_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATH(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY(Message*message);
	void call_RAY_MPI_TAG_HAS_PAIRED_READ(Message*message);
	void call_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY(Message*message);
	void call_RAY_MPI_TAG_GET_PAIRED_READ(Message*message);
	void call_RAY_MPI_TAG_GET_PAIRED_READ_REPLY(Message*message);
	void call_RAY_MPI_TAG_CLEAR_DIRECTIONS(Message*message);
	void call_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY(Message*message);
	void call_RAY_MPI_TAG_FINISH_FUSIONS(Message*message);
	void call_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED(Message*message);
	void call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS(Message*message);
	void call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_START(Message*message);
	void call_RAY_MPI_TAG_ELIMINATE_PATH(Message*message);
	void call_RAY_MPI_TAG_GET_PATH_VERTEX(Message*message);
	void call_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY(Message*message);
	void call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION(Message*message);
	void call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE(Message*message);
	void call_RAY_MPI_TAG_LIBRARY_DISTANCE(Message*message);
	void call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES(Message*message);
	void call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED(Message*message);
	void call_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION(Message*message);
	void call_RAY_MPI_TAG_RECEIVED_COVERAGE_INFORMATION(Message*message);
	void call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE(Message*message);
	void call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY(Message*message);
	void call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY(Message*message);
	void call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(Message*message);
	void call_RAY_MPI_TAG_VERTICES_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY(Message*message);
	void call_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY(Message*message);
	void call_RAY_MPI_TAG_RECEIVED_MESSAGES(Message*message);
	void call_RAY_MPI_TAG_RECEIVED_MESSAGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY(Message*message);
	void call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY(Message*message);
	void call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY(Message*message);
	void call_RAY_MPI_TAG_MUST_RUN_REDUCER(Message*message);
	void call_RAY_MPI_TAG_ASK_BEGIN_REDUCTION(Message*message);
	void call_RAY_MPI_TAG_ASK_BEGIN_REDUCTION_REPLY(Message*message);
	void call_RAY_MPI_TAG_RESUME_VERTEX_DISTRIBUTION(Message*message);
	void call_RAY_MPI_TAG_REDUCE_MEMORY_CONSUMPTION_DONE(Message*message);
	void call_RAY_MPI_TAG_START_REDUCTION(Message*message);
	void call_RAY_MPI_TAG_VERIFY_INGOING_EDGES(Message*message);
	void call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES(Message*message);
	void call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_FORCE(Message*message);
	void call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY_FORCE(Message*message);
	void call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY_FORCE(Message*message);
	void call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_FORCE(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT(Message*message);
	void call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY(Message*message);
	void call_RAY_MPI_TAG_DELETE_VERTICES(Message*message);
	void call_RAY_MPI_TAG_DELETE_VERTICES_DONE(Message*message);
	void call_RAY_MPI_TAG_UPDATE_THRESHOLD(Message*message);
	void call_RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY(Message*message);
	void call_RAY_MPI_TAG_DELETE_VERTEX(Message*message);
	void call_RAY_MPI_TAG_DELETE_INGOING_EDGE(Message*message);
	void call_RAY_MPI_TAG_DELETE_OUTGOING_EDGE(Message*message);
	void call_RAY_MPI_TAG_DELETE_VERTEX_REPLY(Message*message);
	void call_RAY_MPI_TAG_DELETE_OUTGOING_EDGE_REPLY(Message*message);
	void call_RAY_MPI_TAG_DELETE_INGOING_EDGE_REPLY(Message*message);
	void call_RAY_MPI_TAG_CHECK_VERTEX(Message*message);
	void call_RAY_MPI_TAG_CHECK_VERTEX_REPLY(Message*message);
	void call_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY(Message*message);
	void call_RAY_MPI_TAG_MUST_RUN_REDUCER_FROM_MASTER(Message*message);
	void call_RAY_MPI_TAG_LOAD_SEQUENCES(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_READS(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY(Message*message);
	void call_RAY_MPI_TAG_GET_READ_MATE(Message*message);
	void call_RAY_MPI_TAG_GET_READ_MATE_REPLY(Message*message);
	void call_RAY_MPI_TAG_GET_COVERAGE_AND_MARK(Message*message);
	void call_RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY(Message*message);
	
public:
	void constructor(MessagesHandler*m_messagesHandler,
SeedingData*seedingData,
Library*m_library,
bool*m_ready,
VerticesExtractor*m_verticesExtractor,
SequencesLoader*m_sequencesLoader,
ExtensionData*ed,
			int*m_numberOfRanksDoneDetectingDistances,
			int*m_numberOfRanksDoneSendingDistances,
			Parameters*parameters,
			GridTable*m_subgraph,
			RingAllocator*m_outboxAllocator,
				int rank,
			int*m_numberOfMachinesDoneSendingEdges,
			FusionData*m_fusionData,
			int*m_wordSize,
			int*m_minimumCoverage,
			int*m_seedCoverage,
			int*m_peakCoverage,
			ArrayOfReads*m_myReads,
		int size,
	RingAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<uint64_t>*m_identifiers,
	bool*m_mode_sendDistribution,
	bool*m_alive,
	bool*m_colorSpaceMode,
	int*m_mode,
	vector<vector<uint64_t> >*m_allPaths,
	int*m_last_value,
	int*m_ranksDoneAttachingReads,
	int*m_DISTRIBUTE_n,
	int*m_numberOfRanksDoneSeeding,
	int*m_CLEAR_n,
	int*m_readyToSeed,
	int*m_FINISH_n,
	bool*m_nextReductionOccured,
	MyAllocator*m_directionsAllocator,
	int*m_mode_send_coverage_iterator,
	map<int,uint64_t>*m_coverageDistribution,
	int*m_sequence_ready_machines,
	int*m_numberOfMachinesReadyForEdgesDistribution,
	int*m_numberOfMachinesReadyToSendDistribution,
	bool*m_mode_send_outgoing_edges,
	int*m_mode_send_vertices_sequence_id,
	bool*m_mode_send_vertices,
	int*m_numberOfMachinesDoneSendingVertices,
	int*m_numberOfMachinesDoneSendingCoverage,
				StaticVector*m_outbox,
				StaticVector*m_inbox,
		map<int,int>*m_allIdentifiers,OpenAssemblerChooser*m_oa,
int*m_numberOfRanksWithCoverageData,
SeedExtender*seedExtender,int*m_master_mode,bool*m_isFinalFusion,
SequencesIndexer*m_si
);

	void processMessage(Message*message);
	MessageProcessor();

	void flushBuffers();
	void setReducer(MemoryConsumptionReducer*reducer);
};

#endif
