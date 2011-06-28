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

		#ifdef SHOW_TAGS
		if(source==0){
			printf("SEND\tSource\t%i\tDestination\t%i\tCount\t%i\tTag\t%s\n",source,destination,count,MESSAGES[tag]);
			fflush(stdout);
		}
		#endif

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
		int count;
		MPI_Get_count(&status,m_datatype,&count);
		uint64_t*filledBuffer=(uint64_t*)m_buffers+m_head*MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);

		// copy it in a safe buffer
		uint64_t*incoming=(uint64_t*)inboxAllocator->allocate(count*sizeof(uint64_t));

		memcpy(incoming,filledBuffer,count*sizeof(uint64_t));

		// the request can start again
		MPI_Start(m_ring+m_head);
	
		// add the message in the inbox
		Message aMessage(incoming,count,source,tag,source);
		inbox->push_back(aMessage);

		#ifdef SHOW_TAGS
		if(destination==0){
			printf("RECV\tSource\t%i\tDestination\t%i\tCount\t%i\tTag\t%s\n",source,destination,count,MESSAGES[tag]);
			fflush(stdout);
		}
		#endif
	
		#ifdef COUNT_MESSAGES
		m_receivedMessages[source]++;
		#endif
		
		#ifdef DEBUG_MESSAGES
		m_buckets[source][tag]++;
		#endif

		// increment the head
		if(m_head==m_ringSize-1){
			m_head=0;
		}else{
			m_head++;
		}
	}
}

#ifdef COUNT_MESSAGES
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
			cout<<MESSAGES[tag]<<" "<<count<<endl;
		}
	}
	cout<<endl;
}
#endif

#ifdef COUNT_MESSAGES
void MessagesHandler::addCount(int rank,uint64_t count){
	int source=m_allCounts[rank];
	int destination=rank;
	m_allReceivedMessages[destination*m_size+source]=count;
	m_allCounts[rank]++;
}
#endif

#ifdef COUNT_MESSAGES
bool MessagesHandler::isFinished(int rank){
	return m_allCounts[rank]==m_size;
}
#endif

#ifdef COUNT_MESSAGES
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
#endif

#ifdef COUNT_MESSAGES
void MessagesHandler::writeStats(const char*file){
	ofstream f(file);
	
	for(int i=0;i<m_size;i++){
		f<<"\t"<<i;
	}
	f<<endl;

	for(int destination=0;destination<m_size;destination++){
		f<<destination;
		for(int source=0;source<m_size;source++){
			f<<"\t"<<m_allReceivedMessages[destination*m_size+source];
		}
		f<<endl;
	}
	f.close();
}
#endif

void MessagesHandler::initialiseMembers(){
	#ifdef COUNT_MESSAGES
	m_receivedMessages=(uint64_t*)__Malloc(sizeof(uint64_t)*m_size);
	if(rank==MASTER_RANK){
		m_allReceivedMessages=(uint64_t*)__Malloc(sizeof(uint64_t)*m_size*m_size);
		m_allCounts=(int*)__Malloc(sizeof(int)*m_size);
	}

	for(int i=0;i<m_size;i++){
		m_receivedMessages[i]=0;
		if(rank==MASTER_RANK){
			m_allCounts[i]=0;
		}
	}
	#endif

	// the ring itself  contain requests ready to receive messages
	m_ringSize=128;

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
}

#ifdef COUNT_MESSAGES
uint64_t*MessagesHandler::getReceivedMessages(){
	return m_receivedMessages;
}
#endif

void MessagesHandler::freeLeftovers(){
	#ifdef DEBUG_MESSAGES
	showStats();
	#endif
	for(int i=0;i<m_ringSize;i++){
		MPI_Cancel(m_ring+i);
		MPI_Request_free(m_ring+i);
	}
	__Free(m_ring,RAY_MALLOC_TYPE_PERSISTENT_MESSAGE_RING,false);
	__Free(m_buffers,RAY_MALLOC_TYPE_PERSISTENT_MESSAGE_BUFFERS,false);
}

void MessagesHandler::constructor(int*argc,char***argv){
	m_datatype=MPI_UNSIGNED_LONG_LONG;
	MPI_Init(argc,argv);
	char serverName[1000];
	int len;
	MPI_Get_processor_name(serverName,&len);
	MPI_Comm_rank(MPI_COMM_WORLD,&m_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&m_size);
	initialiseMembers();
	m_processorName=serverName;
}

void MessagesHandler::destructor(){
	MPI_Finalize();
}

string MessagesHandler::getName(){
	return m_processorName;
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
