/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _MessageProcessor
#define _MessageProcessor


#include <vector>
using namespace std;

#include <memory/RingAllocator.h>
#include <memory/MyAllocator.h>

#include <heuristics/OpenAssemblerChooser.h>

#include <structures/ArrayOfReads.h>
#include <structures/SplayTree.h>
#include <structures/StaticVector.h>
#include <structures/ReadAnnotation.h>
#include <structures/Vertex.h>
#include <structures/BloomFilter.h>

#include <assembler/Library.h>
#include <assembler/SequencesIndexer.h>
#include <assembler/SeedingData.h>
#include <assembler/SeedExtender.h>
#include <assembler/SequencesLoader.h>
#include <assembler/FusionData.h>
#include <assembler/VerticesExtractor.h>

#include <graph/GridTable.h>
#include <core/Parameters.h>
#include <scaffolder/Scaffolder.h>

#include <communication/Message.h>
#include <communication/BufferedData.h>
#include <communication/VirtualCommunicator.h>
#include <communication/MessageRouter.h>

#include <scheduling/SwitchMan.h>
#include <scripting/ScriptEngine.h>
#include <core/CorePlugin.h>

#include <communication/MessageProcessor_adapters.h>

/**
 * MessageProcessor receives all the messages of a MPI rank
 * Message objects may also be checked using the Message inbox (m_inbox)
 *
 * Sometimes, a message will generate a reply (_REPLY)
 * \author Sébastien Boisvert
 */
class MessageProcessor :  public CorePlugin {

