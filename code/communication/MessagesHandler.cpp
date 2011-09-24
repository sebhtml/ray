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

#include <core/constants.h>
#include <communication/MessagesHandler.h>
#include <core/common_functions.h>
#include <fstream>
#include <assert.h>
#include <memory/malloc_types.h>
#include <iostream>
#include <sstream>
#include <string.h>
using namespace std;


/*
 * send messages,
 */
void MessagesHandler::sendMessages(StaticVector*outbox,int source){
	for(int i=0;i<(int)outbox->size();i++){
		Message*aMessage=((*outbox)[i]);
		int destination=aMessage->getDestination();
		void*buffer=aMessage->getBuffer();
		int count=aMessage->getCount();
		int tag=aMessage->getTag();

		#ifdef ASSERT
		assert(destination>=0);
		if(destination>=m_size){
			cout<<"Tag="<<tag<<" Destination="<<destination<<endl;
		}
		assert(destination<m_size);
		assert(!(buffer==NULL && count>0));
		assert(count<=(int)(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)));
		#endif

		MPI_Request request;

		//  MPI_Issend
		//      Synchronous nonblocking. 
		//      Note that a Wait/Test will complete only when the matching receive is posted
		MPI_Isend(buffer,count,m_datatype,destination,tag,MPI_COMM_WORLD,&request);
		MPI_Request_free(&request);

		#ifdef ASSERT
		assert(request==MPI_REQUEST_NULL);
		#endif

		/** update statistics */
		m_messageStatistics[destination*RAY_MPI_TAG_DUMMY+tag]++;
		m_sentMessages++;
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
	/* persistent communication is not enabled by default */
	#ifdef USE_MPI_PERSISTENT_COMMUNICATION
	int flag;
	MPI_Status status;
	MPI_Test(m_ring+m_head,&flag,&status);

	if(flag){
		// get the length of the message
		// it is not necessary the same as the one posted with MPI_Recv_init
		// that one was a lower bound
		int tag=status.MPI_TAG;
		int source=status.MPI_SOURCE;
		int count;
		MPI_Get_count(&status,m_datatype,&count);
		uint64_t*filledBuffer=(uint64_t*)m_buffers+m_head*MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);

		// copy it in a safe buffer
		uint64_t*incoming=(uint64_t*)inboxAllocator->allocate(count*sizeof(uint64_t));

		memcpy(incoming,filledBuffer,count*sizeof(uint64_t));

		// the request can start again
		MPI_Start(m_ring+m_head);
	
		// add the message in the inbox
		Message aMessage(incoming,count,destination,tag,source);
		inbox->push_back(aMessage);

		m_head++;
		m_head%=m_ringSize;

		/** update statistics */
		m_receivedMessages++;
	}
	#else

	/* use MPI_Iprobe */

	/* if there are urgent messages, read them first ! */
	if(m_urgentMessages.size() > 0){
		for(set<uint64_t>::iterator i=m_urgentMessages.begin();i!=m_urgentMessages.end();i++){
			uint64_t code=*i;
			int rank=0;
			int tag=0;
			decodeUrgentMessage(code,&tag,&rank);

			probeAndRead(rank,tag,inbox,inboxAllocator,destination);

			/* we have read something ! */
			if(inbox->size() > 0){
				/* this message is not urgent anymore */
				m_urgentMessages.erase(code);

				//cout<<"Got urgent message "<<MESSAGES[tag]<<" rank "<<rank<<endl;

	 			/* we are happy with this message */
				/* we won't read anything else for now */
				return;
			}
		}
	}

	/* since we have not read anything successfully, 
 * 		now we try to read any message */
	probeAndRead(MPI_ANY_SOURCE,MPI_ANY_TAG,inbox,inboxAllocator,destination);

	/* this message was maybe in the list of urgent messages */
	if(inbox->size() > 0){
		uint64_t code=encodeUrgentMessage(inbox->at(0)->getTag(),inbox->at(0)->getSource());
		m_urgentMessages.erase(code);
	}

	#endif
}

void MessagesHandler::probeAndRead(int source,int tag,StaticVector*inbox,RingAllocator*inboxAllocator,int destination){
	int flag;
	MPI_Status status;
	MPI_Iprobe(source,tag,MPI_COMM_WORLD,&flag,&status);

	/* read at most one message */
	if(flag){
		MPI_Datatype datatype=MPI_UNSIGNED_LONG_LONG;
		int tag=status.MPI_TAG;
		int source=status.MPI_SOURCE;
		int count=-1;
		MPI_Get_count(&status,datatype,&count);
	
		#ifdef ASSERT
		assert(count >= 0);
		#endif
	
		uint64_t*incoming=NULL;
		if(count > 0){
			incoming=(uint64_t*)inboxAllocator->allocate(count*sizeof(uint64_t));
		}

		MPI_Recv(incoming,count,datatype,source,tag,MPI_COMM_WORLD,&status);

		Message aMessage(incoming,count,destination,tag,source);
		inbox->push_back(aMessage);
	}
}

void MessagesHandler::initialiseMembers(){
	#ifdef USE_MPI_PERSISTENT_COMMUNICATION
	// the ring itself  contain requests ready to receive messages
	m_ringSize=m_size+16;

	m_ring=(MPI_Request*)__Malloc(sizeof(MPI_Request)*m_ringSize,RAY_MALLOC_TYPE_PERSISTENT_MESSAGE_RING,false);
	m_buffers=(uint8_t*)__Malloc(MAXIMUM_MESSAGE_SIZE_IN_BYTES*m_ringSize,RAY_MALLOC_TYPE_PERSISTENT_MESSAGE_BUFFERS,false);
	m_head=0;

	// post a few receives.
	for(int i=0;i<m_ringSize;i++){
		uint8_t*buffer=m_buffers+i*MAXIMUM_MESSAGE_SIZE_IN_BYTES;
		MPI_Recv_init(buffer,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),m_datatype,
			MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,m_ring+i);
		MPI_Start(m_ring+i);
	}
	#endif
}

