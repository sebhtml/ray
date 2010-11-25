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
		MPI_Get_count(&status,MPI_UNSIGNED_LONG_LONG,&length);
		u64*filledBuffer=(u64*)m_buffers+m_head*MPI_BTL_SM_EAGER_LIMIT/sizeof(u64);

		// copy it in a safe buffer
		u64*incoming=(u64*)inboxAllocator->allocate(length*sizeof(u64));
		for(int i=0;i<length;i++){
			incoming[i]=filledBuffer[i];
		}

		// the request can start again
		MPI_Start(m_ring+m_head);
	
		// add the message in the inbox
		Message aMessage(incoming,length,MPI_UNSIGNED_LONG_LONG,source,tag,source);
		inbox->push_back(aMessage);
		m_receivedMessages[source]++;

		// increment the head
		m_head++;
		if(m_head==m_ringSize){
			m_head=0;
		}
	}
}

void MessagesHandler::showStats(){
	cout<<"Rank "<<m_rank;
	for(int i=0;i<m_size;i++){
		cout<<" "<<m_receivedMessages[i];
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
	cout<<endl;

	for(int i=0;i<m_size;i++){
		cout<<i;
		for(int j=0;j<m_size;j++){
			cout<<"\t"<<m_allReceivedMessages[i*m_size+j];
		}
		cout<<endl;
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
	m_ringSize=128;
	m_ring=(MPI_Request*)__Malloc(sizeof(MPI_Request)*m_ringSize);
	m_buffers=(char*)__Malloc(MPI_BTL_SM_EAGER_LIMIT*m_ringSize);
	m_head=0;

	// post a few receives.
	for(int i=0;i<m_ringSize;i++){
		void*buffer=m_buffers+i*MPI_BTL_SM_EAGER_LIMIT;
		MPI_Recv_init(buffer,MPI_BTL_SM_EAGER_LIMIT/sizeof(VERTEX_TYPE),MPI_UNSIGNED_LONG_LONG,
			MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,m_ring+i);
		MPI_Start(m_ring+i);
	}
}

u64*MessagesHandler::getReceivedMessages(){
	return m_receivedMessages;
}

void MessagesHandler::freeLeftovers(){
	for(int i=0;i<m_ringSize;i++){
		MPI_Cancel(m_ring+i);
		MPI_Request_free(m_ring+i);
	}
	__Free(m_ring);
	__Free(m_buffers);
}
