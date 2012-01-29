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
#include <plugins/CorePlugin.h>
#include <plugins/RegisteredPlugin.h>
#include <core/OperatingSystem.h>

#include <iostream>
#include <stdlib.h>
#include <stdint.h>
using namespace std;

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

	bool m_hasFirstMode;
	bool m_firstRegistration;

	vector<CorePlugin*> m_listOfPlugins;

	map<string,MasterMode> m_masterModeSymbols;
	map<string,SlaveMode> m_slaveModeSymbols;
	map<string,MessageTag> m_messageTagSymbols;

	set<SlaveMode> m_allocatedSlaveModes;
	set<MasterMode> m_allocatedMasterModes;
	set<MessageTag> m_allocatedMessageTags;

	set<SlaveMode> m_registeredSlaveModeSymbols;
	set<MasterMode> m_registeredMasterModeSymbols;
	set<MessageTag> m_registeredMessageTagSymbols;

	map<PluginHandle,RegisteredPlugin> m_plugins;

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

	SlaveMode m_currentSlaveModeToAllocate;
	MasterMode m_currentMasterModeToAllocate;
	MessageTag m_currentMessageTagToAllocate;

/** is the program alive ? */
	bool m_alive;

	void runVanilla();
	void runWithProfiler();

	void receiveMessages();
	void sendMessages();
	void processData();
	void processMessages();

	PluginHandle generatePluginHandle();

	bool validationPluginAllocated(PluginHandle plugin);
	bool validationSlaveModeOwnership(PluginHandle plugin,SlaveMode handle);
	bool validationMasterModeOwnership(PluginHandle plugin,MasterMode handle);
	bool validationMessageTagOwnership(PluginHandle plugin,MessageTag handle);

	bool validationMessageTagSymbolAvailable(PluginHandle plugin,const char*symbol);
	bool validationSlaveModeSymbolAvailable(PluginHandle plugin,const char*symbol);
	bool validationMasterModeSymbolAvailable(PluginHandle plugin,const char*symbol);
	bool validationMessageTagSymbolRegistered(PluginHandle plugin,const char*symbol);
	bool validationSlaveModeSymbolRegistered(PluginHandle plugin,const char*symbol);
	bool validationMasterModeSymbolRegistered(PluginHandle plugin,const char*symbol);

	bool validationMessageTagSymbolNotRegistered(PluginHandle plugin,MessageTag handle);
	bool validationSlaveModeSymbolNotRegistered(PluginHandle plugin,SlaveMode handle);
	bool validationMasterModeSymbolNotRegistered(PluginHandle plugin,MasterMode handle);

public:
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

	void stop();

	VirtualProcessor*getVirtualProcessor();
	VirtualCommunicator*getVirtualCommunicator();

	int getMaximumNumberOfAllocatedInboxMessages();
	int getMaximumNumberOfAllocatedOutboxMessages();
	void setMaximumNumberOfOutboxBuffers(int maxNumberOfBuffers);
	
	void registerPlugin(CorePlugin*plugin);
	void resolveSymbols();

	void destructor();

	void printPlugins(string directory);

/********************************************************************/
/** the methods below are available for plugin registration **/

/** allocate an handle for a plugin **/
	PluginHandle allocatePluginHandle();

/** sets the name of a plugin **/
	void setPluginName(PluginHandle plugin,const char*name);

/** sets the description of a plugin **/
	void setPluginDescription(PluginHandle handle,const char*text);

/** sets the authors of a plugin **/
	void setPluginAuthors(PluginHandle handle,const char*text);

/** sets the license of a plugin **/
	void setPluginLicense(PluginHandle handle,const char*text);



/** allocate a master mode **/
	MasterMode allocateMasterModeHandle(PluginHandle plugin,MasterMode desiredValue);

/** sets the symbol for a master mode **/
	void setMasterModeSymbol(PluginHandle plugin,MasterMode mode,const char*symbol);

/** get a master mode from its symbol **/
	MasterMode getMasterModeFromSymbol(PluginHandle plugin,const char*symbol);

/** add a master mode handler */
	void setMasterModeObjectHandler(PluginHandle plugin,MasterMode mode,MasterModeHandler*object);

/** sets the master mode switch for a master mode
 * this tells the core which message tag is to be automaticalled broadcasted for 
 * a given master mode **/
	void setMasterModeToMessageTagSwitch(PluginHandle plugin,MasterMode mode,MessageTag tag);

/* The Ray engine will run these master modes in series 
in a parallel and distributed way.
This is for modes that support the feature.
Not all master modes have yet been ported to that list.

-Sébastien Boisvert, 2011-01-08
*/
	void setMasterModeNextMasterMode(PluginHandle plugin,MasterMode mode,MasterMode next);

/** defines the entry point of the application **/
	void setFirstMasterMode(PluginHandle plugin,MasterMode mode);



/** allocate a slave mode for a handle **/
	SlaveMode allocateSlaveModeHandle(PluginHandle plugin,SlaveMode desiredValue);

/** sets the symbol for a slave mode **/
	void setSlaveModeSymbol(PluginHandle plugin,SlaveMode mode,const char*symbol);

/** add a slave mode handler */
	void setSlaveModeObjectHandler(PluginHandle plugin,SlaveMode mode,SlaveModeHandler*object);

/** get a slave mode from its symbol **/
	SlaveMode getSlaveModeFromSymbol(PluginHandle plugin,const char*symbol);



/** allocate a handle for a message tag **/
	MessageTag allocateMessageTagHandle(PluginHandle plugin, MessageTag desiredValue);
	
/** set the symbol for a message tag **/
	void setMessageTagSymbol(PluginHandle plugin,MessageTag mode,const char*symbol);

/** get a message tag from its symbol **/
	MessageTag getMessageTagFromSymbol(PluginHandle plugin,const char*symbol);

/** add a message tag handler */
	void setMessageTagObjectHandler(PluginHandle plugin,MessageTag tag,MessageTagHandler*object);

/** sets the slave switch for a slave mode
 * this tells the core which slave mode to switch to when receiving a particular message tag **/
	void setMessageTagToSlaveModeSwitch(PluginHandle plugin,MessageTag tag,SlaveMode mode);

/** sets the reply tag for a message tag **/
	void setMessageTagReplyMessageTag(PluginHandle plugin,MessageTag tag,MessageTag reply);

/** sets the number of elements for a message tag **/
	void setMessageTagSize(PluginHandle plugin,MessageTag tag,int size);
};

#endif

