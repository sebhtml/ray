/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#include <core/ComputeCore.h>
#include <iostream>
#include <core/OperatingSystem.h>
using namespace std;

void ComputeCore::setSlaveModeObjectHandler(SlaveMode mode,SlaveModeHandler*object){
	m_slaveModeHandler.setObjectHandler(mode,object);
}

void ComputeCore::setMasterModeObjectHandler(MasterMode mode,MasterModeHandler*object){
	m_masterModeHandler.setObjectHandler(mode,object);
}

void ComputeCore::setMessageTagObjectHandler(MessageTag tag,MessageTagHandler*object){
	m_messageTagHandler.setObjectHandler(tag,object);
}


/**
 * runWithProfiler if -run-profiler is provided
 * otherwise, run runVanilla()
 */
void ComputeCore::run(){

	m_startingTimeMicroseconds = getMicroseconds();

	if(m_runProfiler){
		runWithProfiler();
	}else{
		runVanilla();
	}
}

/**
 * the while loop is *the* main loop of Ray for each 
 * processor.
 * it is similar to the main loop of a video game, actually, but without a display.
 */
void ComputeCore::runVanilla(){
	while(m_alive || (m_router.isEnabled() && !m_router.hasCompletedRelayEvents())){
		// 1. receive the message (0 or 1 message is received)
		// blazing fast, receives 0 or 1 message, never more, never less, other messages will wait for the next iteration !
		receiveMessages(); 

		// 2. process the received message, if any
		// consume the one message received, if any, also very fast because it is done with an array mapping tags to function pointers
		processMessages();

		// 3. process data according to current slave and master modes
		// should be fast, but apparently call_RAY_SLAVE_MODE_EXTENSION is slowish sometimes...
		processData();

		// 4. send messages
		// fast, sends at most 17 messages. In most case it is either 0 or 1 message.,..
		sendMessages();
	}
}

/*
 * This is the main loop of the program.
 * One instance on each MPI rank.
 */
