/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

/* #define DEBUG_VIRTUAL_COMMUNICATOR */

#include <communication/VirtualCommunicator.h>
#include <core/OperatingSystem.h>
#include <core/types.h>
#include <core/ComputeCore.h>

#include <assert.h>
#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
using namespace std;

/** set the number of elements per message for a given tag */
void VirtualCommunicator::setElementsPerQuery(int tag,int size){
	#ifdef ASSERT
	assert(m_elementSizes.count(tag)==0);
	assert(tag!=INVALID_HANDLE);
	#endif

	m_elementSizes[tag]=size;
}

/** get the number of elements per message for a given tag */
int VirtualCommunicator::getElementsPerQuery(int tag){
	#ifdef ASSERT
	assert(m_elementSizes.count(tag)!=0);
	#endif

	return m_elementSizes[tag];
}

/**  associate the reply message tag to a message tag */
void VirtualCommunicator::setReplyType(int query,int reply){
	#ifdef ASSERT
	assert(m_replyTagToQueryTag.count(reply)==0);
	#endif

	m_replyTagToQueryTag[reply]=query;

	#ifdef ASSERT
	assert(m_reverseReplyMap.count(query) == 0);
	#endif

	m_reverseReplyMap[query]=reply;
}

int VirtualCommunicator::getReplyType(int tag){
	#ifdef ASSERT
	if(m_reverseReplyMap.count(tag) == 0){
		cout<<"Error: "<<MESSAGE_TAGS[tag]<<" is not in the reverse-map"<<endl;
	}
	assert(m_reverseReplyMap.count(tag) > 0);
	#endif
	
	return m_reverseReplyMap[tag];
}

/** push a message
 * this may trigger an actual message being flushed in the
 * message-passing interface stack 
 */
void VirtualCommunicator::pushMessage(uint64_t workerId,Message*message){
	int tag=message->getTag();

	#ifdef ASSERT
	assert(m_elementSizes.count(tag)>0);
	#endif

	int period=m_elementSizes[tag];
	int count=message->getCount();

	#ifdef ASSERT
	if(count > period){
		cout<<"Error, count= "<<count<<" but period is "<<period<<endl;
		cout<<"Tag= "<<MESSAGE_TAGS[tag]<<endl;
	}
	assert(count<=period);
	#endif

	m_pushedMessages++;
	#ifdef ASSERT
	if(m_elementsForWorkers.count(workerId)>0){
		cout<<"Error: there is already a pending message for worker "<<workerId<<", will not add message with tag="<<MESSAGE_TAGS[message->getTag()]<<endl;
		cout<<"Did you forget to pull a reply with isMessageProcessed and getMessageResponseElements ?"<<endl;
		cout<<"It is likely if you mix virtual messages with bare messages !"<<endl;
	}
	assert(m_elementsForWorkers.count(workerId)==0);
	#endif
	m_globalPushedMessageStatus=true;
	m_localPushedMessageStatus=true;

	int destination=message->getDestination();

	#ifdef ASSERT
	if(!(destination>=0&&destination<m_size)){
		cout<<"Error: tag="<<message->getTag()<<" destination="<<destination<<" (INVALID)"<<endl;
	}
	assert(destination>=0&&destination<m_size);
	#endif

	#ifdef ASSERT
	assert(m_elementSizes.count(tag)>0);
	#endif

	/**  generate   a group key for the message 
 * 	this is used for priority calculation
 * 	*/
	uint64_t elementId=getMessageUniqueId(destination,tag);
	int oldPriority=0;
	/* delete old priority */
	if(m_messageContent.count(tag)>0&&m_messageContent[tag].count(destination)>0){
		oldPriority=m_messageContent[tag][destination].size();
		m_priorityQueue[oldPriority].erase(elementId);
		if(m_priorityQueue[oldPriority].size()==0){
			m_priorityQueue.erase(oldPriority);
		}
	}
	int newPriority=oldPriority+count;
	m_priorityQueue[newPriority].insert(elementId);

	#ifdef ASSERT
	assert(count>0);
	#endif

	uint64_t*buffer=(uint64_t*)message->getBuffer();
	for(int i=0;i<count;i++){// count is probably 1...
		uint64_t element=buffer[i];
		m_messageContent[tag][destination].push_back(element);
	}

	/** add the worker workerId  to the list of workers that pushed a message of type
 * 	tag to message-passing interface  rank destination
 */
	m_workerCurrentIdentifiers[tag][destination].push_back(workerId);

	#ifdef ASSERT
	assert(m_elementSizes.count(tag)>0);
	#endif

	/** check if the current size is good enough to flush the whole thing */
	int currentSize=m_workerCurrentIdentifiers[tag][destination].size();

	/*
 *	maximum number of pushed messages
 */
	int threshold=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)/period;

	/** this whole block makes sure that the communicator is not overloaded */
	#ifdef ASSERT
	assert(m_elementSizes.count(tag)>0);
	assert(period>=1);
	assert(threshold<=(int)(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)));
	if(currentSize>threshold){
		cout<<"Fatal: too much bits, tag= "<<MESSAGE_TAGS[tag]<<" Threshold= "<<threshold<<" pushed messages; Actual= "<<currentSize<<" pushed messages; Period= "<<period<<" uint64_t/message; Count= "<<count<<" Priority= "<<newPriority<<" Destination: "<<destination<<endl;
		cout<<"This usually means that you did not use the VirtualCommunicator API correctly."<<endl;
		cout<<"Be careful not to push too many messages if the VirtualCommunicator is not ready."<<endl;
		cout<<"IMPORTANT: did you add entries for reply tags.txt and tag sizes ?"<<endl;
	}
	assert(currentSize<=threshold);
	#endif

	if(currentSize>=threshold){
		// must flush the message
		flushMessage(tag,destination);
	}
}

