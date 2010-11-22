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

#ifndef _MessageProcessor
#define _MessageProcessor

#include<EdgesExtractor.h>
#include<RingAllocator.h>
#include<Library.h>
#include<OpenAssemblerChooser.h>
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
#include<Parameters.h>
#include<MyAllocator.h>
#include<Vertex.h>
using namespace std;


class MessageProcessor;
typedef void (MessageProcessor::*FNMETHOD) (Message*message);

class MessageProcessor{
	SequencesLoader*m_sequencesLoader;

	FNMETHOD m_methods[200];

	u64 m_sentinelValue;


	// data for processing
	bool*m_ready;
	int m_consumed;
	time_t m_last;

	Library*m_library;
	bool*m_isFinalFusion;
	int*m_master_mode;
	ExtensionData*ed;
	int*m_numberOfRanksDoneDetectingDistances;
	int*m_numberOfRanksDoneSendingDistances;
	Parameters*parameters;
	int*m_libraryIterator;
	bool*m_libraryIndexInitiated;

	MessagesHandler*m_messagesHandler;

	MyForest*m_subgraph;
	RingAllocator*m_outboxAllocator;
	int rank;
	vector<ReadAnnotation>*m_EXTENSION_receivedReads;
	int*m_numberOfMachinesDoneSendingEdges;
	FusionData*m_fusionData;
	vector<vector<VERTEX_TYPE> >*m_EXTENSION_contigs;
	int*m_wordSize;
	int*m_minimumCoverage;
	int*m_seedCoverage;
	int*m_peakCoverage;
	vector<Read*>*m_myReads;
	bool*m_EXTENSION_currentRankIsDone;
	vector<vector<VERTEX_TYPE> >*m_FINISH_newFusions;
	int size;
	RingAllocator*m_inboxAllocator;
	MyAllocator*m_persistentAllocator;
	vector<int>*m_identifiers;
	bool*m_mode_sendDistribution;
	bool*m_alive;
	vector<VERTEX_TYPE>*m_SEEDING_receivedIngoingEdges;
	VERTEX_TYPE*m_SEEDING_receivedKey;
	int*m_SEEDING_i;
	bool*m_colorSpaceMode;
	bool*m_FINISH_fusionOccured;
	bool*m_Machine_getPaths_INITIALIZED;
	int*m_mode;
	vector<vector<VERTEX_TYPE> >*m_allPaths;
	bool*m_EXTENSION_VertexAssembled_received;
	int*m_EXTENSION_numberOfRanksDone;
	int*m_EXTENSION_currentPosition;
	int*m_last_value;
	vector<int>*m_EXTENSION_identifiers;
	int*m_ranksDoneAttachingReads;
	bool*m_SEEDING_edgesReceived;
	PairedRead*m_EXTENSION_pairedRead;
	bool*m_mode_EXTENSION;
	vector<VERTEX_TYPE>*m_SEEDING_receivedOutgoingEdges;
	int*m_DISTRIBUTE_n;
	vector<VERTEX_TYPE>*m_SEEDING_nodes;
	bool*m_EXTENSION_hasPairedReadReceived;
	int*m_numberOfRanksDoneSeeding;
	bool*m_SEEDING_vertexKeyAndCoverageReceived;
	int*m_SEEDING_receivedVertexCoverage;
	bool*m_EXTENSION_readLength_received;
	bool*m_Machine_getPaths_DONE;
	int*m_CLEAR_n;
	bool*m_FINISH_vertex_received;
	bool*m_EXTENSION_initiated;
	int*m_readyToSeed;
	bool*m_SEEDING_NodeInitiated;
	int*m_FINISH_n;
	bool*m_nextReductionOccured;
	bool*m_EXTENSION_hasPairedReadAnswer;
	MyAllocator*m_directionsAllocator;
	map<int,int>*m_FINISH_pathLengths;
	bool*m_EXTENSION_pairedSequenceReceived;
	int*m_EXTENSION_receivedLength;
	int*m_mode_send_coverage_iterator;
	map<int,VERTEX_TYPE>*m_coverageDistribution;
	VERTEX_TYPE*m_FINISH_received_vertex;
	bool*m_EXTENSION_read_vertex_received;
	int*m_sequence_ready_machines;
	bool*m_SEEDING_InedgesReceived;
	bool*m_EXTENSION_vertexIsAssembledResult;
	bool*m_SEEDING_vertexCoverageReceived;
	VERTEX_TYPE*m_EXTENSION_receivedReadVertex;
	int*m_numberOfMachinesReadyForEdgesDistribution;
	int*m_numberOfMachinesReadyToSendDistribution;
	bool*m_mode_send_outgoing_edges;
	int*m_mode_send_vertices_sequence_id;
	bool*m_mode_send_vertices;
	int*m_numberOfMachinesDoneSendingVertices;
	VerticesExtractor*m_verticesExtractor;
	EdgesExtractor*m_edgesExtractor;
	int*m_numberOfMachinesDoneSendingCoverage;
	bool*m_EXTENSION_reads_received;
	StaticVector*m_outbox;
	map<int,int>*m_allIdentifiers;
	OpenAssemblerChooser*m_oa;
	int*m_numberOfRanksWithCoverageData;
	SeedExtender*seedExtender;


public:
	void constructor(MessagesHandler*m_messagesHandler,
Library*m_library,
bool*m_ready,
VerticesExtractor*m_verticesExtractor,
EdgesExtractor*m_edgesExtractor,
SequencesLoader*m_sequencesLoader,
ExtensionData*ed,
			int*m_numberOfRanksDoneDetectingDistances,
			int*m_numberOfRanksDoneSendingDistances,
			Parameters*parameters,
			int*m_libraryIterator,
			bool*m_libraryIndexInitiated,
			MyForest*m_subgraph,
			RingAllocator*m_outboxAllocator,
				int rank,
			vector<ReadAnnotation>*m_EXTENSION_receivedReads,
			int*m_numberOfMachinesDoneSendingEdges,
			FusionData*m_fusionData,
			vector<vector<VERTEX_TYPE> >*m_EXTENSION_contigs,
			int*m_wordSize,
			int*m_minimumCoverage,
			int*m_seedCoverage,
			int*m_peakCoverage,
			vector<Read*>*m_myReads,
			bool*m_EXTENSION_currentRankIsDone,
	vector<vector<VERTEX_TYPE> >*m_FINISH_newFusions,
		int size,
	RingAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<int>*m_identifiers,
	bool*m_mode_sendDistribution,
	bool*m_alive,
	vector<VERTEX_TYPE>*m_SEEDING_receivedIngoingEdges,
	VERTEX_TYPE*m_SEEDING_receivedKey,
	int*m_SEEDING_i,
	bool*m_colorSpaceMode,
	bool*m_FINISH_fusionOccured,
	bool*m_Machine_getPaths_INITIALIZED,
	int*m_mode,
	vector<vector<VERTEX_TYPE> >*m_allPaths,
	bool*m_EXTENSION_VertexAssembled_received,
	int*m_EXTENSION_numberOfRanksDone,
	int*m_EXTENSION_currentPosition,
	int*m_last_value,
	vector<int>*m_EXTENSION_identifiers,
	int*m_ranksDoneAttachingReads,
	bool*m_SEEDING_edgesReceived,
	PairedRead*m_EXTENSION_pairedRead,
	bool*m_mode_EXTENSION,
	vector<VERTEX_TYPE>*m_SEEDING_receivedOutgoingEdges,
	int*m_DISTRIBUTE_n,
	vector<VERTEX_TYPE>*m_SEEDING_nodes,
	bool*m_EXTENSION_hasPairedReadReceived,
	int*m_numberOfRanksDoneSeeding,
	bool*m_SEEDING_vertexKeyAndCoverageReceived,
	int*m_SEEDING_receivedVertexCoverage,
	bool*m_EXTENSION_readLength_received,
	bool*m_Machine_getPaths_DONE,
	int*m_CLEAR_n,
	bool*m_FINISH_vertex_received,
	bool*m_EXTENSION_initiated,
	int*m_readyToSeed,
	bool*m_SEEDING_NodeInitiated,
	int*m_FINISH_n,
	bool*m_nextReductionOccured,
	bool*m_EXTENSION_hasPairedReadAnswer,
	MyAllocator*m_directionsAllocator,
	map<int,int>*m_FINISH_pathLengths,
	bool*m_EXTENSION_pairedSequenceReceived,
	int*m_EXTENSION_receivedLength,
	int*m_mode_send_coverage_iterator,
	map<int,VERTEX_TYPE>*m_coverageDistribution,
	VERTEX_TYPE*m_FINISH_received_vertex,
	bool*m_EXTENSION_read_vertex_received,
	int*m_sequence_ready_machines,
	bool*m_SEEDING_InedgesReceived,
	bool*m_EXTENSION_vertexIsAssembledResult,
	bool*m_SEEDING_vertexCoverageReceived,
	VERTEX_TYPE*m_EXTENSION_receivedReadVertex,
	int*m_numberOfMachinesReadyForEdgesDistribution,
	int*m_numberOfMachinesReadyToSendDistribution,
	bool*m_mode_send_outgoing_edges,
	int*m_mode_send_vertices_sequence_id,
	bool*m_mode_send_vertices,
	int*m_numberOfMachinesDoneSendingVertices,
	int*m_numberOfMachinesDoneSendingCoverage,
	bool*m_EXTENSION_reads_received,
				StaticVector*m_outbox,
		map<int,int>*m_allIdentifiers,OpenAssemblerChooser*m_oa,
int*m_numberOfRanksWithCoverageData,
SeedExtender*seedExtender,int*m_master_mode,bool*m_isFinalFusion
);