void MessagesHandler::freeLeftovers(){
	#ifdef USE_MPI_PERSISTENT_COMMUNICATION
	for(int i=0;i<m_ringSize;i++){
		MPI_Cancel(m_ring+i);
		MPI_Request_free(m_ring+i);
	}
	__Free(m_ring,RAY_MALLOC_TYPE_PERSISTENT_MESSAGE_RING,false);
	m_ring=NULL;
	__Free(m_buffers,RAY_MALLOC_TYPE_PERSISTENT_MESSAGE_BUFFERS,false);
	m_buffers=NULL;
	__Free(m_messageStatistics,RAY_MALLOC_TYPE_MESSAGE_STATISTICS,false);
	#endif

	m_messageStatistics=NULL;
}

void MessagesHandler::constructor(int*argc,char***argv){
	m_sentMessages=0;
	m_receivedMessages=0;
	m_datatype=MPI_UNSIGNED_LONG_LONG;
	MPI_Init(argc,argv);
	char serverName[1000];
	int len;

	/** initialize the message passing interface stack */
	MPI_Get_processor_name(serverName,&len);
	MPI_Comm_rank(MPI_COMM_WORLD,&m_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&m_size);
	initialiseMembers();
	m_processorName=serverName;

	/** initialize message statistics to 0 */
	m_messageStatistics=(uint64_t*)__Malloc(RAY_MPI_TAG_DUMMY*m_size*sizeof(uint64_t),RAY_MALLOC_TYPE_MESSAGE_STATISTICS,false);
	for(int rank=0;rank<m_size;rank++){
		for(int tag=0;tag<RAY_MPI_TAG_DUMMY;tag++){
			m_messageStatistics[rank*RAY_MPI_TAG_DUMMY+tag]=0;
		}
	}
}

void MessagesHandler::destructor(){
	MPI_Finalize();
}

string*MessagesHandler::getName(){
	return &m_processorName;
}

int MessagesHandler::getRank(){
	return m_rank;
}

int MessagesHandler::getSize(){
	return m_size;
}

void MessagesHandler::barrier(){
	MPI_Barrier(MPI_COMM_WORLD);
}

void MessagesHandler::version(int*a,int*b){
	MPI_Get_version(a,b);
}

void MessagesHandler::appendStatistics(const char*file){
	/** add an entry for RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY
 * 	because this message is sent after writting the current file
 */
	m_messageStatistics[MASTER_RANK*RAY_MPI_TAG_DUMMY+RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY]++;
	m_sentMessages++;

	ofstream fp;
	fp.open(file,ios_base::out|ios_base::app);
	for(int destination=0;destination<m_size;destination++){
		for(int tag=0;tag<RAY_MPI_TAG_DUMMY;tag++){
			uint64_t count=m_messageStatistics[destination*RAY_MPI_TAG_DUMMY+tag];
			fp<<m_rank<<"\t"<<destination<<"\t"<<MESSAGES[tag]<<"\t"<<count<<"\n";
		}
	}
	fp.close();
	cout<<"Rank "<<m_rank<<": sent "<<m_sentMessages<<" messages, received "<<m_receivedMessages<<" messages."<<endl;
}

string MessagesHandler::getMessagePassingInterfaceImplementation(){
	ostringstream implementation;

	#ifdef MPICH2
        implementation<<"MPICH2 (MPICH2) "<<MPICH2_VERSION;
	#endif

	#ifdef OMPI_MPI_H
        implementation<<"Open-MPI (OMPI_MPI_H) "<<OMPI_MAJOR_VERSION<<"."<<OMPI_MINOR_VERSION<<"."<<OMPI_RELEASE_VERSION;
	#endif

	if(implementation.str().length()==0)
		implementation<<"Unknown";
	return implementation.str();
}


void MessagesHandler::addUrgentMessage(int tag,int rank){
	#ifdef USE_MPI_PERSISTENT_COMMUNICATION
	/* do nothing */
	#else
	//cout<<"Adding urgent: "<<MESSAGES[tag]<<" for rank "<<rank<<endl;

	m_urgentMessages.insert(encodeUrgentMessage(tag,rank));

	if(m_urgentMessages.size() > MAX_ALLOCATED_OUTPUT_BUFFERS){
		cout<<"Warning, "<<m_urgentMessages.size()<<" urgent messages pending for reception."<<endl;

		for(set<uint64_t>::iterator i=m_urgentMessages.begin();i!=m_urgentMessages.end();i++){
			uint64_t code=*i;
			int rank=0;
			int tag=0;
			decodeUrgentMessage(code,&tag,&rank);
			
			cout<<"- "<<MESSAGES[tag]<<" with rank "<<rank<<endl;
		}
	}

	//cout<<"Urgent: "<<m_urgentMessages.size()<<endl;
	#endif
}

void MessagesHandler::decodeUrgentMessage(uint64_t code,int*tag,int*rank){
	(*tag)=code / MAX_NUMBER_OF_MPI_PROCESSES;
	(*rank) = code % MAX_NUMBER_OF_MPI_PROCESSES;
}

uint64_t MessagesHandler::encodeUrgentMessage(int tag,int rank){
	/* convert to 64 bits just to be  sure */
	uint64_t a=tag;

	return a*MAX_NUMBER_OF_MPI_PROCESSES + rank;
}