void ComputeCore::runWithProfiler(){
	// define some variables that hold life statistics of this
	// MPI rank
	int ticks=0;
	int sentMessages=0;
	int sentMessagesInProcessMessages=0;
	int sentMessagesInProcessData=0;
	int receivedMessages=0;
	map<int,int> receivedTags;
	map<int,int> sentTagsInProcessMessages;
	map<int,int> sentTagsInProcessData;
	
	int resolution=100;// milliseconds
	int parts=1000/resolution;

	uint64_t startingTime=getMilliSeconds();

	uint64_t lastTime=getMilliSeconds();

/*
	uint64_t lastTickWhenSentMessageInProcessMessage=lastTime;
	uint64_t lastTickWhenSentMessageInProcessData=lastTime;
*/

	vector<int> distancesForProcessMessages;
	vector<int> distancesForProcessData;

	bool profilerVerbose=m_profilerVerbose; 

	while(m_alive  || (m_router.isEnabled() && !m_router.hasCompletedRelayEvents())){
		uint64_t t=getMilliSeconds();
		if(t>=(lastTime+resolution)/parts*parts){

			double seconds=(t-startingTime)/1000.0;

			int balance=sentMessages-receivedMessages;

			if(profilerVerbose){
				printf("Rank %i: %s Time= %.2f s Speed= %i Sent= %i (processMessages: %i, processData: %i) Received= %i Balance= %i\n",
					m_rank,SLAVE_MODES[m_switchMan.getSlaveMode()],
					seconds,ticks,sentMessages,sentMessagesInProcessMessages,sentMessagesInProcessData,
					receivedMessages,balance);
				fflush(stdout);

				m_profiler.printGranularities(m_rank);
			}

			m_profiler.clearGranularities();

			if(receivedTags.size() > 0 && profilerVerbose){
				cout<<"Rank "<<m_messagesHandler.getRank()<<" received in receiveMessages:"<<endl;
				for(map<int,int>::iterator i=receivedTags.begin();i!=receivedTags.end();i++){
					int tag=i->first;
					int count=i->second;
					cout<<"Rank "<<m_messagesHandler.getRank()<<"        "<<MESSAGES[tag]<<"	"<<count<<endl;
				}
			}

			if(sentTagsInProcessMessages.size() > 0 && profilerVerbose){
				cout<<"Rank "<<m_messagesHandler.getRank()<<" sent in processMessages:"<<endl;
				for(map<int,int>::iterator i=sentTagsInProcessMessages.begin();i!=sentTagsInProcessMessages.end();i++){
					int tag=i->first;
					int count=i->second;
					cout<<"Rank "<<m_messagesHandler.getRank()<<"        "<<MESSAGES[tag]<<"	"<<count<<endl;
				}

/*
				int average1=getAverage(&distancesForProcessMessages);
				int deviation1=getStandardDeviation(&distancesForProcessMessages);
			
				cout<<"Rank "<<m_messagesHandler.getRank()<<" distance between processMessages messages: average= "<<average1<<", stddev= "<<deviation1<<
					", n= "<<distancesForProcessMessages.size()<<endl;
				
*/
				#ifdef FULL_DISTRIBUTION
				map<int,int> distribution1;
				for(int i=0;i<(int)distancesForProcessMessages.size();i++){
					distribution1[distancesForProcessMessages[i]]++;
				}
				cout<<"Rank "<<m_messagesHandler.getRank()<<" distribution: "<<endl;
				for(map<int,int>::iterator i=distribution1.begin();i!=distribution1.end();i++){
					cout<<i->first<<" "<<i->second<<endl;
				}
				#endif

			}

			distancesForProcessMessages.clear();

			if(sentTagsInProcessData.size() > 0 && profilerVerbose){
				cout<<"Rank "<<m_messagesHandler.getRank()<<" sent in processData:"<<endl;
				for(map<int,int>::iterator i=sentTagsInProcessData.begin();i!=sentTagsInProcessData.end();i++){
					int tag=i->first;
					int count=i->second;
					cout<<"Rank "<<m_messagesHandler.getRank()<<"        "<<MESSAGES[tag]<<"	"<<count<<endl;
				}
/*
				int average2=getAverage(&distancesForProcessData);
				int deviation2=getStandardDeviation(&distancesForProcessData);
	
				cout<<"Rank "<<m_messagesHandler.getRank()<<" distance between processData messages: average= "<<average2<<", stddev= "<<deviation2<<
					", n= "<<distancesForProcessData.size()<<endl;
				
*/
				#ifdef FULL_DISTRIBUTION
				map<int,int> distribution2;
				for(int i=0;i<(int)distancesForProcessData.size();i++){
					distribution2[distancesForProcessData[i]]++;
				}
				cout<<"Rank "<<m_messagesHandler.getRank()<<" distribution: "<<endl;
				for(map<int,int>::iterator i=distribution2.begin();i!=distribution2.end();i++){
					cout<<i->first<<" "<<i->second<<endl;
				}
				#endif

			}

			distancesForProcessData.clear();

			sentMessages=0;
			sentMessagesInProcessMessages=0;
			sentMessagesInProcessData=0;
			receivedMessages=0;
			receivedTags.clear();
			sentTagsInProcessMessages.clear();
			sentTagsInProcessData.clear();
			ticks=0;

			lastTime=t;
		}

		/* collect some statistics for the profiler */

		// 1. receive the message (0 or 1 message is received)
		receiveMessages(); 
		receivedMessages+=m_inbox.size();
		
		for(int i=0;i<(int)m_inbox.size();i++){
			// stript routing information, if any
			uint8_t tag=m_inbox[i]->getTag();
			receivedTags[tag]++;
		}

		// 2. process the received message, if any
		processMessages();

		int messagesSentInProcessMessages=m_outbox.size();
		sentMessagesInProcessMessages += messagesSentInProcessMessages;
		sentMessages += messagesSentInProcessMessages;

/*
		if(messagesSentInProcessMessages > 0){
			int distance=t- lastTickWhenSentMessageInProcessMessage;
			lastTickWhenSentMessageInProcessMessage=t;
			distancesForProcessMessages.push_back(distance);
		}
*/

		// 3. process data according to current slave and master modes

		int currentSlaveMode=m_switchMan.getSlaveMode();

		uint64_t startingTime = getThreadMicroseconds();
		processData();
		uint64_t endingTime = getThreadMicroseconds();

		int difference = endingTime - startingTime;
		
		m_profiler.addGranularity(currentSlaveMode,difference);

		/* threshold to say something is taking too long */
		/* in microseconds */
		int tooLong=m_profiler.getThreshold();

		if(difference >= tooLong){
			cout<<"Warning, SlaveMode= "<<SLAVE_MODES[currentSlaveMode]<<" GranularityInMicroseconds= "<<difference<<""<<endl;
			m_profiler.printStack();
		}

		m_profiler.resetStack();

		int messagesSentInProcessData = m_outbox.size() - messagesSentInProcessMessages;
		sentMessagesInProcessData += messagesSentInProcessData;
		sentMessages += messagesSentInProcessData;

		for(int i=0;i<messagesSentInProcessMessages;i++){
			// stript routing information, if any
			uint8_t tag=m_outbox[i]->getTag();
			sentTagsInProcessMessages[tag]++;
		}

		for(int i=messagesSentInProcessMessages;i<(int)m_outbox.size();i++){
			// stript routing information, if any
			uint8_t tag=m_outbox[i]->getTag();
			sentTagsInProcessData[tag]++;
		}

		// 4. send messages
		sendMessages();

		/* increment ticks */
		ticks++;
	}

	m_profiler.printAllGranularities();
}

