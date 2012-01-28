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

#include <communication/MessageProcessor_adapters.h>
#include <communication/MessageProcessor.h>

void Adapter_RAY_MPI_TAG_LOAD_SEQUENCES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_LOAD_SEQUENCES::call(Message*message){
	m_object->call_RAY_MPI_TAG_LOAD_SEQUENCES(message);
}


void Adapter_RAY_MPI_TAG_CONTIG_INFO::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_CONTIG_INFO::call(Message*message){
	m_object->call_RAY_MPI_TAG_CONTIG_INFO(message);
}


void Adapter_RAY_MPI_TAG_SCAFFOLDING_LINKS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SCAFFOLDING_LINKS::call(Message*message){
	m_object->call_RAY_MPI_TAG_SCAFFOLDING_LINKS(message);
}


void Adapter_RAY_MPI_TAG_GET_READ_MARKERS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_READ_MARKERS::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_READ_MARKERS(message);
}


void Adapter_RAY_MPI_TAG_GET_READ_MATE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_READ_MATE::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_READ_MATE(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_READS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_READS::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_READS(message);
}


void Adapter_RAY_MPI_TAG_SET_WORD_SIZE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SET_WORD_SIZE::call(Message*message){
	m_object->call_RAY_MPI_TAG_SET_WORD_SIZE(message);
}


void Adapter_RAY_MPI_TAG_VERTEX_READS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_VERTEX_READS::call(Message*message){
	m_object->call_RAY_MPI_TAG_VERTEX_READS(message);
}


void Adapter_RAY_MPI_TAG_VERTEX_INFO::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_VERTEX_INFO::call(Message*message){
	m_object->call_RAY_MPI_TAG_VERTEX_INFO(message);
}


void Adapter_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT(message);
}


void Adapter_RAY_MPI_TAG_VERTEX_READS_FROM_LIST::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_VERTEX_READS_FROM_LIST::call(Message*message){
	m_object->call_RAY_MPI_TAG_VERTEX_READS_FROM_LIST(message);
}


void Adapter_RAY_MPI_TAG_START_INDEXING_SEQUENCES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_START_INDEXING_SEQUENCES::call(Message*message){
	m_object->call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(message);
}


void Adapter_RAY_MPI_TAG_SEQUENCES_READY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SEQUENCES_READY::call(Message*message){
	m_object->call_RAY_MPI_TAG_SEQUENCES_READY(message);
}


void Adapter_RAY_MPI_TAG_VERTICES_DATA::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_VERTICES_DATA::call(Message*message){
	m_object->call_RAY_MPI_TAG_VERTICES_DATA(message);
}


void Adapter_RAY_MPI_TAG_VERTICES_DATA_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_VERTICES_DATA_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_VERTICES_DATA_REPLY(message);
}


void Adapter_RAY_MPI_TAG_PURGE_NULL_EDGES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_PURGE_NULL_EDGES::call(Message*message){
	m_object->call_RAY_MPI_TAG_PURGE_NULL_EDGES(message);
}


void Adapter_RAY_MPI_TAG_VERTICES_DISTRIBUTED::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_VERTICES_DISTRIBUTED::call(Message*message){
	m_object->call_RAY_MPI_TAG_VERTICES_DISTRIBUTED(message);
}


void Adapter_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY(message);
}


void Adapter_RAY_MPI_TAG_OUT_EDGES_DATA::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_OUT_EDGES_DATA::call(Message*message){
	m_object->call_RAY_MPI_TAG_OUT_EDGES_DATA(message);
}


void Adapter_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION::call(Message*message){
	m_object->call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION(message);
}


void Adapter_RAY_MPI_TAG_IN_EDGES_DATA_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_IN_EDGES_DATA_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY(message);
}


void Adapter_RAY_MPI_TAG_IN_EDGES_DATA::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_IN_EDGES_DATA::call(Message*message){
	m_object->call_RAY_MPI_TAG_IN_EDGES_DATA(message);
}


void Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION::call(Message*message){
	m_object->call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(message);
}


void Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER::call(Message*message){
	m_object->call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER(message);
}


void Adapter_RAY_MPI_TAG_TEST_NETWORK_MESSAGE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_TEST_NETWORK_MESSAGE::call(Message*message){
	m_object->call_RAY_MPI_TAG_TEST_NETWORK_MESSAGE(message);
}


void Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION::call(Message*message){
	m_object->call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION(message);
}


void Adapter_RAY_MPI_TAG_COVERAGE_DATA::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_COVERAGE_DATA::call(Message*message){
	m_object->call_RAY_MPI_TAG_COVERAGE_DATA(message);
}


void Adapter_RAY_MPI_TAG_COVERAGE_END::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_COVERAGE_END::call(Message*message){
	m_object->call_RAY_MPI_TAG_COVERAGE_END(message);
}


void Adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES::call(Message*message){
	m_object->call_RAY_MPI_TAG_SEND_COVERAGE_VALUES(message);
}


void Adapter_RAY_MPI_TAG_READY_TO_SEED::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_READY_TO_SEED::call(Message*message){
	m_object->call_RAY_MPI_TAG_READY_TO_SEED(message);
}


void Adapter_RAY_MPI_TAG_START_SEEDING::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_START_SEEDING::call(Message*message){
	m_object->call_RAY_MPI_TAG_START_SEEDING(message);
}


void Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_MARK::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_MARK::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_COVERAGE_AND_MARK(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY(message);
}


void Adapter_RAY_MPI_TAG_SEEDING_IS_OVER::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SEEDING_IS_OVER::call(Message*message){
	m_object->call_RAY_MPI_TAG_SEEDING_IS_OVER(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_SEED_LENGTHS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_SEED_LENGTHS::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_SEED_LENGTHS(message);
}


void Adapter_RAY_MPI_TAG_SEND_SEED_LENGTHS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SEND_SEED_LENGTHS::call(Message*message){
	m_object->call_RAY_MPI_TAG_SEND_SEED_LENGTHS(message);
}


void Adapter_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS::call(Message*message){
	m_object->call_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS(message);
}


void Adapter_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER::call(Message*message){
	m_object->call_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER(message);
}


void Adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS::call(Message*message){
	m_object->call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS(message);
}


void Adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY(message);
}


void Adapter_RAY_MPI_TAG_EXTENSION_IS_DONE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_EXTENSION_IS_DONE::call(Message*message){
	m_object->call_RAY_MPI_TAG_EXTENSION_IS_DONE(message);
}


void Adapter_RAY_MPI_TAG_ASK_EXTENSION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_EXTENSION::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_EXTENSION(message);
}


void Adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_IS_ASSEMBLED(message);
}


void Adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY(message);
}


void Adapter_RAY_MPI_TAG_ASK_EXTENSION_DATA::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_EXTENSION_DATA::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_EXTENSION_DATA(message);
}


void Adapter_RAY_MPI_TAG_EXTENSION_DATA_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_EXTENSION_DATA_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_EXTENSION_DATA_REPLY(message);
}


void Adapter_RAY_MPI_TAG_EXTENSION_DATA::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_EXTENSION_DATA::call(Message*message){
	m_object->call_RAY_MPI_TAG_EXTENSION_DATA(message);
}


void Adapter_RAY_MPI_TAG_EXTENSION_DATA_END::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_EXTENSION_DATA_END::call(Message*message){
	m_object->call_RAY_MPI_TAG_EXTENSION_DATA_END(message);
}


void Adapter_RAY_MPI_TAG_ATTACH_SEQUENCE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ATTACH_SEQUENCE::call(Message*message){
	m_object->call_RAY_MPI_TAG_ATTACH_SEQUENCE(message);
}


void Adapter_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY(message);
}


void Adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION(message);
}


void Adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY(message);
}


void Adapter_RAY_MPI_TAG_ASK_READ_LENGTH::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_READ_LENGTH::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_READ_LENGTH(message);
}


void Adapter_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY(message);
}


void Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY(message);
}


void Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION::call(Message*message){
	m_object->call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION(message);
}


void Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY(message);
}


void Adapter_RAY_MPI_TAG_ASSEMBLE_WAVES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASSEMBLE_WAVES::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASSEMBLE_WAVES(message);
}


void Adapter_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE(message);
}


void Adapter_RAY_MPI_TAG_START_FUSION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_START_FUSION::call(Message*message){
	m_object->call_RAY_MPI_TAG_START_FUSION(message);
}


void Adapter_RAY_MPI_TAG_FUSION_DONE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_FUSION_DONE::call(Message*message){
	m_object->call_RAY_MPI_TAG_FUSION_DONE(message);
}


