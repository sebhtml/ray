/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#ifndef _MessagesHandler
#define _MessagesHandler

#include <mpi.h> // this is the only reference to MPI

#include <memory/MyAllocator.h>
#include <communication/Message.h>
//#include <core/common_functions.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <plugins/CorePlugin.h>

class ComputeCore;

#include <vector>
using namespace std;

#ifdef CONFIG_PERSISTENT_COMMUNICATION

/**
 * linked message
 */
class MessageNode{
public:
	MessageNode*m_previous;
	Message m_message;
	MessageNode*m_next;
};

#endif

/**
 * software layer to handler messages
 * it uses persistant communication
 * MessagesHandler is the only part of Ray that is aware of the message-passing interface.
 * All the other parts rely only on a simple inbox and a simple outbox.
 * This boxes of messages could be implemented with something else than message-passing interface.
 * \author Sébastien Boisvert
 */
class MessagesHandler: public CorePlugin{

	MessageTag RAY_MPI_TAG_DUMMY;

	bool m_destroyed;

	vector<int> m_connections;

	#ifdef USE_PERSISTENT_COMMUNICATION
	/** the number of buffered messages in the persistent layer */
	int m_bufferedMessages;

	/** allocators for messages received with persistent requests */
	MyAllocator m_internalBufferAllocator;
	MyAllocator m_internalMessageAllocator;

	/** linked lists */
	MessageNode**m_heads;

	/** linked lists */
	MessageNode**m_tails;

	#endif

	/** round-robin head */
	int m_currentRankIndexToTryToReceiveFrom;

	/** messages sent */
	uint64_t m_sentMessages;
	/** messages received */
	uint64_t m_receivedMessages;

	/**
 * 	In Ray, all messages have buffer of the same type
 */
	MPI_Datatype m_datatype;

	string m_processorName;

	/** the number of persistent requests in the ring */
	int m_ringSize;

	/** the current persistent request in the ring */
	int m_head;

	/** the ring of persistent requests */
	MPI_Request*m_ring;

	/** the ring of buffers for persistent requests */
	uint8_t*m_buffers;


	int m_rank;
	int m_size;

	/** this variable stores counts for sent messages */
	uint64_t*m_messageStatistics;

	/** initialize persistent communication parameters */
	void initialiseMembers();

	/** probe and read a message -- this method is not utilised */
	void probeAndRead(int source,int tag,StaticVector*inbox,RingAllocator*inboxAllocator);

	#ifdef USE_PERSISTENT_COMMUNICATION
	/** pump a message from the persistent ring */
	void pumpMessageFromPersistentRing();

	/** add a message to the internal messages */
	void addMessage(Message*a);
	#endif

	/** select and fetch a message from the internal messages using a round-robin policy */
	void roundRobinReception(StaticVector*inbox,RingAllocator*inboxAllocator);

	#ifdef CONFIG_PERSISTENT_COMMUNICATION

	roundRobinReception_persistent();

	#endif

	void createBuffers();

public:
	/** initialize the message handler
 * 	*/
	void constructor(int*argc,char***argv);

	/**
 *  send a message or more
 */
	void sendMessages(StaticVector*outbox);

	/**
 * receive one or zero message.
 * the others, if any, will be picked up in the next iteration
 */
	void receiveMessages(StaticVector*inbox,RingAllocator*inboxAllocator);

	/** free the ring elements */
	void freeLeftovers();

	/** get the processor name, usually set to the server name */
	string*getName();

	/** get the identifier of the current message passing interface rank */
	int getRank();

	/** get the number of ranks */
	int getSize();

	/** makes a barrier */
	void barrier();

	/** returns the version of the message passing interface standard that is available */
	void version(int*a,int*b);

	void destructor();

	/** write sent message counts to a file */
	void appendStatistics(const char*file);

	string getMessagePassingInterfaceImplementation();

	void setConnections(vector<int>*connections);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif


