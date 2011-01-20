/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#include<VirtualCommunicator.h>
#include<common_functions.h>
#include <mpi.h>

void VirtualCommunicator::setElementsPerQuery(int tag,int size){
	m_elementSizes[tag]=size;
}

void VirtualCommunicator::setReplyType(int query,int reply){
	m_replyTypes[query]=reply;
}

void VirtualCommunicator::pushMessage(int workerId,Message*message){
	m_messagesWereAdded=true;
	int destination=message->getDestination();
	int tag=message->getTag();
	int count=message->getCount();
	#ifdef ASSERT
	assert(count>0);
	#endif
	uint64_t*buffer=(uint64_t*)message->getBuffer();
	for(int i=0;i<count;i++){// count is probably 1...
		uint64_t element=buffer[i];
		m_messageContent[tag][destination].push_back(element);
	}
	m_workerIdentifiers[tag][destination].push_back(workerId);

	int currentSize=m_messageContent[tag][destination].size();
	int threshold=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);
	int period=m_elementSizes[tag];
	threshold/=period;
	threshold*=period;
	#ifdef ASSERT
	assert(threshold<=(int)(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)));
	#endif
	if(currentSize>=threshold){
		// must flush the message
		flushMessage(tag,destination);
	}
}

void VirtualCommunicator::flushMessage(int tag,int destination){
	int currentSize=m_messageContent[tag][destination].size();
	#ifdef ASSERT
	assert(currentSize>0);
	#endif
	uint64_t*messageContent=(uint64_t*)m_outboxAllocator->allocate(currentSize*sizeof(uint64_t));
	for(int j=0;j<currentSize;j++){
		messageContent[j]=m_messageContent[tag][destination][j];
	}
	m_messageContent[tag][destination].clear();
	Message aMessage(messageContent,currentSize,MPI_UNSIGNED_LONG_LONG,destination,tag,m_rank);
	m_outbox->push_back(aMessage);
	m_pendingMessages++;
	m_activeTag=tag;
	m_activeDestination=destination;
}

bool VirtualCommunicator::isMessageProcessed(int workerId){
	return m_elementsForWorkers.count(workerId)>0;
}

vector<uint64_t> VirtualCommunicator::getResponseElements(int workerId){
	#ifdef ASSERT
	assert(isMessageProcessed(workerId));
	#endif
	vector<uint64_t> elements=m_elementsForWorkers[workerId];
	m_elementsForWorkers.erase(workerId);
	#ifdef ASSERT
	assert(!isMessageProcessed(workerId));
	#endif
	return elements;
}


void VirtualCommunicator::constructor(int rank,int size,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox){
	m_rank=rank;
	m_size=size;
	m_outboxAllocator=outboxAllocator;
	m_inbox=inbox;
	m_outbox=outbox;
	m_pendingMessages=0;
	m_messagesWereAdded=false;
}

bool VirtualCommunicator::isReady(){
	m_messagesWereAdded=false;// reset the counter 
	if(m_pendingMessages>0&&m_inbox->size()>0){// we have mail
		Message*message=m_inbox->at(0);
		int incomingTag=message->getTag();
		int source=message->getSource();
		int replyType=m_replyTypes[m_activeTag];
		if(incomingTag==replyType&&source==m_activeDestination){
			m_pendingMessages--;
			uint64_t*buffer=(uint64_t*)message->getBuffer();
			int elementsPerWorker=m_elementSizes[m_activeTag];
			int workers=m_workerIdentifiers[m_activeTag][m_activeDestination].size();
			#ifdef ASSERT
			int count=message->getCount();
			assert(count==workers*elementsPerWorker);
			#endif
			for(int i=0;i<workers;i++){
				int workerId=m_workerIdentifiers[m_activeTag][m_activeDestination][i];
				int basePosition=i*elementsPerWorker;
				for(int j=0;j<elementsPerWorker;j++){
					uint64_t element=buffer[basePosition+j];
					m_elementsForWorkers[workerId].push_back(element);
				}
			}
			m_workerIdentifiers[m_activeTag][m_activeDestination].clear();
		}
	}
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif
	return m_pendingMessages==0;
}

void VirtualCommunicator::forceFlushIfNothingWasAppended(){
	if(m_messagesWereAdded){
		return;
	}
	for(map<int,map<int,vector<uint64_t> > >::iterator i=m_messageContent.begin();i!=m_messageContent.end();i++){
		int tag=i->first;
		for(map<int,vector<uint64_t> >::iterator j=i->second.begin();j!=i->second.end();j++){
			int destination=j->first;
			if(j->second.size()>0){
				flushMessage(tag,destination);
				return;
			}
		}
	}
}
