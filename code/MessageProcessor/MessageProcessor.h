/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert

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

#ifndef _MessageProcessor_h
#define _MessageProcessor_h

#include <code/SeedExtender/OpenAssemblerChooser.h>
#include <code/SeedExtender/SeedExtender.h>
#include <code/SequencesLoader/ArrayOfReads.h>
#include <code/SequencesLoader/SequencesLoader.h>
#include <code/SequencesIndexer/ReadAnnotation.h>
#include <code/SequencesIndexer/SequencesIndexer.h>
#include <code/KmerAcademyBuilder/BloomFilter.h>
#include <code/Library/Library.h>
#include <code/SeedingData/SeedingData.h>
#include <code/FusionData/FusionData.h>
#include <code/VerticesExtractor/VerticesExtractor.h>
#include <code/VerticesExtractor/Vertex.h>
#include <code/VerticesExtractor/GridTable.h>
#include <code/Mock/Parameters.h>
#include <code/Scaffolder/Scaffolder.h>

#include <RayPlatform/memory/RingAllocator.h>
#include <RayPlatform/memory/MyAllocator.h>
#include <RayPlatform/structures/SplayTree.h>
#include <RayPlatform/structures/StaticVector.h>
#include <RayPlatform/communication/Message.h>
#include <RayPlatform/communication/BufferedData.h>
#include <RayPlatform/communication/VirtualCommunicator.h>
#include <RayPlatform/communication/MessageRouter.h>
#include <RayPlatform/scheduling/SwitchMan.h>
#include <RayPlatform/plugins/CorePlugin.h>
#include <RayPlatform/core/ComputeCore.h>

#include <vector>
using namespace std;

__DeclarePlugin(MessageProcessor);

__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_CONTIG_INFO);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SCAFFOLDING_LINKS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_READ_MARKERS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_READ_MATE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_READS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SET_WORD_SIZE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_READS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_INFO);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_READS_FROM_LIST);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_START_INDEXING_SEQUENCES);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEQUENCES_READY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTICES_DATA);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_PURGE_NULL_EDGES);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTICES_DISTRIBUTED);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_OUT_EDGES_DATA_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_OUT_EDGES_DATA);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_IN_EDGES_DATA_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_IN_EDGES_DATA); /**/
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_COVERAGE_DATA);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_COVERAGE_END);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEND_COVERAGE_VALUES);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_READY_TO_SEED);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_START_SEEDING);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_COVERAGE_AND_MARK);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_EDGES);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEEDING_IS_OVER);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_SEED_LENGTHS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEND_SEED_LENGTHS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_IS_DONE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_EXTENSION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ATTACH_SEQUENCE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_LENGTH);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASSEMBLE_WAVES);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_START_FUSION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_FUSION_DONE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_LENGTH);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATH);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_HAS_PAIRED_READ);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PAIRED_READ);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PAIRED_READ_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_CLEAR_DIRECTIONS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_FINISH_FUSIONS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_START);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ELIMINATE_PATH);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_VERTEX);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_WRITE_AMOS);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_WRITE_AMOS_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_LIBRARY_DISTANCE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_READ_SEQUENCE);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY);
__DeclareMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_I_FINISHED_SCAFFOLDING);

__DeclareMessageTagAdapter(MessageProcessor, RAY_MESSAGE_TAG_PUSH_SEEDS);
__DeclareMessageTagAdapter(MessageProcessor, RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY);

/**
 * MessageProcessor receives all the messages of a MPI rank
 * Message objects may also be checked using the Message inbox (m_inbox)
 *
 * Sometimes, a message will generate a reply (_REPLY)
 *
 * \author Sébastien Boisvert
 */
class MessageProcessor :  public CorePlugin {

