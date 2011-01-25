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
	m_pushedMessages++;
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

	uint64_t elementId=tag*MAX_NUMBER_OF_MPI_PROCESSES+destination;
	int oldPriority=0;
	// delete old priority
	if(m_messageContent.count(tag)>0&&m_messageContent[tag].count(destination)>0){
		oldPriority=m_messageContent[tag][destination].size();
		m_priorityQueue[oldPriority].erase(elementId);
		if(m_priorityQueue[oldPriority].size()==0){
			m_priorityQueue.erase(oldPriority);
		}
	}
	int count=message->getCount();
	int newPriority=oldPriority+count;
	m_priorityQueue[newPriority].insert(elementId);
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
	int currentSize=m_messageContent[tag][destination].size();

	int period=m_elementSizes[tag];
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
	m_flushedMessages++;
	#ifdef ASSERT
	assert(m_messageContent.count(tag)>0&&m_messageContent[tag].count(destination)>0);
	#endif
	int priority=m_messageContent[tag][destination].size();
	uint64_t elementId=tag*MAX_NUMBER_OF_MPI_PROCESSES+destination;
	m_priorityQueue[priority].erase(elementId);
	if(m_priorityQueue[priority].size()==0){
		m_priorityQueue.erase(priority);
	}
	m_activeDestination=destination;
	m_activeTag=tag;
	int currentSize=priority;
	#ifdef ASSERT
	if(currentSize==0){
		cout<<"Cannot flush empty buffer!"<<endl;
	}
	assert(currentSize>0);
	int requiredResponseLength=currentSize*m_elementSizes[tag]*sizeof(uint64_t);
	m_distribution[requiredResponseLength]++;
	//cout<<"Rank "<<m_rank<<" "<<__func__<<" RequiredResponseLength="<<requiredResponseLength<<" Tag="<<tag<<" Destination="<<destination<<endl;
	assert(requiredResponseLength<=MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif

/*
	cout<<"Rank "<<m_rank<<" "<<__func__<<" Tag="<<tag<<" Destination="<<destination<<" Capacity: "<<requiredResponseLength<<" / "<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<endl;
	cout.flush();
*/
	//if(requiredResponseLength!=MAXIMUM_MESSAGE_SIZE_IN_BYTES){
		//cout<<"Rank "<<m_rank<<" "<<__func__<<" Under Capacity: "<<requiredResponseLength<<"/"<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<endl;
	//}

	uint64_t*messageContent=(uint64_t*)m_outboxAllocator->allocate(currentSize*sizeof(uint64_t));

	for(int j=0;j<currentSize;j++){
		messageContent[j]=m_messageContent[tag][destination][j];
	}

	m_messageContent[tag].erase(destination);
	if(m_messageContent[tag].size()==0){
		m_messageContent.erase(tag);
	}

	Message aMessage(messageContent,currentSize,MPI_UNSIGNED_LONG_LONG,destination,tag,m_rank);
	m_outbox->push_back(aMessage);

	m_pendingMessages++;

	//cout<<"Rank "<<m_rank<<" "<<__func__<<" Tag="<<tag<<" Destination="<<destination<<" CurrentSize="<<currentSize<<" PendingMessages="<<m_messages[destination]<<" TotalPending="<<m_pendingMessages<<endl;
	//cout.flush();
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
	m_pushedMessages=0;
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
		if(m_activeTag==queryTag&&m_activeDestination==source){
			m_pendingMessages--;
			//cout<<"Rank "<<m_rank<<" "<<__func__<<" QueryTag="<<queryTag<<" Source="<<source<<" TotalPending="<<m_pendingMessages<<endl;
			cout.flush();
			uint64_t*buffer=(uint64_t*)message->getBuffer();
			#ifdef ASSERT
			assert(m_elementSizes.count(queryTag)>0);
			#endif
			int elementsPerWorker=m_elementSizes[queryTag];
			vector<uint64_t> workers=m_workerCurrentIdentifiers[m_activeTag][m_activeDestination];
			m_workerCurrentIdentifiers[m_activeTag][m_activeDestination].clear();
			
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

// force the first encountered thing
void VirtualCommunicator::forceFlush(){
	if(m_priorityQueue.size()==0){
		return;
	}
	#ifdef ASSERT
	assert(!m_priorityQueue.rbegin()->second.empty());
	#endif
	uint64_t elementId=*(m_priorityQueue.rbegin()->second.begin());
	int selectedDestination=elementId%MAX_NUMBER_OF_MPI_PROCESSES;
	int selectedTag=elementId/MAX_NUMBER_OF_MPI_PROCESSES;
	#ifdef ASSERT
	assert(m_messageContent.count(selectedTag)>0&&m_messageContent[selectedTag].count(selectedDestination)>0);
	assert(!m_messageContent[selectedTag][selectedDestination].empty());
	#endif
	//cout<<"Rank "<<m_rank<<" "<<__func__<<" Tag="<<selectedTag<<" Destination="<<selectedDestination<<" Count="<<maxSize<<endl;
	flushMessage(selectedTag,selectedDestination);
}

bool VirtualCommunicator::hasMessagesToFlush(){
	return !m_messageContent.empty();
}

bool VirtualCommunicator::nextIsAlmostFull(){
	if(m_priorityQueue.empty()){
		return false;
	}
	
	uint64_t elementId=*(m_priorityQueue.rbegin()->second.begin());
	int selectedDestination=elementId%MAX_NUMBER_OF_MPI_PROCESSES;
	int selectedTag=elementId/MAX_NUMBER_OF_MPI_PROCESSES;
	
	int period=m_elementSizes[selectedTag];
	int currentSize=m_messageContent[selectedTag][selectedDestination].size();
	int threshold=MAXIMUM_MESSAGE_SIZE_IN_BYTES/8;
	int value=currentSize*period;
	return value>=threshold;
}

void VirtualCommunicator::printStatistics(){
	printf("Rank %i VirtualCommunicator Statistics\n",m_rank);
	fflush(stdout);
	uint64_t total=0;
	for(map<int,uint64_t>::iterator i=m_distribution.begin();i!=m_distribution.end();i++){
		break;
		int size=i->first;
		uint64_t count=i->second;
		total+=count;
		printf("Rank %i Size: %i Count: %lu\n",m_rank,size,count);
		fflush(stdout);
	}
	printf("Rank %i: %lu virtual messages for %lu pushed messages\n",m_rank,m_flushedMessages,m_pushedMessages);
	fflush(stdout);
}