void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE(message);
}


void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY(message);
}


void Adapter_RAY_MPI_TAG_GET_PATH_LENGTH::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_PATH_LENGTH::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_PATH_LENGTH(message);
}


void Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION(message);
}


void Adapter_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY(message);
}


void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_VERTEX_PATHS(message);
}


void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY(message);
}


void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END(message);
}


void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATH::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATH::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_VERTEX_PATH(message);
}


void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY(message);
}


void Adapter_RAY_MPI_TAG_HAS_PAIRED_READ::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_HAS_PAIRED_READ::call(Message*message){
	m_object->call_RAY_MPI_TAG_HAS_PAIRED_READ(message);
}


void Adapter_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY(message);
}


void Adapter_RAY_MPI_TAG_GET_PAIRED_READ::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_PAIRED_READ::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_PAIRED_READ(message);
}


void Adapter_RAY_MPI_TAG_GET_PAIRED_READ_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_PAIRED_READ_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_PAIRED_READ_REPLY(message);
}


void Adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS::call(Message*message){
	m_object->call_RAY_MPI_TAG_CLEAR_DIRECTIONS(message);
}


void Adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY(message);
}


void Adapter_RAY_MPI_TAG_FINISH_FUSIONS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_FINISH_FUSIONS::call(Message*message){
	m_object->call_RAY_MPI_TAG_FINISH_FUSIONS(message);
}


void Adapter_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED::call(Message*message){
	m_object->call_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED(message);
}


void Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS::call(Message*message){
	m_object->call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS(message);
}


void Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY(message);
}


void Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED::call(Message*message){
	m_object->call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED(message);
}


void Adapter_RAY_MPI_TAG_EXTENSION_START::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_EXTENSION_START::call(Message*message){
	m_object->call_RAY_MPI_TAG_EXTENSION_START(message);
}


void Adapter_RAY_MPI_TAG_ELIMINATE_PATH::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ELIMINATE_PATH::call(Message*message){
	m_object->call_RAY_MPI_TAG_ELIMINATE_PATH(message);
}


void Adapter_RAY_MPI_TAG_GET_PATH_VERTEX::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_PATH_VERTEX::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_PATH_VERTEX(message);
}


void Adapter_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY(message);
}


void Adapter_RAY_MPI_TAG_WRITE_AMOS::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_WRITE_AMOS::call(Message*message){
	m_object->call_RAY_MPI_TAG_WRITE_AMOS(message);
}


void Adapter_RAY_MPI_TAG_WRITE_AMOS_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_WRITE_AMOS_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_WRITE_AMOS_REPLY(message);
}


void Adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION::call(Message*message){
	m_object->call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION(message);
}


void Adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE::call(Message*message){
	m_object->call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE(message);
}


void Adapter_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY(message);
}


void Adapter_RAY_MPI_TAG_LIBRARY_DISTANCE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_LIBRARY_DISTANCE::call(Message*message){
	m_object->call_RAY_MPI_TAG_LIBRARY_DISTANCE(message);
}


void Adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES(message);
}


void Adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED::call(Message*message){
	m_object->call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED(message);
}


void Adapter_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION::call(Message*message){
	m_object->call_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION(message);
}


void Adapter_RAY_MPI_TAG_KMER_ACADEMY_DATA::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_KMER_ACADEMY_DATA::call(Message*message){
	m_object->call_RAY_MPI_TAG_KMER_ACADEMY_DATA(message);
}


void Adapter_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED::call(Message*message){
	m_object->call_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED(message);
}


void Adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE(message);
}


void Adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY::call(Message*message){
	m_object->call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY(message);
}


void Adapter_RAY_MPI_TAG_I_FINISHED_SCAFFOLDING::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_I_FINISHED_SCAFFOLDING::call(Message*message){
	m_object->call_RAY_MPI_TAG_I_FINISHED_SCAFFOLDING(message);
}


void Adapter_RAY_MPI_TAG_GET_CONTIG_CHUNK::setObject(MessageProcessor*object){
	m_object=object;
}

void Adapter_RAY_MPI_TAG_GET_CONTIG_CHUNK::call(Message*message){
	m_object->call_RAY_MPI_TAG_GET_CONTIG_CHUNK(message);
}