	__AddAdapter(MessageProcessor,RAY_MPI_TAG_CONTIG_INFO);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SCAFFOLDING_LINKS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_READ_MARKERS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_READ_MATE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_READS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SET_WORD_SIZE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_READS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_INFO);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_READS_FROM_LIST);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_START_INDEXING_SEQUENCES);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SEQUENCES_READY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_VERTICES_DATA);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_PURGE_NULL_EDGES);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_VERTICES_DISTRIBUTED);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_OUT_EDGES_DATA_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_OUT_EDGES_DATA);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_IN_EDGES_DATA_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_IN_EDGES_DATA); /**/
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_COVERAGE_DATA);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_COVERAGE_END);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SEND_COVERAGE_VALUES);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_READY_TO_SEED);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_START_SEEDING);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_COVERAGE_AND_MARK);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_EDGES);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SEEDING_IS_OVER);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_SEED_LENGTHS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SEND_SEED_LENGTHS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_IS_DONE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_EXTENSION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ATTACH_SEQUENCE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_LENGTH);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASSEMBLE_WAVES);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_START_FUSION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_FUSION_DONE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_LENGTH);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATH);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_HAS_PAIRED_READ);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_PAIRED_READ);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_PAIRED_READ_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_CLEAR_DIRECTIONS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_FINISH_FUSIONS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_START);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ELIMINATE_PATH);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_VERTEX);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_WRITE_AMOS);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_WRITE_AMOS_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_LIBRARY_DISTANCE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_READ_SEQUENCE);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY);
	__AddAdapter(MessageProcessor,RAY_MPI_TAG_I_FINISHED_SCAFFOLDING);

	__AddAdapter(MessageProcessor, RAY_MESSAGE_TAG_PUSH_SEEDS);
	__AddAdapter(MessageProcessor, RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY);

	uint64_t m_bloomBits;

	MessageTag RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION;
	MessageTag RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER;
	MessageTag RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION;
	MessageTag RAY_MPI_TAG_PURGE_NULL_EDGES;
	MessageTag RAY_MPI_TAG_READY_TO_SEED;
	MessageTag RAY_MPI_TAG_REQUEST_READ_SEQUENCE;
	MessageTag RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY;
	MessageTag RAY_MPI_TAG_REQUEST_SEED_LENGTHS;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_EDGES;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY;
	MessageTag RAY_MPI_TAG_SAVE_WAVE_PROGRESSION;
	MessageTag RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY;
	MessageTag RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY;
	MessageTag RAY_MPI_TAG_SCAFFOLDING_LINKS;
	MessageTag RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY;
	MessageTag RAY_MPI_TAG_SEEDING_IS_OVER;
	MessageTag RAY_MPI_TAG_SEND_COVERAGE_VALUES;
	MessageTag RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY;
	MessageTag RAY_MPI_TAG_SEND_SEED_LENGTHS;
	MessageTag RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY;
	MessageTag RAY_MPI_TAG_SEQUENCES_READY;
	MessageTag RAY_MPI_TAG_SET_WORD_SIZE;
	MessageTag RAY_MPI_TAG_START_FUSION;
	MessageTag RAY_MPI_TAG_START_INDEXING_SEQUENCES;
	MessageTag RAY_MPI_TAG_START_SEEDING;
	MessageTag RAY_MPI_TAG_START_VERTICES_DISTRIBUTION;
	MessageTag RAY_MPI_TAG_TEST_NETWORK_MESSAGE;
	MessageTag RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY;
	MessageTag RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION;
	MessageTag RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_INFO;
	MessageTag RAY_MPI_TAG_VERTEX_INFO_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_READS;
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST;
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_READS_REPLY;
	MessageTag RAY_MPI_TAG_VERTICES_DATA;
	MessageTag RAY_MPI_TAG_VERTICES_DATA_REPLY;
	MessageTag RAY_MPI_TAG_VERTICES_DISTRIBUTED;
	MessageTag RAY_MPI_TAG_WRITE_AMOS;
	MessageTag RAY_MPI_TAG_WRITE_AMOS_REPLY;

	MessageTag RAY_MPI_TAG_CONTIG_INFO;
	MessageTag RAY_MPI_TAG_CONTIG_INFO_REPLY;
	MessageTag RAY_MPI_TAG_COVERAGE_DATA;
	MessageTag RAY_MPI_TAG_COVERAGE_DATA_REPLY;
	MessageTag RAY_MPI_TAG_COVERAGE_END;
	MessageTag RAY_MPI_TAG_DISTRIBUTE_FUSIONS;
	MessageTag RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED;
	MessageTag RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY;
	MessageTag RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY;
	MessageTag RAY_MPI_TAG_ELIMINATE_PATH;
	MessageTag RAY_MPI_TAG_EXTENSION_DATA;
	MessageTag RAY_MPI_TAG_EXTENSION_DATA_REPLY;
	MessageTag RAY_MPI_TAG_EXTENSION_IS_DONE;
	MessageTag RAY_MPI_TAG_EXTENSION_START;
	MessageTag RAY_MPI_TAG_FINISH_FUSIONS;
	MessageTag RAY_MPI_TAG_FINISH_FUSIONS_FINISHED;
	MessageTag RAY_MPI_TAG_FUSION_DONE;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_MARK;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY;
	MessageTag RAY_MPI_TAG_GET_PAIRED_READ;
	MessageTag RAY_MPI_TAG_GET_PAIRED_READ_REPLY;
	MessageTag RAY_MPI_TAG_GET_PATH_LENGTH;
	MessageTag RAY_MPI_TAG_GET_PATH_LENGTH_REPLY;
	MessageTag RAY_MPI_TAG_GET_PATH_VERTEX;
	MessageTag RAY_MPI_TAG_GET_PATH_VERTEX_REPLY;
	MessageTag RAY_MPI_TAG_GET_READ_MARKERS;
	MessageTag RAY_MPI_TAG_GET_READ_MARKERS_REPLY;
	MessageTag RAY_MPI_TAG_GET_READ_MATE;
	MessageTag RAY_MPI_TAG_GET_READ_MATE_REPLY;
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY;
	MessageTag RAY_MPI_TAG_HAS_PAIRED_READ;
	MessageTag RAY_MPI_TAG_HAS_PAIRED_READ_REPLY;
	MessageTag RAY_MPI_TAG_I_FINISHED_SCAFFOLDING;
	MessageTag RAY_MPI_TAG_IN_EDGES_DATA;
	MessageTag RAY_MPI_TAG_IN_EDGES_DATA_REPLY;
	MessageTag RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS;
	MessageTag RAY_MPI_TAG_KMER_ACADEMY_DATA;
	MessageTag RAY_MPI_TAG_KMER_ACADEMY_DATA_REPLY;
	MessageTag RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED;
	MessageTag RAY_MPI_TAG_LIBRARY_DISTANCE;
	MessageTag RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY;
	MessageTag RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS;
	MessageTag RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY;
	MessageTag RAY_MPI_TAG_OUT_EDGES_DATA;
	MessageTag RAY_MPI_TAG_OUT_EDGES_DATA_REPLY;

	MessageTag RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER;
	MessageTag RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY;
	MessageTag RAY_MPI_TAG_ASK_EXTENSION;
	MessageTag RAY_MPI_TAG_ASK_LIBRARY_DISTANCES;
	MessageTag RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED;
	MessageTag RAY_MPI_TAG_ASK_READ_LENGTH;
	MessageTag RAY_MPI_TAG_ASK_READ_LENGTH_REPLY;
	MessageTag RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION;
	MessageTag RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY;
	MessageTag RAY_MPI_TAG_ASSEMBLE_WAVES;
	MessageTag RAY_MPI_TAG_ASSEMBLE_WAVES_DONE;
	MessageTag RAY_MPI_TAG_ATTACH_SEQUENCE;
	MessageTag RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY;
	MessageTag RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION;
	MessageTag RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE;
	MessageTag RAY_MPI_TAG_CLEAR_DIRECTIONS;
	MessageTag RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY;

	MessageTag RAY_MESSAGE_TAG_PUSH_SEEDS;
	MessageTag RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY;

	MasterMode RAY_MASTER_MODE_ASK_DISTANCES;
	MasterMode RAY_MASTER_MODE_DO_NOTHING;
	MasterMode RAY_MASTER_MODE_PREPARE_SEEDING;
	MasterMode RAY_MASTER_MODE_PURGE_NULL_EDGES;
	MasterMode RAY_MASTER_MODE_START_UPDATING_DISTANCES;
	MasterMode RAY_MASTER_MODE_TRIGGER_DETECTION;
	MasterMode RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS;
	MasterMode RAY_MASTER_MODE_TRIGGER_FUSIONS;
	MasterMode RAY_MASTER_MODE_TRIGGER_SEEDING;
	MasterMode RAY_MASTER_MODE_WRITE_SCAFFOLDS;

	MasterMode RAY_MASTER_MODE_REGISTER_SEEDS;

	SlaveMode RAY_SLAVE_MODE_PURGE_NULL_EDGES;

	SlaveMode RAY_SLAVE_MODE_AMOS;
	SlaveMode RAY_SLAVE_MODE_ASSEMBLE_WAVES;
	SlaveMode RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION;
	SlaveMode RAY_SLAVE_MODE_ADD_VERTICES;
	SlaveMode RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS;
	SlaveMode RAY_SLAVE_MODE_EXTENSION;
	SlaveMode RAY_SLAVE_MODE_FINISH_FUSIONS;
	SlaveMode RAY_SLAVE_MODE_FUSION;
	SlaveMode RAY_SLAVE_MODE_INDEX_SEQUENCES;
	SlaveMode RAY_SLAVE_MODE_LOAD_SEQUENCES;
	SlaveMode RAY_SLAVE_MODE_SEND_DISTRIBUTION;
	SlaveMode RAY_SLAVE_MODE_SEND_EXTENSION_DATA;
	SlaveMode RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES;
	SlaveMode RAY_SLAVE_MODE_SEND_SEED_LENGTHS;
	SlaveMode RAY_SLAVE_MODE_START_SEEDING;


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
	ArrayOfReads*m_myReads;
	int m_size;
	SequencesIndexer*m_si;

	RingAllocator*m_inboxAllocator;
	MyAllocator*m_persistentAllocator;
	vector<PathHandle>*m_identifiers;
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
	map<CoverageDepth,LargeCount>*m_coverageDistribution;
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
			ArrayOfReads*m_myReads,
		int size,
	RingAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<PathHandle>*m_identifiers,
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
	int*m_mode_send_coverage_iterator,
	map<CoverageDepth,LargeCount>*m_coverageDistribution,
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

	void call_RAY_MPI_TAG_CONTIG_INFO(Message*message);
	void call_RAY_MPI_TAG_SCAFFOLDING_LINKS(Message*message);
	void call_RAY_MPI_TAG_GET_READ_MARKERS(Message*message);
	void call_RAY_MPI_TAG_GET_READ_MATE(Message*message);
	void call_RAY_MPI_TAG_REQUEST_VERTEX_READS(Message*message);
	void call_RAY_MPI_TAG_SET_WORD_SIZE(Message*message);
	void call_RAY_MPI_TAG_VERTEX_READS(Message*message);
	void call_RAY_MPI_TAG_VERTEX_INFO(Message*message);
	void call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT(Message*message);
	void call_RAY_MPI_TAG_VERTEX_READS_FROM_LIST(Message*message);
	void call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(Message*message);
	void call_RAY_MPI_TAG_SEQUENCES_READY(Message*message);
	void call_RAY_MPI_TAG_VERTICES_DATA(Message*message);
	void call_RAY_MPI_TAG_PURGE_NULL_EDGES(Message*message);
	void call_RAY_MPI_TAG_VERTICES_DISTRIBUTED(Message*message);
	void call_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_OUT_EDGES_DATA(Message*message);
	void call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION(Message*message);
	void call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_IN_EDGES_DATA(Message*message);
	void call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message);
	void call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER(Message*message);
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
	void call_RAY_MPI_TAG_EXTENSION_DATA_REPLY(Message*message);
	void call_RAY_MPI_TAG_EXTENSION_DATA(Message*message);
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
	
	void call_RAY_MESSAGE_TAG_PUSH_SEEDS(Message*message);
	void call_RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY(Message*message);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif
