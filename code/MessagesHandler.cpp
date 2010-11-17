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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#include<MessagesHandler.h>
#include<common_functions.h>

/*
 * send messages,
 * if the message goes to self, do a memcpy!
 */
void MessagesHandler::sendMessages(vector<Message>*outbox,MyAllocator*outboxAllocator){
	if(outbox->size()==0){
		return;
	}

	for(int i=0;i<(int)outbox->size();i++){
		Message*aMessage=&((*m_outbox)[i]);
		#ifdef DEBUG
		int theRank=aMessage->getDestination();
		assert(theRank>=0);
		assert(theRank<getSize());
		#endif

		MPI_Request request;
		MPI_Isend(aMessage->getBuffer(), aMessage->getCount(), aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(), MPI_COMM_WORLD,&request);
		addRequest(&request);
	}

	outbox->clear();
	freeRequests(outboxAllocator);
	outboxAllocator->reset();
}

// O(1) add
void MessagesHandler::addRequest(MPI_Request*request){
	Request*newRequest=(Request*)__Malloc(sizeof(Request));
	newRequest->m_mpiRequest=request;
	newRequest->m_next=m_root;
	m_root=newRequest;
}

// O(n) freeing
void MessagesHandler::freeRequests(){
	Request*previous=NULL;
	Request*current=m_root;
	while(current!=NULL){
		MPI_Request*request=current->m_mpiRequest;
		MPI_Status status;
		int flag;
		MPI_Test(request,&flag,&status);
		Request*next=current->m_next;
		if(flag){
			MPI_Request_free(request);
			__Free(current);
			previous->m_next=next;
		}
		current=next;
	}
}

/*	
 * using Iprobe, probe for new messages as they arrive
 * if no more message are available, return.
 * messages are kept in the inbox.
 */

void MessagesHandler::receiveMessages(vector<Message>*inbox,MyAllocator*inboxAllocator){
	int flag;
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	while(flag){
		MPI_Datatype datatype=MPI_UNSIGNED_LONG_LONG;
		int sizeOfType=8;
		int tag=status.MPI_TAG;
		if(tag==TAG_SEND_SEQUENCE || tag==TAG_REQUEST_READ_SEQUENCE_REPLY){
			datatype=MPI_BYTE;
			sizeOfType=1;
		}
		int source=status.MPI_SOURCE;
		int length;
		MPI_Get_count(&status,datatype,&length);
		void*incoming=(void*)inboxAllocator->allocate(length*sizeOfType);
		MPI_Status status2;
		MPI_Recv(incoming,length,datatype,source,tag,MPI_COMM_WORLD,&status2);
		Message aMessage(incoming,length,datatype,source,tag,source);
		inbox->push_back(aMessage);
		MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	}
}

MessagesHandler::MessagesHandler(){
	m_requests=NULL;
}
