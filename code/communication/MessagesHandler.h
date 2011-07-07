/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#ifndef _MessagesHandler
#define _MessagesHandler

#include <mpi.h>
#include <memory/MyAllocator.h>
#include <communication/Message.h>
#include <core/common_functions.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>

/**
 * software layer to handler messages
 * it uses persistant communication
 * MessagesHandler is the only part of Ray that is aware of the message-passing interface.
 * All the other parts rely only on a simple inbox and a simple outbox.
 * This boxes of messages could be implemented with something else than message-passing interface.
 */
class MessagesHandler{
	/** messages sent */
	uint64_t m_sentMessages;
	/** messages received */
	uint64_t m_receivedMessages;

	/**
 * 	In Ray, all messages have buffer of the same type
 */
	MPI_Datatype m_datatype;

	string m_processorName;

	int m_ringSize;
	int m_head;
	MPI_Request*m_ring;
	uint8_t*m_buffers;
	int m_rank;
	int m_size;

	/** this variable stores counts for sent messages */
	uint64_t*m_messageStatistics;

	void initialiseMembers();

public:
	/** initialize the message handler
 * 	*/
	void constructor(int*argc,char***argv);

	/**
 *  send a message or more
 */
	void sendMessages(StaticVector*outbox,int source);
	/**
 * receive one or zero message.
 * the others, if any, will be picked up in the next iteration
 */
	void receiveMessages(StaticVector*inbox,RingAllocator*inboxAllocator,int destination);

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
};

#endif