void ComputeCore::processMessages(){
	#ifdef ASSERT
	assert(m_inbox.size()>=0&&m_inbox.size()<=1);
	#endif

	if(m_inbox.size()==0)
		return;


	// if routing is enabled, we want to strip the routing tags if it
	// is required
	if(m_router.isEnabled()){
		if(m_router.routeIncomingMessages()){
			// if the message has routing tag, we don't need to process it...
			return;
		}
	}

	Message*message=m_inbox[0];
	Tag messageTag=message->getTag();

	// check if the tag is in the list of slave switches
	m_switchMan.openSlaveModeLocally(messageTag,m_rank);

	m_messageTagHandler.callHandler(messageTag,message);
}

void ComputeCore::sendMessages(){
	// assert that we did not overflow the ring
	#ifdef ASSERT
	if(m_outboxAllocator.getCount() > m_maximumAllocatedOutputBuffers){
		cout<<"Rank "<<m_rank<<" Error, allocated "<<m_outboxAllocator.getCount()<<" buffers, but maximum is ";
		cout<<m_maximumAllocatedOutputBuffers<<endl;
		cout<<" outboxSize= "<<m_outbox.size()<<endl;
		cout<<"This means that too many messages were created in this time slice."<<endl;
	}

	assert(m_outboxAllocator.getCount()<=m_maximumAllocatedOutputBuffers);
	m_outboxAllocator.resetCount();
	int messagesToSend=m_outbox.size();
	if(messagesToSend>m_maximumNumberOfOutboxMessages){
		cout<<"Fatal: "<<messagesToSend<<" messages to send, but max is "<<m_maximumNumberOfOutboxMessages<<endl;
		cout<<"tags=";
		for(int i=0;i<(int)m_outbox.size();i++){
			uint8_t tag=m_outbox[i]->getTag();
			cout<<" "<<MESSAGES[tag]<<endl;
		}
		cout<<endl;
	}

	assert(messagesToSend<=m_maximumNumberOfOutboxMessages);
	if(messagesToSend>m_maximumNumberOfOutboxMessages){
		uint8_t tag=m_outbox[0]->getTag();
		cout<<"Tag="<<tag<<" n="<<messagesToSend<<" max="<<m_maximumNumberOfOutboxMessages<<endl;
	}
	#endif

	// route messages if the router is enabled
	if(m_router.isEnabled()){
		// if message routing is enabled,
		// generate routing tags.
		m_router.routeOutcomingMessages();
	}

	// parameters.showCommunicationEvents() 
	if( m_showCommunicationEvents && m_outbox.size() > 0){
		uint64_t microseconds=getMicroseconds() - m_startingTimeMicroseconds;
		for(int i=0;i<(int)m_outbox.size();i++){
			cout<<"[Communication] "<<microseconds<<" microseconds, SEND ";
			m_outbox[i]->print();
			cout<<endl;
		}
	}

	// finally, send the messages
	m_messagesHandler.sendMessages(&m_outbox);
}

