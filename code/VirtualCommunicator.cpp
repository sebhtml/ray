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
	m_replyTagToQueryTag[reply]=query;
}

void VirtualCommunicator::pushMessage(int workerId,Message*message){
	m_pushedMessageSlot=true;
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

	m_workerCurrentIdentifiers[tag][destination].push_back(workerId);

	int period=m_elementSizes[tag];
	int currentSize=m_messageContent[tag][destination].size();
	int threshold=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)/period;
	#ifdef ASSERT
	assert(m_elementSizes.count(tag)>0);
	#endif
	#ifdef ASSERT
	assert(period>=1);
	#endif
	#ifdef ASSERT
	assert(threshold<=(int)(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)));
	assert(currentSize<=threshold);
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
	int requiredResponseLength=currentSize*m_elementSizes[tag]*sizeof(uint64_t);
	//cout<<__func__<<" "<<requiredResponseLength<<endl;
	assert(requiredResponseLength<=MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif

	//cout<<"Capacity: "<<requiredResponseLength<<"/"<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<endl;

	uint64_t*messageContent=(uint64_t*)m_outboxAllocator->allocate(currentSize*sizeof(uint64_t));

	for(int j=0;j<currentSize;j++){
		messageContent[j]=m_messageContent[tag][destination][j];
	}
	m_messageContent[tag][destination].clear();
	Message aMessage(messageContent,currentSize,MPI_UNSIGNED_LONG_LONG,destination,tag,m_rank);
	m_outbox->push_back(aMessage);
	m_pendingMessages++;
	m_workerIdentifiers[tag][destination].push(m_workerCurrentIdentifiers[tag][destination]);
	m_workerCurrentIdentifiers[tag][destination].clear();
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

void VirtualCommunicator::resetPushedMessageSlot(){
	m_pushedMessageSlot=false;
}

bool VirtualCommunicator::getPushedMessageSlot(){
	return m_pushedMessageSlot;
}

void VirtualCommunicator::constructor(int rank,int size,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox){
	resetPushedMessageSlot();
	m_rank=rank;
	m_size=size;
	m_outboxAllocator=outboxAllocator;
	m_inbox=inbox;
	m_outbox=outbox;
	m_pendingMessages=0;
	m_messagesWereAdded=false;
}

void VirtualCommunicator::processInbox(set<int>*activeWorkers){
	m_messagesWereAdded=false;
	if(m_pendingMessages>0&&m_inbox->size()>0){// we have mail
		Message*message=m_inbox->at(0);
		int incomingTag=message->getTag();
		int source=message->getSource();
		int queryTag=m_replyTagToQueryTag[incomingTag];
		if(m_workerIdentifiers.count(queryTag)>0
		&& m_workerIdentifiers[queryTag].count(source)>0
		&& !m_workerIdentifiers[queryTag][source].empty()){
			m_pendingMessages--;
			uint64_t*buffer=(uint64_t*)message->getBuffer();
			int elementsPerWorker=m_elementSizes[queryTag];
			vector<int> workers=m_workerIdentifiers[queryTag][source].front();
			
			#ifdef ASSERT
			assert(workers.size()>0);
			#endif

			#ifdef ASSERT
			int count=message->getCount();
			if(count!=(int)workers.size()*elementsPerWorker){
				cout<<"Rank="<<m_rank<<" Count="<<count<<" Workers="<<workers.size()<<" ElementsPerWorker="<<elementsPerWorker<<" OngoingQueries="<<m_workerIdentifiers[queryTag][source].size()<<endl;
			}
			assert(count==(int)workers.size()*elementsPerWorker);
			#endif
			for(int i=0;i<(int)workers.size();i++){
				int workerId=workers[i];
				activeWorkers->insert(workerId);
				int basePosition=i*elementsPerWorker;
				for(int j=0;j<elementsPerWorker;j++){
					uint64_t element=buffer[basePosition+j];
					m_elementsForWorkers[workerId].push_back(element);
				}
			}

			m_workerIdentifiers[queryTag][source].pop();
		}
	}
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif
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
