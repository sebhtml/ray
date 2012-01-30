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

#ifndef _MessageProcessor_adapters_h
#define _MessageProcessor_adapters_h

#include <handlers/MessageTagHandler.h>

class MessageProcessor;

class Adapter_RAY_MPI_TAG_LOAD_SEQUENCES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_CONTIG_INFO: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SCAFFOLDING_LINKS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_READ_MARKERS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_READ_MATE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_READS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SET_WORD_SIZE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_VERTEX_READS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_VERTEX_INFO: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_VERTEX_READS_FROM_LIST: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_START_INDEXING_SEQUENCES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SEQUENCES_READY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_VERTICES_DATA: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_VERTICES_DATA_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_PURGE_NULL_EDGES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_VERTICES_DISTRIBUTED: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_OUT_EDGES_DATA: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_IN_EDGES_DATA_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_IN_EDGES_DATA: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_TEST_NETWORK_MESSAGE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_COVERAGE_DATA: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_COVERAGE_END: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_READY_TO_SEED: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_START_SEEDING: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_MARK: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SEEDING_IS_OVER: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_SEED_LENGTHS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SEND_SEED_LENGTHS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_EXTENSION_IS_DONE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_EXTENSION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_EXTENSION_DATA: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_EXTENSION_DATA_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_EXTENSION_DATA: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_EXTENSION_DATA_END: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ATTACH_SEQUENCE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_READ_LENGTH: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASSEMBLE_WAVES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_START_FUSION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_FUSION_DONE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_PATH_LENGTH: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_VERTEX_PATH: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_HAS_PAIRED_READ: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_PAIRED_READ: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_PAIRED_READ_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_FINISH_FUSIONS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_EXTENSION_START: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ELIMINATE_PATH: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_PATH_VERTEX: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_WRITE_AMOS: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_WRITE_AMOS_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_LIBRARY_DISTANCE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_KMER_ACADEMY_DATA: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_I_FINISHED_SCAFFOLDING: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

class Adapter_RAY_MPI_TAG_GET_CONTIG_CHUNK: public MessageTagHandler{
	MessageProcessor*m_object;
public:
	void setObject(MessageProcessor*object);
	void call(Message*message);
};

#endif
