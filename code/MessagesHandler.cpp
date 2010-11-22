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
#include<assert.h>


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
 * using Iprobe, probe for new messages as they arrive
 * if no more message are available, return.
 * messages are kept in the inbox.
 */

void MessagesHandler::receiveMessages(StaticVector*inbox,RingAllocator*inboxAllocator,int destination){
	int flag;
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	while(flag){
		int sizeOfType=8;
		int tag=status.MPI_TAG;
		int source=status.MPI_SOURCE;
		int length;
		MPI_Get_count(&status,MPI_UNSIGNED_LONG_LONG,&length);
		void*incoming=(void*)inboxAllocator->allocate(length*sizeOfType);
		MPI_Recv(incoming,length,MPI_UNSIGNED_LONG_LONG,source,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		m_receivedMessages[source]++;
		Message aMessage(incoming,length,MPI_UNSIGNED_LONG_LONG,source,tag,source);
		inbox->push_back(aMessage);
		MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	}
}

MessagesHandler::MessagesHandler(){
}

void MessagesHandler::showStats(){
	cout<<"Rank "<<m_rank;
	for(int i=0;i<m_size;i++){
		cout<<" "<<m_receivedMessages[i];
	}
	cout<<endl;
}

void MessagesHandler::addCount(int rank,int count){
	m_receivedMessages[rank*m_size+m_allCounts[rank]]=count;
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
	FILE*f=fopen(file,"w+");
	for(int i=0;i<m_size;i++){
		fprintf(f,"%i",i);
		for(int j=0;j<m_size;i++){
			fprintf(f,"\t%lu",m_allReceivedMessages[i*m_size+j]);
		}
		fprintf(f,"\n");
	}
	fclose(f);
	cout<<"Rank "<<MASTER_RANK<<" wrote "<<file<<endl;
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
}

u64*MessagesHandler::getReceivedMessages(){
	return m_receivedMessages;
}
