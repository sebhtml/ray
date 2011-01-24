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
	#ifdef ASSERT
	assert(m_elementSizes.count(tag)==0);
	#endif
	m_elementSizes[tag]=size;
}

void VirtualCommunicator::setReplyType(int query,int reply){
	#ifdef ASSERT
	assert(m_replyTagToQueryTag.count(reply)==0);
	#endif
	m_replyTagToQueryTag[reply]=query;
}

void VirtualCommunicator::pushMessage(uint64_t workerId,Message*message){
	#ifdef ASSERT
	assert(m_elementsForWorkers.count(workerId)==0);
	#endif
	m_globalPushedMessageStatus=true;
	m_localPushedMessageStatus=true;
	int destination=message->getDestination();
	#ifdef ASSERT
	assert(destination>=0&&destination<m_size);
	#endif

	int tag=message->getTag();
	#ifdef ASSERT
	assert(m_elementSizes.count(tag)>0);
	#endif
	int count=message->getCount();
	//cout<<"Rank "<<m_rank<<" "<<__func__<<" Worker="<<workerId<<" Tag="<<tag<<" Destination="<<destination<<endl;
	#ifdef ASSERT
	assert(count>0);
	#endif
	uint64_t*buffer=(uint64_t*)message->getBuffer();
	for(int i=0;i<count;i++){// count is probably 1...
		uint64_t element=buffer[i];
		m_messageContent[tag][destination].push_back(element);
	}

	m_workerCurrentIdentifiers[tag][destination].push_back(workerId);

	#ifdef ASSERT
	assert(m_elementSizes.count(tag)>0);
	#endif
	int period=m_elementSizes[tag];
	int currentSize=m_messageContent[tag][destination].size();
	int threshold=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)/period;
	#ifdef ASSERT
	assert(m_elementSizes.count(tag)>0);
	assert(period>=1);
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
	//cout<<"Rank "<<m_rank<<" "<<__func__<<" RequiredResponseLength="<<requiredResponseLength<<" Tag="<<tag<<" Destination="<<destination<<endl;
	assert(requiredResponseLength<=MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif

	//cout<<"Rank "<<m_rank<<" "<<__func__<<" Capacity: "<<requiredResponseLength<<"/"<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<endl;
	//if(requiredResponseLength!=MAXIMUM_MESSAGE_SIZE_IN_BYTES){
		//cout<<"Rank "<<m_rank<<" "<<__func__<<" Under Capacity: "<<requiredResponseLength<<"/"<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<endl;
	//}

	uint64_t*messageContent=(uint64_t*)m_outboxAllocator->allocate(currentSize*sizeof(uint64_t));

	for(int j=0;j<currentSize;j++){
		messageContent[j]=m_messageContent[tag][destination][j];
	}

	m_messageContent[tag][destination].clear();

	Message aMessage(messageContent,currentSize,MPI_UNSIGNED_LONG_LONG,destination,tag,m_rank);
	m_outbox->push_back(aMessage);

	m_pendingMessages++;
	m_messages[destination]++;

	//cout<<"Rank "<<m_rank<<" "<<__func__<<" Tag="<<tag<<" Destination="<<destination<<" CurrentSize="<<currentSize<<" PendingMessages="<<m_messages[destination]<<" TotalPending="<<m_pendingMessages<<endl;
	//cout.flush();

	// save the list of workers
	m_workerIdentifiers[tag][destination].push(m_workerCurrentIdentifiers[tag][destination]);
	
	// clear the list of workers
	m_workerCurrentIdentifiers[tag][destination].clear();
}

bool VirtualCommunicator::isMessageProcessed(uint64_t workerId){
	return m_elementsForWorkers.count(workerId)>0;
}

vector<uint64_t> VirtualCommunicator::getResponseElements(uint64_t workerId){
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

	resetLocalPushedMessageStatus();
	resetGlobalPushedMessageStatus();
}