	Adapter_RAY_MPI_TAG_LOAD_SEQUENCES m_adapter_RAY_MPI_TAG_LOAD_SEQUENCES;
	Adapter_RAY_MPI_TAG_CONTIG_INFO m_adapter_RAY_MPI_TAG_CONTIG_INFO;
	Adapter_RAY_MPI_TAG_SCAFFOLDING_LINKS m_adapter_RAY_MPI_TAG_SCAFFOLDING_LINKS;
	Adapter_RAY_MPI_TAG_GET_READ_MARKERS m_adapter_RAY_MPI_TAG_GET_READ_MARKERS;
	Adapter_RAY_MPI_TAG_GET_READ_MATE m_adapter_RAY_MPI_TAG_GET_READ_MATE;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_READS m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_READS;
	Adapter_RAY_MPI_TAG_SET_WORD_SIZE m_adapter_RAY_MPI_TAG_SET_WORD_SIZE;
	Adapter_RAY_MPI_TAG_VERTEX_READS m_adapter_RAY_MPI_TAG_VERTEX_READS;
	Adapter_RAY_MPI_TAG_VERTEX_INFO m_adapter_RAY_MPI_TAG_VERTEX_INFO;
	Adapter_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT m_adapter_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	Adapter_RAY_MPI_TAG_BUILD_GRAPH m_adapter_RAY_MPI_TAG_BUILD_GRAPH;
	Adapter_RAY_MPI_TAG_VERTEX_READS_FROM_LIST m_adapter_RAY_MPI_TAG_VERTEX_READS_FROM_LIST;
	Adapter_RAY_MPI_TAG_START_INDEXING_SEQUENCES m_adapter_RAY_MPI_TAG_START_INDEXING_SEQUENCES;
	Adapter_RAY_MPI_TAG_SEQUENCES_READY m_adapter_RAY_MPI_TAG_SEQUENCES_READY;
	Adapter_RAY_MPI_TAG_VERTICES_DATA m_adapter_RAY_MPI_TAG_VERTICES_DATA;
	Adapter_RAY_MPI_TAG_VERTICES_DATA_REPLY m_adapter_RAY_MPI_TAG_VERTICES_DATA_REPLY;
	Adapter_RAY_MPI_TAG_PURGE_NULL_EDGES m_adapter_RAY_MPI_TAG_PURGE_NULL_EDGES;
	Adapter_RAY_MPI_TAG_VERTICES_DISTRIBUTED m_adapter_RAY_MPI_TAG_VERTICES_DISTRIBUTED;
	Adapter_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY m_adapter_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY;
	Adapter_RAY_MPI_TAG_OUT_EDGES_DATA m_adapter_RAY_MPI_TAG_OUT_EDGES_DATA;
	Adapter_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION m_adapter_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION;
	Adapter_RAY_MPI_TAG_IN_EDGES_DATA_REPLY m_adapter_RAY_MPI_TAG_IN_EDGES_DATA_REPLY;
	Adapter_RAY_MPI_TAG_IN_EDGES_DATA m_adapter_RAY_MPI_TAG_IN_EDGES_DATA;
	Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION m_adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION;
	Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER m_adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER;
	Adapter_RAY_MPI_TAG_TEST_NETWORK_MESSAGE m_adapter_RAY_MPI_TAG_TEST_NETWORK_MESSAGE;
	Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION m_adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION;
	Adapter_RAY_MPI_TAG_COVERAGE_DATA m_adapter_RAY_MPI_TAG_COVERAGE_DATA;
	Adapter_RAY_MPI_TAG_COVERAGE_END m_adapter_RAY_MPI_TAG_COVERAGE_END;
	Adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES m_adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES;
	Adapter_RAY_MPI_TAG_READY_TO_SEED m_adapter_RAY_MPI_TAG_READY_TO_SEED;
	Adapter_RAY_MPI_TAG_START_SEEDING m_adapter_RAY_MPI_TAG_START_SEEDING;
	Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_MARK m_adapter_RAY_MPI_TAG_GET_COVERAGE_AND_MARK;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY;
	Adapter_RAY_MPI_TAG_SEEDING_IS_OVER m_adapter_RAY_MPI_TAG_SEEDING_IS_OVER;
	Adapter_RAY_MPI_TAG_REQUEST_SEED_LENGTHS m_adapter_RAY_MPI_TAG_REQUEST_SEED_LENGTHS;
	Adapter_RAY_MPI_TAG_SEND_SEED_LENGTHS m_adapter_RAY_MPI_TAG_SEND_SEED_LENGTHS;
	Adapter_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS m_adapter_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS;
	Adapter_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER m_adapter_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER;
	Adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS m_adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS;
	Adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY m_adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES;
	Adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY;
	Adapter_RAY_MPI_TAG_EXTENSION_IS_DONE m_adapter_RAY_MPI_TAG_EXTENSION_IS_DONE;
	Adapter_RAY_MPI_TAG_ASK_EXTENSION m_adapter_RAY_MPI_TAG_ASK_EXTENSION;
	Adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED m_adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED;
	Adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY m_adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY;
	Adapter_RAY_MPI_TAG_ASK_EXTENSION_DATA m_adapter_RAY_MPI_TAG_ASK_EXTENSION_DATA;
	Adapter_RAY_MPI_TAG_EXTENSION_DATA_REPLY m_adapter_RAY_MPI_TAG_EXTENSION_DATA_REPLY;
	Adapter_RAY_MPI_TAG_EXTENSION_DATA m_adapter_RAY_MPI_TAG_EXTENSION_DATA;
	Adapter_RAY_MPI_TAG_EXTENSION_DATA_END m_adapter_RAY_MPI_TAG_EXTENSION_DATA_END;
	Adapter_RAY_MPI_TAG_ATTACH_SEQUENCE m_adapter_RAY_MPI_TAG_ATTACH_SEQUENCE;
	Adapter_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY m_adapter_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY;
	Adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION m_adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION;
	Adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY m_adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY;
	Adapter_RAY_MPI_TAG_ASK_READ_LENGTH m_adapter_RAY_MPI_TAG_ASK_READ_LENGTH;
	Adapter_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY m_adapter_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY;
	Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY m_adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY;
	Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION m_adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION;
	Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY m_adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY;
	Adapter_RAY_MPI_TAG_ASSEMBLE_WAVES m_adapter_RAY_MPI_TAG_ASSEMBLE_WAVES;
	Adapter_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE m_adapter_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE;
	Adapter_RAY_MPI_TAG_START_FUSION m_adapter_RAY_MPI_TAG_START_FUSION;
	Adapter_RAY_MPI_TAG_FUSION_DONE m_adapter_RAY_MPI_TAG_FUSION_DONE;
	Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE m_adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY m_adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY;
	Adapter_RAY_MPI_TAG_GET_PATH_LENGTH m_adapter_RAY_MPI_TAG_GET_PATH_LENGTH;
	Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION m_adapter_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION;
	Adapter_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY m_adapter_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY;
	Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS m_adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS;
	Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY m_adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY;
	Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END m_adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END;
	Adapter_RAY_MPI_TAG_ASK_VERTEX_PATH m_adapter_RAY_MPI_TAG_ASK_VERTEX_PATH;
	Adapter_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY m_adapter_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY;
	Adapter_RAY_MPI_TAG_HAS_PAIRED_READ m_adapter_RAY_MPI_TAG_HAS_PAIRED_READ;
	Adapter_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY m_adapter_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY;
	Adapter_RAY_MPI_TAG_GET_PAIRED_READ m_adapter_RAY_MPI_TAG_GET_PAIRED_READ;
	Adapter_RAY_MPI_TAG_GET_PAIRED_READ_REPLY m_adapter_RAY_MPI_TAG_GET_PAIRED_READ_REPLY;
	Adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS m_adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS;
	Adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY m_adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY;
	Adapter_RAY_MPI_TAG_FINISH_FUSIONS m_adapter_RAY_MPI_TAG_FINISH_FUSIONS;
	Adapter_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED m_adapter_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED;
	Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS m_adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS;
	Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY m_adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY;
	Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED m_adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED;
	Adapter_RAY_MPI_TAG_EXTENSION_START m_adapter_RAY_MPI_TAG_EXTENSION_START;
	Adapter_RAY_MPI_TAG_ELIMINATE_PATH m_adapter_RAY_MPI_TAG_ELIMINATE_PATH;
	Adapter_RAY_MPI_TAG_GET_PATH_VERTEX m_adapter_RAY_MPI_TAG_GET_PATH_VERTEX;
	Adapter_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY m_adapter_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY;
	Adapter_RAY_MPI_TAG_WRITE_AMOS m_adapter_RAY_MPI_TAG_WRITE_AMOS;
	Adapter_RAY_MPI_TAG_WRITE_AMOS_REPLY m_adapter_RAY_MPI_TAG_WRITE_AMOS_REPLY;
	Adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION m_adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION;
	Adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE m_adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE;
	Adapter_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY m_adapter_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY;
	Adapter_RAY_MPI_TAG_LIBRARY_DISTANCE m_adapter_RAY_MPI_TAG_LIBRARY_DISTANCE;
	Adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES m_adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES;
	Adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED m_adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED;
	Adapter_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION m_adapter_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION;
	Adapter_RAY_MPI_TAG_KMER_ACADEMY_DATA m_adapter_RAY_MPI_TAG_KMER_ACADEMY_DATA;
	Adapter_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED m_adapter_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED;
	Adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY m_adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY;
	Adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE m_adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE;
	Adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY m_adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY;
	Adapter_RAY_MPI_TAG_I_FINISHED_SCAFFOLDING m_adapter_RAY_MPI_TAG_I_FINISHED_SCAFFOLDING;
	Adapter_RAY_MPI_TAG_GET_CONTIG_CHUNK m_adapter_RAY_MPI_TAG_GET_CONTIG_CHUNK;
	Adapter_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL m_adapter_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL;