/**
 * receivedMessages receives 0 or 1 messages.
 * If more messages are available to be pumped, they will wait until the
 * next ComputeCore cycle.
 */
void ComputeCore::receiveMessages(){
	m_inbox.clear();
	m_messagesHandler.receiveMessages(&m_inbox,&m_inboxAllocator);

	#ifdef ASSERT
	int receivedMessages=m_inbox.size();
	assert(receivedMessages<=m_maximumNumberOfInboxMessages);
	#endif

	if(m_inbox.size() > 0 && m_showCommunicationEvents){
		uint64_t theTime=getMicroseconds();
		uint64_t microseconds=theTime - m_startingTimeMicroseconds;
		for(int i=0;i<(int)m_inbox.size();i++){
			cout<<"[Communication] "<<microseconds<<" microseconds, RECEIVE ";
			m_inbox[i]->print();
			cout<<endl;
		}
	}
}

/** process data my calling current slave and master methods */
void ComputeCore::processData(){
	// call the master method first
	MasterMode master=m_switchMan.getMasterMode();
	m_masterModeHandler.callHandler(master);
	m_tickLogger.logMasterTick(master);

	// then call the slave method
	SlaveMode slave=m_switchMan.getSlaveMode();
	m_slaveModeHandler.callHandler(slave);
	m_tickLogger.logSlaveTick(slave);
}

void ComputeCore::constructor(int*argc,char***argv){

	m_messagesHandler.constructor(argc,argv);

	m_runProfiler=false;
	m_showCommunicationEvents=false;
	m_profilerVerbose=false;

	m_rank=m_messagesHandler.getRank();
	m_size=m_messagesHandler.getSize();

	m_virtualCommunicator.constructor(m_rank,m_size,&m_outboxAllocator,&m_inbox,&m_outbox);

	/***********************************************************************************/
	/** initialize the VirtualProcessor */
	m_virtualProcessor.constructor(&m_outbox,&m_inbox,&m_outboxAllocator,
		&m_virtualCommunicator);

	m_maximumNumberOfOutboxMessages=m_size;
	m_maximumNumberOfInboxMessages=1;
}

void ComputeCore::enableProfiler(){
	m_runProfiler=true;
}

void ComputeCore::showCommunicationEvents(){
	m_showCommunicationEvents=true;
}

void ComputeCore::enableProfilerVerbosity(){
	m_profilerVerbose=true;
}

MessagesHandler*ComputeCore::getMessagesHandler(){
	return &m_messagesHandler;
}

Profiler*ComputeCore::getProfiler(){
	return &m_profiler;
}

TickLogger*ComputeCore::getTickLogger(){
	return &m_tickLogger;
}

SwitchMan*ComputeCore::getSwitchMan(){
	return &m_switchMan;
}

StaticVector*ComputeCore::getOutbox(){
	return &m_outbox;
}

StaticVector*ComputeCore::getInbox(){
	return &m_inbox;
}

MessageRouter*ComputeCore::getRouter(){
	return &m_router;
}

RingAllocator*ComputeCore::getOutboxAllocator(){
	return &m_outboxAllocator;
}

RingAllocator*ComputeCore::getInboxAllocator(){
	return &m_inboxAllocator;
}

bool*ComputeCore::getLife(){
	return &m_alive;
}

VirtualProcessor*ComputeCore::getVirtualProcessor(){
	return &m_virtualProcessor;
}

VirtualCommunicator*ComputeCore::getVirtualCommunicator(){
	return &m_virtualCommunicator;
}

int ComputeCore::getMaximumNumberOfAllocatedOutboxMessages(){
	return m_maximumNumberOfOutboxMessages;
}

int ComputeCore::getMaximumNumberOfAllocatedInboxMessages(){
	return m_maximumNumberOfInboxMessages;
}

void ComputeCore::setMaximumNumberOfOutboxBuffers(int maxNumberOfBuffers){
	m_maximumAllocatedOutputBuffers=maxNumberOfBuffers;

	// we need more buffers
	if(m_maximumNumberOfOutboxMessages < m_maximumAllocatedOutputBuffers)
		m_maximumNumberOfOutboxMessages=m_maximumAllocatedOutputBuffers;
}
