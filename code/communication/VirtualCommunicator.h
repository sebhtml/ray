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

// TYPE: COMMUNICATION

#ifndef _VirtualCommunicator
#define _VirtualCommunicator

#include <map>
#include <vector>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <communication/Message.h>
#include <stdint.h>
#include <memory/MyAllocator.h>
#include <queue>
using namespace std;

/**
 * this class provides an architecture for virtualization of message communication.
 *
 * This means that messages are grouped, but the programmer don't have to group them himself...
 *
 * Instead of using the standard m_inbox and m_outbox in Ray, the user uses
 * the methods below.
 *
 * The object is specific to a step of the algorithm because worker Identifiers are attributed.
 *
* this class is event-driven and tag-specific and destination-specific
*/
class VirtualCommunicator{
	bool m_debug;
	uint64_t m_pushedMessages;
	uint64_t m_flushedMessages;

	// priority, elementId, QueueElement
	map<int,set<uint64_t> > m_priorityQueue;

	// associates an MPI tag with a length reservation.
	// for instance, asking the coverage is 1 but asking ingoing edges is 5
	// key: MPI tag
	// number of elements per query
	// getting ingoing edges for a vertex requires 5 because at most there will be 4 and the size is a requirement
	map<int,int> m_elementSizes;

	// indicates to who belongs each elements to communicate, grouped according to m_elementSizes
	map<int,map<int,vector<uint64_t> > > m_workerCurrentIdentifiers;

	// the message contents
	// first key: MPI tag
	// second key: MPI destination
	// vector: contains elements to communicate
	map<int,map<int,vector<uint64_t> > > m_messageContent;

	// response to give to workers
	map<uint64_t,vector<uint64_t> > m_elementsForWorkers;

	// reply types.
	map<int,int> m_replyTagToQueryTag;

	int m_rank;
	int m_size;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	int m_activeDestination;
	int m_activeTag;
	int m_pendingMessages;

	bool m_localPushedMessageStatus;
	bool m_globalPushedMessageStatus;
	// flush a message associated to a tag and a destination
	void flushMessage(int tag,int destination);

public:
	/**
 * initiate the object
 * called once
 * time complexity: constant
 */ 
	void constructor(int rank,int size,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox);
	
	/**
 * associate a resonse tag to a query tag
 * called once
 * time complexity: constant
 */
	void setReplyType(int tag,int reply);

	/**
 * associate a period size to a tag type
 * this is bounded by the maximum between the query size and the reply size for
 * a single message
 * called once
 * time complexity: constant
 */
	void setElementsPerQuery(int tag,int size);
	
/**
 * get the number of elements per query
 */
	int getElementsPerQuery(int tag);

	/**
 * this method must be called before calling workers.
 * it will fetch messages from inbox according to ongoing queries.
 * time complexity: linear in the number of workers to set active (in general about one hundred
 */
	void processInbox(vector<uint64_t>*activeWorkers);

	/**
 * push a worker message
 * may not be sent instantaneously
 * called once per iteration on workers
 * if a worker calls it, then the iteration is stopped and won't be resumed until
 * the VirtualCommunicator is ready.
 * to do so, isReady is called before calling each worker.
 * if a called calls pushMessage and because of that a buffer becomes full, then isReady will return false 
 * for the next call
 * time complexity: log (number of tags) + log(number of MPI ranks)
 */
	
	void pushMessage(uint64_t workerId,Message*message);

	/**
 * return true if the response is ready to be read
 * time complexity: log(number of workers)
 */
	bool isMessageProcessed(uint64_t workerId);
	
	/**
 *
 * after calling isMessageProcessed, the worker must retrieve its data with 
 * getMessageResponseElements, else the whole thing will fail
 *
 * after reading the response, it is erased from
 * the current object
 * time complexity: log(number of workers)
 */
	vector<uint64_t> getMessageResponseElements(uint64_t workerId);
	
/**
 * if all workers are awaiting responses and 
 * none of the buffer is full, then this forces the flushing of a buffer
 * non-empty buffer.
 * time complexity: log(number of priority values)
 */
	void forceFlush();

	/**
 * set the slot to false. The slot says yes if a message was pushed
 * time complexity: constant
 */
	void resetLocalPushedMessageStatus();

	/** get the slot.
 * time complexity: constant
 */
	bool getLocalPushedMessageStatus();

	/**
 * reset the global slot
 * time complexity: constant
 */
	void resetGlobalPushedMessageStatus();

	/** get the slot.
 * time complexity: constant
 */
	bool getGlobalPushedMessageStatus();

	/** check if the communicator is ready
 * time complexity: constant
 */
	bool isReady();

/** check if the communicator has messages to flush
 * time complexity: constant
 */
	bool hasMessagesToFlush();

	bool nextIsAlmostFull();
	void printStatistics();
	void setDebug();
};

#endif
