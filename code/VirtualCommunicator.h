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

#include<map>
#include <vector>
#include<RingAllocator.h>
#include <StaticVector.h>
#include <Message.h>
#include <stdint.h>
#include <MyAllocator.h>
#include <queue>
using namespace std;

/*
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

	// associates an MPI tag with a length reservation.
	// for instance, asking the coverage is 1 but asking ingoing edges is 5
	// key: MPI tag
	// number of elements per query
	// getting ingoing edges for a vertex requires 5 because at most there will be 4 and the size is a requirement
	map<int,int> m_elementSizes;

	// indicates to who belongs each elements to communicate, grouped according to m_elementSizes
	map<int,map<int,queue<vector<uint64_t> > > > m_workerIdentifiers;

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

	map<int,int> m_messages;
	int m_pendingMessages;

	bool m_localPushedMessageStatus;
	bool m_globalPushedMessageStatus;
	// flush a message associated to a tag and a destination
	void flushMessage(int tag,int destination);

public:
	// initiate the object
	// called once
	void constructor(int rank,int size,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox);
	
	// associate a resonse tag to a query tag
	// called once
	void setReplyType(int tag,int reply);

	// associate a period size to a tag type
	// this is bounded by the maximum between the query size and the reply size for
	// a single message
	// called once
	void setElementsPerQuery(int tag,int size);
	
	// this method must be called before calling workers.
	// it will fetch messages from inbox according to ongoing queries.
	void processInbox(set<uint64_t>*activeWorkers);

	// push a worker message
	// may not be sent instantaneously
	// called once per iteration on workers
	// if a worker calls it, then the iteration is stopped and won't be resumed until
	// the VirtualCommunicator is ready.
	// to do so, isReady is called before calling each worker.
	// if a called calls pushMessage and because of that a buffer becomes full, then isReady will return false 
	// for the next call
	void pushMessage(uint64_t workerId,Message*message);

	// return true if the response is ready to be read
	bool isMessageProcessed(uint64_t workerId);
	
	// after reading the response, it is erased from
	// the current object
	vector<uint64_t> getResponseElements(uint64_t workerId);
	
	// if all workers are awaiting responses and 
	// none of the buffer is full, then this forces the flushing of the first
	// non-empty buffer.
	void forceFlush(bool value);

	// set the slot to false. The slot says yes if a message was pushed
	void resetLocalPushedMessageStatus();

	// get the slot.
	bool getLocalPushedMessageStatus();

	// reset the global slot
	void resetGlobalPushedMessageStatus();

	bool getGlobalPushedMessageStatus();

	bool isReady();
};

#endif
