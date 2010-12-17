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

#include<MessagesHandler.h>
#include<common_functions.h>
#include<fstream>
#include<assert.h>
using namespace std;

//#define DEBUG_MESSAGES

const char*__TAG_NAMES[]={
"TAG_WELCOME",
"TAG_SEND_SEQUENCE",
"TAG_SEQUENCES_READY",
"TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS",
"TAG_VERTICES_DATA",
"TAG_VERTICES_DISTRIBUTED",
"TAG_VERTEX_PTR_REQUEST",
"TAG_OUT_EDGE_DATA_WITH_PTR",
"TAG_OUT_EDGES_DATA",
"TAG_SHOW_VERTICES",
"TAG_START_VERTICES_DISTRIBUTION",
"TAG_EDGES_DISTRIBUTED",
"TAG_IN_EDGES_DATA",
"TAG_IN_EDGE_DATA_WITH_PTR",
"TAG_START_EDGES_DISTRIBUTION",
"TAG_START_EDGES_DISTRIBUTION_ASK",
"TAG_START_EDGES_DISTRIBUTION_ANSWER",
"TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION",
"TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER",
"TAG_PREPARE_COVERAGE_DISTRIBUTION",
"TAG_COVERAGE_DATA",
"TAG_COVERAGE_END",
"TAG_SEND_COVERAGE_VALUES",
"TAG_READY_TO_SEED",
"TAG_START_SEEDING",
"TAG_REQUEST_VERTEX_COVERAGE",
"TAG_REQUEST_VERTEX_COVERAGE_REPLY",
"TAG_REQUEST_VERTEX_KEY_AND_COVERAGE",
"TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY",
"TAG_REQUEST_VERTEX_OUTGOING_EDGES",
"TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY",
"TAG_SEEDING_IS_OVER",
"TAG_GOOD_JOB_SEE_YOU_SOON",
"TAG_I_GO_NOW",
"TAG_SET_WORD_SIZE",
"TAG_MASTER_IS_DONE_ATTACHING_READS",
"TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY",
"TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER",
"TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY",
"TAG_REQUEST_VERTEX_INGOING_EDGES",
"TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY",
"TAG_EXTENSION_IS_DONE",
"TAG_ASK_EXTENSION",
"TAG_ASK_IS_ASSEMBLED",
"TAG_ASK_REVERSE_COMPLEMENT",
"TAG_REQUEST_VERTEX_POINTER",
"TAG_ASK_IS_ASSEMBLED_REPLY",
"TAG_MARK_AS_ASSEMBLED",
"TAG_ASK_EXTENSION_DATA",
"TAG_EXTENSION_DATA",
"TAG_EXTENSION_END",
"TAG_EXTENSION_DATA_END",
"TAG_ATTACH_SEQUENCE",
"TAG_REQUEST_READS",
"TAG_REQUEST_READS_REPLY",
"TAG_ASK_READ_VERTEX_AT_POSITION",
"TAG_ASK_READ_VERTEX_AT_POSITION_REPLY",
"TAG_ASK_READ_LENGTH",
"TAG_ASK_READ_LENGTH_REPLY",
"TAG_SAVE_WAVE_PROGRESSION",
"TAG_COPY_DIRECTIONS",
"TAG_ASSEMBLE_WAVES",
"TAG_SAVE_WAVE_PROGRESSION_REVERSE",
"TAG_ASSEMBLE_WAVES_DONE",
"TAG_START_FUSION",
"TAG_FUSION_DONE",
"TAG_ASK_VERTEX_PATHS_SIZE",
"TAG_ASK_VERTEX_PATHS_SIZE_REPLY",
"TAG_GET_PATH_LENGTH",
"TAG_GET_PATH_LENGTH_REPLY",
"TAG_CALIBRATION_MESSAGE",
"TAG_BEGIN_CALIBRATION",
"TAG_END_CALIBRATION",
"TAG_COMMUNICATION_STABILITY_MESSAGE",
"TAG_ASK_VERTEX_PATH",
"TAG_ASK_VERTEX_PATH_REPLY",
"TAG_INDEX_PAIRED_SEQUENCE",
"TAG_HAS_PAIRED_READ",
"TAG_HAS_PAIRED_READ_REPLY",
"TAG_GET_PAIRED_READ",
"TAG_GET_PAIRED_READ_REPLY",
"TAG_CLEAR_DIRECTIONS",
"TAG_CLEAR_DIRECTIONS_REPLY",
"TAG_FINISH_FUSIONS",
"TAG_FINISH_FUSIONS_FINISHED",
"TAG_DISTRIBUTE_FUSIONS",
"TAG_DISTRIBUTE_FUSIONS_FINISHED",
"TAG_EXTENSION_START",
"TAG_ELIMINATE_PATH",
"TAG_GET_PATH_VERTEX",
"TAG_GET_PATH_VERTEX_REPLY",
"TAG_SET_COLOR_MODE",
"TAG_AUTOMATIC_DISTANCE_DETECTION",
"TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE",
"TAG_LIBRARY_DISTANCE",
"TAG_ASK_LIBRARY_DISTANCES",
"TAG_ASK_LIBRARY_DISTANCES_FINISHED",
"TAG_UPDATE_LIBRARY_INFORMATION",
"TAG_RECEIVED_COVERAGE_INFORMATION",
"TAG_REQUEST_READ_SEQUENCE",
"TAG_REQUEST_READ_SEQUENCE_REPLY",
"TAG_SEND_SEQUENCE_REPLY",
"TAG_SAVE_WAVE_PROGRESSION_REPLY",
"TAG_SEND_SEQUENCE_REGULATOR",
"TAG_START_INDEXING_SEQUENCES",
"TAG_VERTICES_DATA_REPLY",
"TAG_IN_EDGES_DATA_REPLY",
"TAG_OUT_EDGES_DATA_REPLY",
"TAG_INDEX_PAIRED_SEQUENCE_REPLY",
"TAG_EXTENSION_DATA_REPLY",
"TAG_BARRIER",
"TAG_SHOW_SEQUENCES",
"TAG_LIBRARY_DISTANCE_REPLY",
"TAG_RECEIVED_MESSAGES",
"TAG_RECEIVED_MESSAGES_REPLY",
"TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY",
"TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY",
"TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY",
"TAG_ASK_IS_ASSEMBLED_REPLY_END"
};