void VirtualCommunicator::processInbox(vector<uint64_t>*activeWorkers){
	if(m_pendingMessages>0&&m_inbox->size()>0){// we have mail
		Message*message=m_inbox->at(0);// there is 0 or 1 message in the inbox
		int incomingTag=message->getTag();
		int source=message->getSource();
		if(m_replyTagToQueryTag.count(incomingTag)==0){
			return;
		}
		#ifdef ASSERT
		assert(m_replyTagToQueryTag.count(incomingTag)>0);
		#endif
		int queryTag=m_replyTagToQueryTag[incomingTag];
		if(m_workerIdentifiers.count(queryTag)>0
		&& m_workerIdentifiers[queryTag].count(source)>0
		&& !m_workerIdentifiers[queryTag][source].empty()){
			m_pendingMessages--;
			m_messages[source]--;
			//cout<<"Rank "<<m_rank<<" "<<__func__<<" QueryTag="<<queryTag<<" Source="<<source<<" PendingMessages="<<m_messages[source]<<" TotalPending="<<m_pendingMessages<<endl;
			//cout.flush();
			uint64_t*buffer=(uint64_t*)message->getBuffer();
			#ifdef ASSERT
			assert(m_elementSizes.count(queryTag)>0);
			#endif
			int elementsPerWorker=m_elementSizes[queryTag];
			vector<uint64_t> workers=m_workerIdentifiers[queryTag][source].front();
			m_workerIdentifiers[queryTag][source].pop();

			#ifdef ASSERT
			assert(workers.size()>0);
			assert(elementsPerWorker>0);
			int count=message->getCount();
			assert(count>0);
			if(count!=(int)workers.size()*elementsPerWorker){
				cout<<"Rank="<<m_rank<<" Count="<<count<<" Workers="<<workers.size()<<" ElementsPerWorker="<<elementsPerWorker<<endl;
			}
			assert(count==(int)workers.size()*elementsPerWorker);
			#endif
			for(int i=0;i<(int)workers.size();i++){
				uint64_t workerId=workers[i];
				activeWorkers->push_back(workerId);
				int basePosition=i*elementsPerWorker;
				#ifdef ASSERT
				assert(m_elementsForWorkers.count(workerId)==0);
				#endif
				for(int j=0;j<elementsPerWorker;j++){
					uint64_t element=buffer[basePosition+j];
					m_elementsForWorkers[workerId].push_back(element);
				}
				#ifdef ASSERT
				assert(isMessageProcessed(workerId));
				#endif
			}
		}
	}
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif
}

// force the first encountered thing
void VirtualCommunicator::forceFlush(bool value){
	int selectedTag=-1;
	int selectedDestination=-1;
	int maxSize=-999;
	bool foundOne=false;
	for(map<int,map<int,vector<uint64_t> > >::iterator i=m_messageContent.begin();i!=m_messageContent.end();i++){
		int tag=i->first;
		for(map<int,vector<uint64_t> >::iterator j=i->second.begin();j!=i->second.end();j++){
			int destination=j->first;
			int size=j->second.size();

			if(size>0&&size>maxSize){
				maxSize=size;
				foundOne=true;
				selectedTag=tag;
				selectedDestination=destination;
			}
		}
	}
	if(foundOne){
		//cout<<"Rank "<<m_rank<<" "<<__func__<<" Tag="<<selectedTag<<" Destination="<<selectedDestination<<" Count="<<maxSize<<endl;
		flushMessage(selectedTag,selectedDestination);
		return;
	}
}

bool VirtualCommunicator::getLocalPushedMessageStatus(){
	return m_localPushedMessageStatus;
}

void VirtualCommunicator::resetLocalPushedMessageStatus(){
	m_localPushedMessageStatus=false;
}

void VirtualCommunicator::resetGlobalPushedMessageStatus(){
	m_globalPushedMessageStatus=false;
}

bool VirtualCommunicator::getGlobalPushedMessageStatus(){
	return m_globalPushedMessageStatus;
}

bool VirtualCommunicator::isReady(){
	return m_pendingMessages==0;
}