	MessageRouter*m_router;

	int m_kmerAcademyFinishedRanks;
	BloomFilter m_bloomFilter;

	VirtualCommunicator*m_virtualCommunicator;
	Scaffolder*m_scaffolder;
	int m_count;

	uint64_t m_lastSize;

	SequencesLoader*m_sequencesLoader;

	uint64_t m_sentinelValue;

	SeedingData*m_seedingData;

	/* switch man for synchronization
 */
	SwitchMan*m_switchMan;

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
	Parameters*m_parameters;

	GridTable*m_subgraph;

	RingAllocator*m_outboxAllocator;
	int m_rank;
	int*m_numberOfMachinesDoneSendingEdges;
	FusionData*m_fusionData;
	int*m_wordSize;
	ArrayOfReads*m_myReads;
	int m_size;
	SequencesIndexer*m_si;

	RingAllocator*m_inboxAllocator;
	MyAllocator*m_persistentAllocator;
	vector<uint64_t>*m_identifiers;
	bool*m_mode_sendDistribution;
	bool*m_alive;
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


public:

	void assignHandlers(ScriptEngine*engine);

	void constructor(
MessageRouter*router,
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
			ArrayOfReads*m_myReads,
		int size,
	RingAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<uint64_t>*m_identifiers,
	bool*m_mode_sendDistribution,
	bool*m_alive,
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
		OpenAssemblerChooser*m_oa,
int*m_numberOfRanksWithCoverageData,
SeedExtender*seedExtender,int*m_master_mode,bool*m_isFinalFusion,
SequencesIndexer*m_si
);

	void processMessage(Message*message);
	MessageProcessor();

	void flushBuffers();
	void setScaffolder(Scaffolder*a);
	void setVirtualCommunicator(VirtualCommunicator*a);
	void setSwitchMan(SwitchMan*a);

// list of declarations