/*
 * send messages,
 */
void MessagesHandler::sendMessages(StaticVector*outbox,int source){
	for(int i=0;i<(int)outbox->size();i++){
		Message*aMessage=((*outbox)[i]);
		#ifdef ASSERT
		int destination=aMessage->getDestination();
		assert(destination>=0);
		#endif
		MPI_Request request;
		//  MPI_Issend
		//      Synchronous nonblocking. Note that a Wait/Test will complete only when the matching receive is posted
		#ifdef ASSERT
		assert(!(aMessage->getBuffer()==NULL && aMessage->getCount()>0));
		#endif
		#ifndef ASSERT
		MPI_Isend(aMessage->getBuffer(),aMessage->getCount(),aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(),MPI_COMM_WORLD,&request);
		#else
		int value=MPI_Isend(aMessage->getBuffer(),aMessage->getCount(),aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(),MPI_COMM_WORLD,&request);
		assert(value==MPI_SUCCESS);
		#endif
		
		MPI_Request_free(&request);

		#ifdef ASSERT
		assert(request==MPI_REQUEST_NULL);
		#endif
	}

	outbox->clear();
}



/*	
 * receiveMessages is implemented as recommanded by Mr. George Bosilca from
the University of Tennessee (via the Open-MPI mailing list)

De: George Bosilca <bosilca@…>
Reply-to: Open MPI Developers <devel@…>
À: Open MPI Developers <devel@…>
Sujet: Re: [OMPI devel] Simple program (103 lines) makes Open-1.4.3 hang
Date: 2010-11-23 18:03:04

If you know the max size of the receives I would take a different approach. 
Post few persistent receives, and manage them in a circular buffer. 
Instead of doing an MPI_Iprobe, use MPI_Test on the current head of your circular buffer. 
Once you use the data related to the receive, just do an MPI_Start on your request.
This approach will minimize the unexpected messages, and drain the connections faster. 
Moreover, at the end it is very easy to MPI_Cancel all the receives not yet matched.

    george. 
 */