	void call_TAG_WELCOME(Message*message);
	void call_TAG_SEND_SEQUENCE(Message*message);
	void call_TAG_SEND_SEQUENCE_REPLY(Message*message);
	void call_TAG_SEQUENCES_READY(Message*message);
	void call_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS(Message*message);
	void call_TAG_VERTICES_DATA(Message*message);
	void call_TAG_VERTICES_DISTRIBUTED(Message*message);
	void call_TAG_VERTEX_PTR_REQUEST(Message*message);
	void call_TAG_OUT_EDGE_DATA_WITH_PTR(Message*message);
	void call_TAG_OUT_EDGES_DATA(Message*message);
	void call_TAG_SHOW_VERTICES(Message*message);
	void call_TAG_START_VERTICES_DISTRIBUTION(Message*message);
	void call_TAG_EDGES_DISTRIBUTED(Message*message);
	void call_TAG_IN_EDGES_DATA(Message*message);
	void call_TAG_IN_EDGE_DATA_WITH_PTR(Message*message);
	void call_TAG_START_EDGES_DISTRIBUTION(Message*message);
	void call_TAG_START_EDGES_DISTRIBUTION_ASK(Message*message);
	void call_TAG_START_EDGES_DISTRIBUTION_ANSWER(Message*message);
	void call_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message);
	void call_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER(Message*message);
	void call_TAG_PREPARE_COVERAGE_DISTRIBUTION(Message*message);
	void call_TAG_COVERAGE_DATA(Message*message);
	void call_TAG_COVERAGE_END(Message*message);
	void call_TAG_SEND_COVERAGE_VALUES(Message*message);
	void call_TAG_READY_TO_SEED(Message*message);
	void call_TAG_START_SEEDING(Message*message);
	void call_TAG_REQUEST_VERTEX_COVERAGE(Message*message);
	void call_TAG_REQUEST_VERTEX_COVERAGE_REPLY(Message*message);
	void call_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE(Message*message);
	void call_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY(Message*message);
	void call_TAG_REQUEST_VERTEX_OUTGOING_EDGES(Message*message);
	void call_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY(Message*message);
	void call_TAG_SEEDING_IS_OVER(Message*message);
	void call_TAG_GOOD_JOB_SEE_YOU_SOON(Message*message);
	void call_TAG_I_GO_NOW(Message*message);
	void call_TAG_SET_WORD_SIZE(Message*message);
	void call_TAG_MASTER_IS_DONE_ATTACHING_READS(Message*message);
	void call_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY(Message*message);
	void call_TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER(Message*message);
	void call_TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY(Message*message);
	void call_TAG_REQUEST_VERTEX_INGOING_EDGES(Message*message);
	void call_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY(Message*message);
	void call_TAG_EXTENSION_IS_DONE(Message*message);
	void call_TAG_ASK_EXTENSION(Message*message);
	void call_TAG_ASK_IS_ASSEMBLED(Message*message);
	void call_TAG_ASK_REVERSE_COMPLEMENT(Message*message);
	void call_TAG_REQUEST_VERTEX_POINTER(Message*message);
	void call_TAG_ASK_IS_ASSEMBLED_REPLY(Message*message);
	void call_TAG_MARK_AS_ASSEMBLED(Message*message);
	void call_TAG_ASK_EXTENSION_DATA(Message*message);
	void call_TAG_EXTENSION_DATA(Message*message);
	void call_TAG_EXTENSION_END(Message*message);
	void call_TAG_EXTENSION_DATA_END(Message*message);
	void call_TAG_ATTACH_SEQUENCE(Message*message);
	void call_TAG_REQUEST_READS(Message*message);
	void call_TAG_REQUEST_READS_REPLY(Message*message);
	void call_TAG_ASK_READ_VERTEX_AT_POSITION(Message*message);
	void call_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY(Message*message);
	void call_TAG_ASK_READ_LENGTH(Message*message);
	void call_TAG_ASK_READ_LENGTH_REPLY(Message*message);
	void call_TAG_SAVE_WAVE_PROGRESSION(Message*message);
	void call_TAG_COPY_DIRECTIONS(Message*message);
	void call_TAG_ASSEMBLE_WAVES(Message*message);
	void call_TAG_SAVE_WAVE_PROGRESSION_REVERSE(Message*message);
	void call_TAG_ASSEMBLE_WAVES_DONE(Message*message);
	void call_TAG_START_FUSION(Message*message);
	void call_TAG_FUSION_DONE(Message*message);
	void call_TAG_ASK_VERTEX_PATHS_SIZE(Message*message);
	void call_TAG_ASK_VERTEX_PATHS_SIZE_REPLY(Message*message);
	void call_TAG_GET_PATH_LENGTH(Message*message);
	void call_TAG_GET_PATH_LENGTH_REPLY(Message*message);
	void call_TAG_CALIBRATION_MESSAGE(Message*message);
	void call_TAG_BEGIN_CALIBRATION(Message*message);
	void call_TAG_END_CALIBRATION(Message*message);
	void call_TAG_COMMUNICATION_STABILITY_MESSAGE(Message*message);
	void call_TAG_ASK_VERTEX_PATH(Message*message);
	void call_TAG_ASK_VERTEX_PATH_REPLY(Message*message);
	void call_TAG_INDEX_PAIRED_SEQUENCE(Message*message);
	void call_TAG_HAS_PAIRED_READ(Message*message);
	void call_TAG_HAS_PAIRED_READ_REPLY(Message*message);
	void call_TAG_GET_PAIRED_READ(Message*message);
	void call_TAG_GET_PAIRED_READ_REPLY(Message*message);
	void call_TAG_CLEAR_DIRECTIONS(Message*message);
	void call_TAG_CLEAR_DIRECTIONS_REPLY(Message*message);
	void call_TAG_FINISH_FUSIONS(Message*message);
	void call_TAG_FINISH_FUSIONS_FINISHED(Message*message);
	void call_TAG_DISTRIBUTE_FUSIONS(Message*message);
	void call_TAG_DISTRIBUTE_FUSIONS_FINISHED(Message*message);
	void call_TAG_EXTENSION_START(Message*message);
	void call_TAG_ELIMINATE_PATH(Message*message);
	void call_TAG_GET_PATH_VERTEX(Message*message);
	void call_TAG_GET_PATH_VERTEX_REPLY(Message*message);
	void call_TAG_SET_COLOR_MODE(Message*message);
	void call_TAG_AUTOMATIC_DISTANCE_DETECTION(Message*message);
	void call_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE(Message*message);
	void call_TAG_LIBRARY_DISTANCE(Message*message);
	void call_TAG_ASK_LIBRARY_DISTANCES(Message*message);
	void call_TAG_ASK_LIBRARY_DISTANCES_FINISHED(Message*message);
	void call_TAG_UPDATE_LIBRARY_INFORMATION(Message*message);
	void call_TAG_RECEIVED_COVERAGE_INFORMATION(Message*message);
	void call_TAG_REQUEST_READ_SEQUENCE(Message*message);
	void call_TAG_REQUEST_READ_SEQUENCE_REPLY(Message*message);
	void call_TAG_SAVE_WAVE_PROGRESSION_REPLY(Message*message);
	void call_TAG_SEND_SEQUENCE_REGULATOR(Message*message);
	void call_TAG_START_INDEXING_SEQUENCES(Message*message);
	void call_TAG_VERTICES_DATA_REPLY(Message*message);
	void call_TAG_IN_EDGES_DATA_REPLY(Message*message);
	void call_TAG_OUT_EDGES_DATA_REPLY(Message*message);
	void call_TAG_INDEX_PAIRED_SEQUENCE_REPLY(Message*message);
	void call_TAG_EXTENSION_DATA_REPLY(Message*message);
	void call_TAG_BARRIER(Message*message);
	void call_TAG_SHOW_SEQUENCES(Message*message);
	void call_TAG_LIBRARY_DISTANCE_REPLY(Message*message);
	void call_TAG_UPDATE_LIBRARY_INFORMATION_REPLY(Message*message);
	void call_TAG_RECEIVED_MESSAGES(Message*message);

	void processMessage(Message*message);
	MessageProcessor();
};

#endif
