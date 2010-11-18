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
void MessagesHandler::sendMessages(vector<Message>*outbox,OutboxAllocator*outboxAllocator,vector<Message>*inbox,int rank,MyAllocator*inboxAllocator){
	if(outbox->size()==0){
		return;
	}

	for(int i=0;i<(int)outbox->size();i++){
		Message*aMessage=&((*outbox)[i]);
		#ifdef DEBUG
		int destination=aMessage->getDestination();
		assert(destination>=0);
		#endif

		MPI_Request request;
		//  MPI_Issend
		//      Synchronous nonblocking. Note that a Wait/Test will complete only when the matching receive is posted

		#ifdef DEBUG
		assert(!(aMessage->getBuffer()==NULL && aMessage->getCount()>0));
		int value=MPI_Issend(aMessage->getBuffer(),aMessage->getCount(),aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(),MPI_COMM_WORLD,&request);
		assert(value==MPI_SUCCESS);
		#else
		MPI_Issend(aMessage->getBuffer(), aMessage->getCount(), aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(), MPI_COMM_WORLD,&request);
		#endif
		addRequest(&request,aMessage->getBuffer());
	}

	outbox->clear();
	freeRequests(outboxAllocator); 
}

// O(1) add, add it to the root
void MessagesHandler::addRequest(MPI_Request*request,void*buffer){
	PendingRequest*newRequest=(PendingRequest*)m_customAllocator.allocate(sizeof(PendingRequest));
	newRequest->m_mpiRequest=request;
	newRequest->m_buffer=buffer;
	newRequest->m_next=m_root;
	m_root=newRequest;
}

// O(n) freeing, pass through everything and free stuff that are completed.
void MessagesHandler::freeRequests(OutboxAllocator*outboxAllocator){
	// free m_root
	//
	//
	// m_root ----------------> Request
	//                              > MPI_Request*m_mpiRequest
	//                              > Request*m_next

	while(m_root!=NULL){
		MPI_Request*request=m_root->m_mpiRequest;
		int flag=0;
		#ifdef DEBUG_MPI
		assert(*request!=MPI_REQUEST_NULL);
		#endif
		
		if(*request==MPI_REQUEST_NULL){// Open-MPI is blazing fast, just amazing.
			flag=1;
		}else{
			MPI_Test(request,&flag,MPI_STATUS_IGNORE);
		}
		if(flag){
			PendingRequest*next=m_root->m_next;
			//cout<<"Freeing root "<<m_root->m_buffer<<endl;
			outboxAllocator->free(m_root->m_buffer);
			m_customAllocator.free(m_root);
			m_root=next;
		}else{
			break;// root is not ready to be freed
		}
	}

	if(m_root==NULL){// everyone is happy, nothing to do further
		return;
	}

	PendingRequest*previous=m_root;
	PendingRequest*current=previous->m_next;

	// m_root is not completed to this point, if it is not NULL
	// current is the next
	
	// m_root ----------------> Request  (this is previous)
	//                              > MPI_Request*m_mpiRequest
	//                              > Request*m_next           ----------------> Request  (This one is current)
	//                                                                             > MPI_Request*m_mpiRequest
	//                                                                             > Request*m_next  (this is next)

	while(current!=NULL){
		MPI_Request*request=current->m_mpiRequest;
		int flag=0;
		if(*request==MPI_REQUEST_NULL){// Open-MPI is impressive
			flag=1;
		}else{
			MPI_Test(request,&flag,MPI_STATUS_IGNORE);
		}
		PendingRequest*next=current->m_next;
		if(flag){
			outboxAllocator->free(current->m_buffer);
			//cout<<"Freeing current "<<current->m_buffer<<endl;
			m_customAllocator.free(current);
			previous->m_next=next;
		}else{
			previous=current;
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
	//cout<<"MPI_Iprobe"<<endl;
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

MessagesHandler::MessagesHandler(){
	m_root=NULL;
	m_customAllocator.constructor(MAX_ALLOCATED_MESSAGES,sizeof(PendingRequest));
}

void MessagesHandler::printRequests(){
	PendingRequest*a=m_root;
	cout<<"Requests"<<endl;
	while(a!=NULL){
		cout<<a->m_mpiRequest<<endl;
		a=a->m_next;
	}
	cout<<"End of Requests"<<endl;
}