void VirtualCommunicator::flushMessage(int tag,int destination){
	#ifdef DEBUG_VIRTUAL_COMMUNICATOR
	cout<<"VirtualCommunicator:: sending multiplexed message to "<<destination<<endl;
	#endif

	m_flushedMessages++;

	#ifdef ASSERT
	assert(m_messageContent.count(tag)>0&&m_messageContent[tag].count(destination)>0);
	#endif

	// find the priority and erase it
	int priority=m_messageContent[tag][destination].size();
	uint64_t elementId=getMessageUniqueId(destination,tag);
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
	int requiredResponseLength=m_workerCurrentIdentifiers[tag][destination].size()*m_elementSizes[tag]*sizeof(uint64_t);
	assert(requiredResponseLength<=MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif

	uint64_t*messageContent=(uint64_t*)m_outboxAllocator->allocate(currentSize*sizeof(uint64_t));

	for(int j=0;j<currentSize;j++){
		messageContent[j]=m_messageContent[tag][destination][j];
	}

	m_messageContent[tag].erase(destination);
	if(m_messageContent[tag].size()==0){
		m_messageContent.erase(tag);
	}

	Message aMessage(messageContent,currentSize,destination,tag,m_rank);
	m_outbox->push_back(aMessage);

	m_pendingMessages++;
}

bool VirtualCommunicator::isMessageProcessed(uint64_t workerId){
	return m_elementsForWorkers.count(workerId)>0;
}

void VirtualCommunicator::getMessageResponseElements(uint64_t workerId,vector<uint64_t>*out){
	#ifdef ASSERT
	assert(isMessageProcessed(workerId));
	#endif

	for(int i=0;i<(int)(m_elementsForWorkers[workerId].size());i++)
		out->push_back(m_elementsForWorkers[workerId][i]);

	m_elementsForWorkers.erase(workerId);

	#ifdef ASSERT
	assert(!isMessageProcessed(workerId));
	#endif
}

void VirtualCommunicator::constructor(int rank,int size,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox){
	m_debug=false;

	if(m_debug)
		cout<<"Rank "<<rank<<" Initializing VirtualCommunicator"<<endl;

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
			#ifdef DEBUG_VIRTUAL_COMMUNICATOR
			cout<<"VirtualCommunicator: receiving multiplexed message, de-multiplexing data..."<<endl;
			#endif

			m_pendingMessages--;
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
			if(count==0){
				cout<<"QueryTag = "<<queryTag<<endl;
			}
			assert(count>0);
			if(count!=(int)workers.size()*elementsPerWorker){
				cout<<"Rank="<<m_rank<<" Count="<<count<<" Workers="<<workers.size()<<" ElementsPerWorker="<<elementsPerWorker<<" QueryTag="<<MESSAGE_TAGS[queryTag]<<endl;
			}
			assert(count==(int)workers.size()*elementsPerWorker);
			#endif

			// add the workers to a list
			// so they can be activated again
			for(int i=0;i<(int)workers.size();i++){
				uint64_t workerId=workers[i];
				activeWorkers->push_back(workerId);
				if(m_debug){
					cout<<"Reactivating "<<workerId<<" tag="<<queryTag<<endl;
				}
				int basePosition=i*elementsPerWorker;

				#ifdef ASSERT
				if(m_elementsForWorkers.count(workerId)>0){
					cout<<"there already are elements for "<<workerId<<endl;
				}
				assert(m_elementsForWorkers.count(workerId)==0);
				#endif

				// classify the data and bins
				for(int j=0;j<elementsPerWorker;j++){
					uint64_t element=buffer[basePosition+j];
					m_elementsForWorkers[workerId].push_back(element);
				}

				// make sure that is someone 
				// asks if workerId can fetch its thing,
				// it will return true.
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
	if(m_debug){
		cout<<__func__<<endl;
	}

	if(m_priorityQueue.size()==0){
		if(m_debug){
			cout<<"queue is empty"<<endl;
		}
		return;
	}

	// if forceFlush is called
	// and there are no message
	// the thing will crash 
	// it is designed like that
	#ifdef ASSERT
	assert(!m_priorityQueue.rbegin()->second.empty());
	#endif

	uint64_t elementId=*(m_priorityQueue.rbegin()->second.begin());
	int selectedDestination=getDestinationFromMessageUniqueId(elementId);
	int selectedTag=getTagFromMessageUniqueId(elementId);

	#ifdef ASSERT
	assert(m_messageContent.count(selectedTag)>0&&m_messageContent[selectedTag].count(selectedDestination)>0);
	assert(!m_messageContent[selectedTag][selectedDestination].empty());
	#endif

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
	int selectedDestination=getDestinationFromMessageUniqueId(elementId);
	int selectedTag=getTagFromMessageUniqueId(elementId);
	
	int period=m_elementSizes[selectedTag];
	int currentSize=m_messageContent[selectedTag][selectedDestination].size();
	int threshold=MAXIMUM_MESSAGE_SIZE_IN_BYTES/8;
	int value=currentSize*period;

	return value>=threshold;
}

void VirtualCommunicator::printStatistics(){
	double ratio=100.0*m_flushedMessages/m_pushedMessages;

	cout<<"Rank "<<m_rank<<" : VirtualCommunicator (service provided by VirtualCommunicator): "<<m_pushedMessages;
	cout<<" virtual messages generated "<<m_flushedMessages;
	cout<<" real messages ("<<ratio<<"%)"<<endl;
}

/** debugging will display a lot of messages */
void VirtualCommunicator::setDebug(){
	m_debug=true;
}

uint64_t VirtualCommunicator::getMessageUniqueId(int destination ,int tag){
	uint64_t a=tag;
	a=a*MAX_NUMBER_OF_MPI_PROCESSES+destination;
	return a;
}

int VirtualCommunicator::getTagFromMessageUniqueId(uint64_t a){
	return a/MAX_NUMBER_OF_MPI_PROCESSES;
}

int VirtualCommunicator::getDestinationFromMessageUniqueId(uint64_t a){
	int rank=a%MAX_NUMBER_OF_MPI_PROCESSES;
	return rank;
}

void VirtualCommunicator::registerPlugin(ComputeCore*core){
	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"VirtualCommunicator");
	core->setPluginDescription(m_plugin,"Multiplexing and demultiplexing of worker messages (bundled with RayPlatform)");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU Lesser General License version 3");
}

void VirtualCommunicator::resolveSymbols(ComputeCore*core){

}