	void call_RAY_MPI_TAG_LOAD_SEQUENCES(Message*message);
	void call_RAY_MPI_TAG_CONTIG_INFO(Message*message);
	void call_RAY_MPI_TAG_SCAFFOLDING_LINKS(Message*message);
	void call_RAY_MPI_TAG_GET_READ_MARKERS(Message*message);
	void call_RAY_MPI_TAG_GET_READ_MATE(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_READS(Message*message);
	void call_RAY_MPI_TAG_SET_WORD_SIZE(Message*message);
	void call_RAY_MPI_TAG_VERTEX_READS(Message*message);
	void call_RAY_MPI_TAG_VERTEX_INFO(Message*message);
	void call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT(Message*message);
	void call_RAY_MPI_TAG_BUILD_GRAPH(Message*message);
	void call_RAY_MPI_TAG_VERTEX_READS_FROM_LIST(Message*message);
	void call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(Message*message);
	void call_RAY_MPI_TAG_SEQUENCES_READY(Message*message);
	void call_RAY_MPI_TAG_VERTICES_DATA(Message*message);
	void call_RAY_MPI_TAG_VERTICES_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_PURGE_NULL_EDGES(Message*message);
	void call_RAY_MPI_TAG_VERTICES_DISTRIBUTED(Message*message);
	void call_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_OUT_EDGES_DATA(Message*message);
	void call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION(Message*message);
	void call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_IN_EDGES_DATA(Message*message);
	void call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message);
	void call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER(Message*message);
	void call_RAY_MPI_TAG_TEST_NETWORK_MESSAGE(Message*message);
	void call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION(Message*message);
	void call_RAY_MPI_TAG_COVERAGE_DATA(Message*message);
	void call_RAY_MPI_TAG_COVERAGE_END(Message*message);
	void call_RAY_MPI_TAG_SEND_COVERAGE_VALUES(Message*message);
	void call_RAY_MPI_TAG_READY_TO_SEED(Message*message);
	void call_RAY_MPI_TAG_START_SEEDING(Message*message);
	void call_RAY_MPI_TAG_GET_COVERAGE_AND_MARK(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_SEEDING_IS_OVER(Message*message);
	void call_RAY_MPI_TAG_REQUEST_SEED_LENGTHS(Message*message);
	void call_RAY_MPI_TAG_SEND_SEED_LENGTHS(Message*message);
	void call_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS(Message*message);
	void call_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER(Message*message);
	void call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS(Message*message);
	void call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_IS_DONE(Message*message);
	void call_RAY_MPI_TAG_ASK_EXTENSION(Message*message);
	void call_RAY_MPI_TAG_ASK_IS_ASSEMBLED(Message*message);
	void call_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_EXTENSION_DATA(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_DATA(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_DATA_END(Message*message);
	void call_RAY_MPI_TAG_ATTACH_SEQUENCE(Message*message);
	void call_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION(Message*message);
	void call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_READ_LENGTH(Message*message);
	void call_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY(Message*message);
	void call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY(Message*message);
	void call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION(Message*message);
	void call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASSEMBLE_WAVES(Message*message);
	void call_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE(Message*message);
	void call_RAY_MPI_TAG_START_FUSION(Message*message);
	void call_RAY_MPI_TAG_FUSION_DONE(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY(Message*message);
	void call_RAY_MPI_TAG_GET_PATH_LENGTH(Message*message);
	void call_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION(Message*message);
	void call_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY(Message*message);
	void call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END(Message*message);
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
	void call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY(Message*message);
	void call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_START(Message*message);
	void call_RAY_MPI_TAG_ELIMINATE_PATH(Message*message);
	void call_RAY_MPI_TAG_GET_PATH_VERTEX(Message*message);
	void call_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY(Message*message);
	void call_RAY_MPI_TAG_WRITE_AMOS(Message*message);
	void call_RAY_MPI_TAG_WRITE_AMOS_REPLY(Message*message);
	void call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION(Message*message);
	void call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE(Message*message);
	void call_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY(Message*message);
	void call_RAY_MPI_TAG_LIBRARY_DISTANCE(Message*message);
	void call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES(Message*message);
	void call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED(Message*message);
	void call_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION(Message*message);
	void call_RAY_MPI_TAG_KMER_ACADEMY_DATA(Message*message);
	void call_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED(Message*message);
	void call_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY(Message*message);
	void call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE(Message*message);
	void call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY(Message*message);
	void call_RAY_MPI_TAG_I_FINISHED_SCAFFOLDING(Message*message);
	void call_RAY_MPI_TAG_GET_CONTIG_CHUNK(Message*message);
	void call_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL(Message*message);
	
	void registerPlugin(ComputeCore*core);
};

#endif
	