void MessagesHandler::receiveMessages(StaticVector*inbox,RingAllocator*inboxAllocator,int destination){

	int flag;
	MPI_Status status;
	MPI_Test(m_ring+m_head,&flag,&status);

	if(flag){
		// get the length of the message
		// it is not necessary the same as the one posted with MPI_Recv_init
		// that one was a lower bound
		int tag=status.MPI_TAG;
		int source=status.MPI_SOURCE;
		int length;
		MPI_Get_count(&status,MPI_UINT64_T,&length);
		u64*filledBuffer=(u64*)m_buffers+m_head*MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(u64);

		// copy it in a safe buffer
		u64*incoming=(u64*)inboxAllocator->allocate(length*sizeof(u64));
		for(int i=0;i<length;i++){
			incoming[i]=filledBuffer[i];
		}

		// the request can start again
		MPI_Start(m_ring+m_head);
	
		// add the message in the inbox
		Message aMessage(incoming,length,MPI_UINT64_T,source,tag,source);
		inbox->push_back(aMessage);
		m_receivedMessages[source]++;
		
		#ifdef DEBUG_MESSAGES
		m_buckets[source][tag]++;

		if(m_rank==0 && source==0){
			if(tag==TAG_REQUEST_VERTEX_INGOING_EDGES){
				cout<<"Self="<<m_rank<<" Source="<<source<<" Tag="<<__TAG_NAMES[tag]<<" Vertex="<<incoming[0]<<endl;
			}else if(tag==TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY){
				cout<<"Self="<<m_rank<<" Source="<<source<<" Tag="<<__TAG_NAMES[tag]<<endl;
			}
		}
		#endif

		// increment the head
		m_head++;
		if(m_head==m_ringSize){
			m_head=0;
		}
	}
}

void MessagesHandler::showStats(){
	if(m_rank!=0){
		return;
	}
	cout<<"self="<<m_rank<<endl;
	for(map<int,map<int,int> >::iterator i=m_buckets.begin();
		i!=m_buckets.end();i++){
		int source=i->first;
		cout<<" source="<<source<<endl;
		for(map<int,int>::iterator j=i->second.begin();j!=i->second.end();j++){
			int tag=j->first;
			int count=j->second;
			cout<<__TAG_NAMES[tag]<<" "<<count<<endl;
		}
	}
	cout<<endl;
}

void MessagesHandler::addCount(int rank,u64 count){
	m_allReceivedMessages[rank*m_size+m_allCounts[rank]]=count;
	m_allCounts[rank]++;
}

bool MessagesHandler::isFinished(int rank){
	return m_allCounts[rank]==m_size;
}

bool MessagesHandler::isFinished(){
	for(int i=0;i<m_size;i++){
		if(!isFinished(i)){
			return false;
		}
	}

	// update the counts for root, because it was updated.
	for(int i=0;i<m_size;i++){
		m_allCounts[MASTER_RANK*m_size+i]=m_receivedMessages[i];
	}

	return true;
}

void MessagesHandler::writeStats(const char*file){
	ofstream f(file);
	
	for(int i=0;i<m_size;i++){
		f<<"\t"<<i;
	}
	f<<endl;

	for(int i=0;i<m_size;i++){
		f<<i;
		for(int j=0;j<m_size;j++){
			f<<"\t"<<m_allReceivedMessages[i*m_size+j];
		}
		f<<endl;
	}
	f.close();
}

void MessagesHandler::constructor(int rank,int size){
	m_rank=rank;
	m_size=size;
	m_receivedMessages=(u64*)__Malloc(sizeof(u64)*m_size);
	if(rank==MASTER_RANK){
		m_allReceivedMessages=(u64*)__Malloc(sizeof(u64)*m_size*m_size);
		m_allCounts=(int*)__Malloc(sizeof(int)*m_size);
	}

	for(int i=0;i<m_size;i++){
		m_receivedMessages[i]=0;
		if(rank==MASTER_RANK){
			m_allCounts[i]=0;
		}
	}

	// the ring contains 128 elements.
	m_ringSize=NUMBER_OF_PERSISTENT_REQUESTS_IN_RING;
	m_ring=(MPI_Request*)__Malloc(sizeof(MPI_Request)*m_ringSize);
	m_buffers=(char*)__Malloc(MAXIMUM_MESSAGE_SIZE_IN_BYTES*m_ringSize);
	m_head=0;

	// post a few receives.
	for(int i=0;i<m_ringSize;i++){
		void*buffer=m_buffers+i*MAXIMUM_MESSAGE_SIZE_IN_BYTES;
		MPI_Recv_init(buffer,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),MPI_UINT64_T,
			MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,m_ring+i);
		MPI_Start(m_ring+i);
	}
}

u64*MessagesHandler::getReceivedMessages(){
	return m_receivedMessages;
}

void MessagesHandler::freeLeftovers(){
	#ifdef DEBUG_MESSAGES
	showStats();
	#endif
	for(int i=0;i<m_ringSize;i++){
		MPI_Cancel(m_ring+i);
		MPI_Request_free(m_ring+i);
	}
	__Free(m_ring);
	__Free(m_buffers);
}
