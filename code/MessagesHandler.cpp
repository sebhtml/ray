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

		int value=MPI_Isend(aMessage->getBuffer(),aMessage->getCount(),aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(),MPI_COMM_WORLD,&request);
		
		#ifdef ASSERT
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
		MPI_Datatype datatype=MPI_UNSIGNED_LONG_LONG;
		int sizeOfType=8;
		int tag=status.MPI_TAG;
		int source=status.MPI_SOURCE;
		int length;
		MPI_Get_count(&status,datatype,&length);
		void*incoming=(void*)inboxAllocator->allocate(length*sizeOfType);
		MPI_Recv(incoming,length,datatype,source,tag,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
		Message aMessage(incoming,length,datatype,source,tag,source);
		inbox->push_back(aMessage);
		MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	}
}


