/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _ComputeCore_h
#define _ComputeCore_h

#include <handlers/MessageTagHandler.h>
#include <handlers/SlaveModeHandler.h>
#include <handlers/MasterModeHandler.h>
#include <communication/MessagesHandler.h>
#include <profiling/Profiler.h>
#include <structures/StaticVector.h>
#include <communication/Message.h>
#include <communication/MessageRouter.h>
#include <profiling/TickLogger.h>
#include <scheduling/SwitchMan.h>
#include <memory/RingAllocator.h>
#include <scheduling/VirtualProcessor.h>
#include <communication/VirtualCommunicator.h>

#include <stdint.h>

/** this class is a compute core
 * to use it, you must set the handlers of your program
 * using setMessageTagObjectHandler(), setSlaveModeObjectHandler(),
 * and setMasterModeObjectHandler().
 *
 * after that, you simply have to call run()
 *
 * \author Sébastien Boisvert
 */
class ComputeCore{
/** the maximum number of messages with a non-NULL buffers in
 * the outbox */
	int m_maximumAllocatedOutputBuffers;

/** the maximum number of messages in the outbox */
	int m_maximumNumberOfOutboxMessages;

/** the maximum number of inbox messages */
	int m_maximumNumberOfInboxMessages;

	/** the virtual communicator of the MPI rank */
	VirtualCommunicator m_virtualCommunicator;

	/** the virtual processor of the MPI rank */
	VirtualProcessor m_virtualProcessor;

	uint64_t m_startingTimeMicroseconds;

	int m_rank;
	int m_size;
	bool m_showCommunicationEvents;
	bool m_profilerVerbose;
	bool m_runProfiler;

/** the profiler
 * enabled with -run-profiler
 */
	Profiler m_profiler;

/** the switch man */
	SwitchMan m_switchMan;
	TickLogger m_tickLogger;

/** the message router */
	MessageRouter m_router;

	StaticVector m_outbox;
	StaticVector m_inbox;

	/** middleware to handle messages */
	MessagesHandler m_messagesHandler;

/** this object handles messages */
	MessageTagHandler m_messageTagHandler;

/** this object handles master modes */
	MasterModeHandler m_masterModeHandler;

/* this object handles slave modes */
	SlaveModeHandler m_slaveModeHandler;

	// allocator for outgoing messages
	RingAllocator m_outboxAllocator;
	
	// allocator for ingoing messages
	RingAllocator m_inboxAllocator;

/** is the program alive ? */
	bool m_alive;

	void runVanilla();
	void runWithProfiler();

	void receiveMessages();
	void sendMessages();
	void processData();
	void processMessages();

public:

/** add a slave mode handler */
	void setSlaveModeObjectHandler(SlaveMode mode,SlaveModeHandler*object);

/** add a master mode handler */
	void setMasterModeObjectHandler(MasterMode mode,MasterModeHandler*object);

/** add a message tag handler */
	void setMessageTagObjectHandler(MessageTag tag,MessageTagHandler*object);

	/** this is the main method */
	void run();

	/** get the middleware object */
	MessagesHandler*getMessagesHandler();

	void constructor(int*argc,char***argv);

	void enableProfiler();
	void showCommunicationEvents();
	void enableProfilerVerbosity();

	Profiler*getProfiler();

	TickLogger*getTickLogger();
	SwitchMan*getSwitchMan();
	StaticVector*getOutbox();
	StaticVector*getInbox();
	MessageRouter*getRouter();

	RingAllocator*getOutboxAllocator();
	RingAllocator*getInboxAllocator();

	bool*getLife();

	VirtualProcessor*getVirtualProcessor();
	VirtualCommunicator*getVirtualCommunicator();

	int getMaximumNumberOfAllocatedInboxMessages();
	int getMaximumNumberOfAllocatedOutboxMessages();
	void setMaximumNumberOfOutboxBuffers(int maxNumberOfBuffers);
};

#endif
