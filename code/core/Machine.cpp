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

#include <memory/malloc_types.h>
#include <graph/GridTableIterator.h>
#include <cryptography/crypto.h>
#include <core/OperatingSystem.h>
#include <structures/SplayNode.h>
#include <core/Machine.h>
#include <core/statistics.h>
#include <assembler/VerticesExtractor.h>
#include <profiling/Profiler.h>
#include <sstream>
#include <communication/Message.h>
#include <time.h>
#include <heuristics/TipWatchdog.h>
#include <heuristics/BubbleTool.h>
#include <assert.h>
#include <core/common_functions.h>
#include <iostream>
#include <fstream>
#include <graph/CoverageDistribution.h>
#include <string.h>
#include <structures/SplayTreeIterator.h>
#include <structures/Read.h>
#include <assembler/Loader.h>
#include <memory/MyAllocator.h>
#include <core/constants.h>
#include <algorithm>

using namespace std;

/* Pick-up the option -show-communication-events */

Machine::Machine(int argc,char**argv){
	m_messagesHandler.constructor(&argc,&argv);
	m_rank=m_messagesHandler.getRank();
	m_size=m_messagesHandler.getSize();

	m_switchMan.constructor(m_size);

	if(isMaster() && argc==1){
		m_parameters.showUsage();
		exit(EXIT_NEEDS_ARGUMENTS);
	}else if(argc==2){
		string param=argv[1];
		if(param.find("help")!=string::npos){
			m_parameters.showUsage();
			m_messagesHandler.destructor();
			exit(EXIT_NEEDS_ARGUMENTS);
		}else if(param.find("usage")!=string::npos){
			m_parameters.showUsage();
			m_messagesHandler.destructor();
			exit(EXIT_NEEDS_ARGUMENTS);
		}else if(param.find("man")!=string::npos){
			m_parameters.showUsage();
			m_messagesHandler.destructor();
			exit(EXIT_NEEDS_ARGUMENTS);
		}else if(param.find("-version")!=string::npos){
			showRayVersionShort();
			m_messagesHandler.destructor();
			exit(EXIT_NEEDS_ARGUMENTS);
		}
	}

	cout<<"Rank "<<m_rank<<": Rank= "<<m_rank<<" Size= "<<m_size<<" ProcessIdentifier= "<<portableProcessId()<<" ProcessorName= "<<*(m_messagesHandler.getName())<<endl;

	m_argc=argc;
	m_argv=argv;
	m_bubbleData=new BubbleData();
	#ifdef SHOW_SENT_MESSAGES
	m_stats=new StatisticsData();
	#endif
	m_fusionData=new FusionData();
	m_seedingData=new SeedingData();

	m_ed=new ExtensionData();

	assignMasterHandlers();
	assignSlaveHandlers();
}

void Machine::start(){
	m_partitioner.constructor(&m_outboxAllocator,&m_inbox,&m_outbox,&m_parameters,&m_switchMan);

	m_searcher.constructor(&m_parameters,&m_outbox,&m_timePrinter,&m_switchMan,&m_virtualCommunicator,&m_inbox,
		&m_outboxAllocator);

	m_startingTimeMicroseconds = getMicroseconds();

	m_initialisedAcademy=false;
	m_initialisedKiller=false;
	m_coverageInitialised=false;
	m_writeKmerInitialised=false;
	m_timePrinter.constructor();
	m_ready=true;
	m_fusionData->m_fusionStarted=false;
	m_ed->m_EXTENSION_numberOfRanksDone=0;
	m_messageSentForEdgesDistribution=false;
	m_numberOfRanksDoneSeeding=0;
	m_numberOfMachinesReadyForEdgesDistribution=0;
	m_aborted=false;
	m_readyToSeed=0;
	m_wordSize=-1;
	m_reverseComplementVertex=false;
	m_last_value=0;
	m_mode_send_ingoing_edges=false;
	m_mode_send_vertices=false;
	m_mode_sendDistribution=false;
	m_mode_send_outgoing_edges=false;
	m_mode_send_vertices_sequence_id=0;
	m_mode_send_vertices_sequence_id_position=0;
	m_numberOfMachinesDoneSendingVertices=0;
	m_numberOfMachinesDoneSendingEdges=0;
	m_numberOfMachinesReadyToSendDistribution=0;
	m_numberOfMachinesDoneSendingCoverage=0;
	m_messageSentForVerticesDistribution=false;
	m_sequence_ready_machines=0;
	m_isFinalFusion=false;

	m_messagesHandler.barrier();

	if(isMaster()){
		cout<<endl<<"**************************************************"<<endl;
    		cout<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    		cout<<"This is free software, and you are welcome to redistribute it"<<endl;
    		cout<<"under certain conditions; see \"LICENSE\" for details."<<endl;
		cout<<"**************************************************"<<endl;
		cout<<endl;
		cout<<"Ray Copyright (C) 2010, 2011  Sébastien Boisvert, François Laviolette, Jacques Corbeil"<<endl;
		cout<<"Centre de recherche en infectiologie de l'Université Laval"<<endl;
		cout<<"Project funded by the Canadian Institutes of Health Research (Doctoral award 200902CGM-204212-172830 to S.B.)"<<endl;
 		cout<<"http://denovoassembler.sf.net/"<<endl<<endl;

		cout<<"Reference to cite: "<<endl<<endl;
		cout<<"Sébastien Boisvert, François Laviolette & Jacques Corbeil."<<endl;
		cout<<"Ray: simultaneous assembly of reads from a mix of high-throughput sequencing technologies."<<endl;
		cout<<"Journal of Computational Biology (Mary Ann Liebert, Inc. publishers, New York, U.S.A.)."<<endl;
		cout<<"November 2010, Volume 17, Issue 11, Pages 1519-1533."<<endl;
		cout<<"doi:10.1089/cmb.2009.0238"<<endl;
		cout<<"http://dx.doi.org/doi:10.1089/cmb.2009.0238"<<endl;
		cout<<endl;
	}

	m_messagesHandler.barrier();

	m_parameters.setSize(getSize());
	MAX_ALLOCATED_MESSAGES_IN_OUTBOX=getSize();
	MAX_ALLOCATED_MESSAGES_IN_INBOX=1;

	// this peak is attained in VerticesExtractor::deleteVertices
	m_maximumAllocatedOutputBuffers = MAX_ALLOCATED_OUTPUT_BUFFERS; 

	if(MAX_ALLOCATED_MESSAGES_IN_OUTBOX<m_maximumAllocatedOutputBuffers){
		MAX_ALLOCATED_MESSAGES_IN_OUTBOX=m_maximumAllocatedOutputBuffers;
	}

	m_inboxAllocator.constructor(MAX_ALLOCATED_MESSAGES_IN_INBOX,MAXIMUM_MESSAGE_SIZE_IN_BYTES,
		RAY_MALLOC_TYPE_INBOX_ALLOCATOR,m_parameters.showMemoryAllocations());
	m_outboxAllocator.constructor(m_maximumAllocatedOutputBuffers,MAXIMUM_MESSAGE_SIZE_IN_BYTES,
		RAY_MALLOC_TYPE_OUTBOX_ALLOCATOR,m_parameters.showMemoryAllocations());

	m_inbox.constructor(MAX_ALLOCATED_MESSAGES_IN_INBOX,RAY_MALLOC_TYPE_INBOX_VECTOR,m_parameters.showMemoryAllocations());
	m_outbox.constructor(MAX_ALLOCATED_MESSAGES_IN_OUTBOX,RAY_MALLOC_TYPE_OUTBOX_VECTOR,m_parameters.showMemoryAllocations());

	m_scaffolder.constructor(&m_outbox,&m_inbox,&m_outboxAllocator,&m_parameters,
	&m_virtualCommunicator,&m_switchMan);
	m_scaffolder.setTimePrinter(&m_timePrinter);

	m_edgePurger.constructor(&m_outbox,&m_inbox,&m_outboxAllocator,&m_parameters,m_switchMan.getSlaveModePointer(),m_switchMan.getMasterModePointer(),
	&m_virtualCommunicator,&m_subgraph,&m_virtualProcessor);

	m_coverageGatherer.constructor(&m_parameters,&m_inbox,&m_outbox,m_switchMan.getSlaveModePointer(),&m_subgraph,
		&m_outboxAllocator);


	m_fusionTaskCreator.constructor(&m_virtualProcessor,&m_outbox,
		&m_outboxAllocator,m_switchMan.getSlaveModePointer(),&m_parameters,&(m_ed->m_EXTENSION_contigs),
		&(m_ed->m_EXTENSION_identifiers),&(m_fusionData->m_FUSION_eliminated),
		&m_virtualCommunicator);

	m_joinerTaskCreator.constructor(&m_virtualProcessor,&m_outbox,
		&m_outboxAllocator,m_switchMan.getSlaveModePointer(),&m_parameters,&(m_ed->m_EXTENSION_contigs),
		&(m_ed->m_EXTENSION_identifiers),&(m_fusionData->m_FUSION_eliminated),
		&m_virtualCommunicator,&(m_fusionData->m_FINISH_newFusions));


	m_amos.constructor(&m_parameters,&m_outboxAllocator,&m_outbox,m_fusionData,m_ed,m_switchMan.getMasterModePointer(),m_switchMan.getSlaveModePointer(),&m_scaffolder,
		&m_inbox,&m_virtualCommunicator);

	m_mp.assignHandlers(&m_scriptEngine);

	m_mp.setScaffolder(&m_scaffolder);
	m_mp.setVirtualCommunicator(&m_virtualCommunicator);
	m_mp.setSwitchMan(&m_switchMan);

	m_switchMan.setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);

	m_mode_AttachSequences=false;
	m_startEdgeDistribution=false;

	m_ranksDoneAttachingReads=0;

	m_messagesHandler.barrier();

	int maximumNumberOfProcesses=65536;
	if(getSize()>maximumNumberOfProcesses){
		cout<<"The maximum number of processes is "<<maximumNumberOfProcesses<<" (this can be changed in the code)"<<endl;
	}
	assert(getSize()<=maximumNumberOfProcesses);

	bool fullReport=false;

	if(m_argc==2){
		string argument=m_argv[1];
		if(argument=="-version" || argument=="--version")
			fullReport=true;
	}

	if(isMaster()){
		showRayVersion(&m_messagesHandler,fullReport);

		if(!fullReport){
			cout<<endl;
		}
	}

	/** only show the version. */
	if(fullReport){
		m_messagesHandler.destructor();
		return;
	}

	m_parameters.constructor(m_argc,m_argv,getRank());

	// initiate the network test.
	m_networkTest.constructor(m_rank,m_size,&m_inbox,&m_outbox,&m_parameters,&m_outboxAllocator,m_messagesHandler.getName(),
		&m_timePrinter);

	m_networkTest.setSwitchMan(&m_switchMan);

	int PERSISTENT_ALLOCATOR_CHUNK_SIZE=4194304; // 4 MiB
	m_persistentAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE,RAY_MALLOC_TYPE_PERSISTENT_DATA_ALLOCATOR,
		m_parameters.showMemoryAllocations());

	int directionAllocatorChunkSize=4194304; // 4 MiB

	m_directionsAllocator.constructor(directionAllocatorChunkSize,RAY_MALLOC_TYPE_WAVE_ALLOCATOR,
		m_parameters.showMemoryAllocations());

	/** create the directory for the assembly */
	
	string directory=m_parameters.getPrefix();
	if(fileExists(directory.c_str())){
		if(m_parameters.getRank() == MASTER_RANK)
			cout<<"Error, "<<directory<<" already exists, change the -o parameter to another value."<<endl;

		m_messagesHandler.destructor();
		return;
	}

	m_messagesHandler.barrier();
	
	// create the directory
	if(m_parameters.getRank() == MASTER_RANK){
		createDirectory(directory.c_str());
		m_parameters.writeCommandFile();

		m_timePrinter.setFile(m_parameters.getPrefix());
	}

	// options are loaded from here
	// plus the directory exists now
	if(m_parameters.hasOption("-route-messages")){
		m_router.enable(&m_inbox,&m_outbox,&m_outboxAllocator,m_parameters.getRank(),
			m_parameters.getPrefix(),m_parameters.getSize(),
			m_parameters.getConnectionType(),m_parameters.getRoutingDegree());

		// update the connections
		vector<int> connections;
		m_router.getGraph()->getIncomingConnections(m_parameters.getRank(),&connections);
		m_messagesHandler.setConnections(&connections);

		// terminal control messages can not be routed.
		m_router.addTagToCheckForRelayFrom0(RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON);
		m_router.addTagToCheckForRelayTo0(RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY);
	}

	m_seedExtender.constructor(&m_parameters,&m_directionsAllocator,m_ed,&m_subgraph,&m_inbox,&m_profiler2,
		&m_outbox,m_seedingData,m_switchMan.getSlaveModePointer());

	m_profiler = &m_profiler2;
	m_profiler->constructor(m_parameters.runProfiler());

	m_verticesExtractor.setProfiler(m_profiler);
	m_kmerAcademyBuilder.setProfiler(m_profiler);
	m_edgePurger.setProfiler(m_profiler);

	ostringstream prefixFull;
	prefixFull<<m_parameters.getMemoryPrefix()<<"_Main";
	int chunkSize=16777216;
	m_diskAllocator.constructor(chunkSize,RAY_MALLOC_TYPE_DATA_ALLOCATOR,
		m_parameters.showMemoryAllocations());

	m_sl.constructor(m_size,&m_diskAllocator,&m_myReads);

	m_fusionData->constructor(getSize(),MAXIMUM_MESSAGE_SIZE_IN_BYTES,getRank(),&m_outbox,&m_outboxAllocator,m_parameters.getWordSize(),
		m_ed,m_seedingData,m_switchMan.getSlaveModePointer(),&m_parameters);

	m_virtualCommunicator.constructor(m_rank,m_size,&m_outboxAllocator,&m_inbox,&m_outbox);

	m_scriptEngine.configureVirtualCommunicator(&m_virtualCommunicator);

	/***********************************************************************************/
	/** initialize the VirtualProcessor */
	m_virtualProcessor.constructor(&m_outbox,&m_inbox,&m_outboxAllocator,&m_parameters,
		&m_virtualCommunicator);


	m_scriptEngine.configureSwitchMan(&m_switchMan);

	m_library.constructor(getRank(),&m_outbox,&m_outboxAllocator,&m_sequence_id,&m_sequence_idInFile,
		m_ed,getSize(),&m_timePrinter,m_switchMan.getSlaveModePointer(),m_switchMan.getMasterModePointer(),
	&m_parameters,&m_fileId,m_seedingData,&m_inbox,&m_virtualCommunicator);

	m_subgraph.constructor(getRank(),&m_parameters);
	
	m_seedingData->constructor(&m_seedExtender,getRank(),getSize(),&m_outbox,&m_outboxAllocator,m_switchMan.getSlaveModePointer(),&m_parameters,&m_wordSize,&m_subgraph,&m_inbox,&m_virtualCommunicator);

	m_alive=true;
	m_loadSequenceStep=false;
	m_totalLetters=0;

	m_messagesHandler.barrier();

	if(m_parameters.showMemoryUsage()){
		showMemoryUsage(getRank());
	}

	m_messagesHandler.barrier();

	if(isMaster()){
		cout<<endl;
	}

	m_mp.constructor(
&m_router,
m_seedingData,
&m_library,&m_ready,
&m_verticesExtractor,
&m_sl,
			m_ed,
			&m_numberOfRanksDoneDetectingDistances,
			&m_numberOfRanksDoneSendingDistances,
			&m_parameters,
			&m_subgraph,
			&m_outboxAllocator,
			getRank(),
			&m_numberOfMachinesDoneSendingEdges,
			m_fusionData,
			&m_wordSize,
			&m_myReads,
		getSize(),
	&m_inboxAllocator,
	&m_persistentAllocator,
	&m_identifiers,
	&m_mode_sendDistribution,
	&m_alive,
	m_switchMan.getSlaveModePointer(),
	&m_allPaths,
	&m_last_value,
	&m_ranksDoneAttachingReads,
	&m_DISTRIBUTE_n,
	&m_numberOfRanksDoneSeeding,
	&m_CLEAR_n,
	&m_readyToSeed,
	&m_FINISH_n,
	&m_reductionOccured,
	&m_directionsAllocator,
	&m_mode_send_coverage_iterator,
	&m_coverageDistribution,
	&m_sequence_ready_machines,
	&m_numberOfMachinesReadyForEdgesDistribution,
	&m_numberOfMachinesReadyToSendDistribution,
	&m_mode_send_outgoing_edges,
	&m_mode_send_vertices_sequence_id,
	&m_mode_send_vertices,
	&m_numberOfMachinesDoneSendingVertices,
	&m_numberOfMachinesDoneSendingCoverage,
				&m_outbox,&m_inbox,
	&m_oa,
	&m_numberOfRanksWithCoverageData,&m_seedExtender,
	m_switchMan.getMasterModePointer(),&m_isFinalFusion,&m_si);

	if(m_argc==1||((string)m_argv[1])=="--help"){
		if(isMaster()){
			m_aborted=true;
			m_parameters.showUsage();
		}
	}else{
		if(isMaster()){
			m_switchMan.setMasterMode(RAY_MASTER_MODE_LOAD_CONFIG);
			m_sl.constructor(getSize(),&m_diskAllocator,
				&m_myReads);
		}

		m_lastTime=time(NULL);
		run();
	}

	if(isMaster() && !m_aborted){
		m_timePrinter.printDurations();

		cout<<endl;
	}

	m_messagesHandler.barrier();

	if(m_parameters.showMemoryUsage()){
		showMemoryUsage(getRank());
	}

	m_messagesHandler.barrier();

	if(isMaster() && !m_aborted && !m_parameters.hasOption("-test-network-only")){
		m_scaffolder.printFinalMessage();
		cout<<endl;
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getOutputFile()<<endl;
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getScaffoldFile()<<endl;
		cout<<"Check for "<<m_parameters.getPrefix()<<"*"<<endl;
		cout<<endl;
		if(m_parameters.useAmos()){
			cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getAmosFile()<<" (reads mapped onto contiguous sequences in AMOS format)"<<endl;

		}
		cout<<endl;
	}

	m_messagesHandler.barrier();
	m_messagesHandler.freeLeftovers();
	m_persistentAllocator.clear();
	m_directionsAllocator.clear();
	m_inboxAllocator.clear();
	m_outboxAllocator.clear();

	m_diskAllocator.clear();

	m_messagesHandler.destructor();
}

void Machine::call_RAY_MASTER_MODE_WRITE_SCAFFOLDS(){
	m_scaffolder.call_RAY_MASTER_MODE_WRITE_SCAFFOLDS();
}

/**
 * runWithProfiler if -run-profiler is provided
 * otherwise, run runVanilla()
 */
void Machine::run(){
	if(m_parameters.runProfiler()){
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
void Machine::runVanilla(){
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
void Machine::runWithProfiler(){
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

	bool profilerVerbose=m_parameters.hasOption("-with-profiler-details");

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

				m_profiler->printGranularities(m_rank);
			}

			m_profiler->clearGranularities();

			if(receivedTags.size() > 0 && profilerVerbose){
				cout<<"Rank "<<m_parameters.getRank()<<" received in receiveMessages:"<<endl;
				for(map<int,int>::iterator i=receivedTags.begin();i!=receivedTags.end();i++){
					int tag=i->first;
					int count=i->second;
					cout<<"Rank "<<m_parameters.getRank()<<"        "<<MESSAGES[tag]<<"	"<<count<<endl;
				}
			}

			if(sentTagsInProcessMessages.size() > 0 && profilerVerbose){
				cout<<"Rank "<<m_parameters.getRank()<<" sent in processMessages:"<<endl;
				for(map<int,int>::iterator i=sentTagsInProcessMessages.begin();i!=sentTagsInProcessMessages.end();i++){
					int tag=i->first;
					int count=i->second;
					cout<<"Rank "<<m_parameters.getRank()<<"        "<<MESSAGES[tag]<<"	"<<count<<endl;
				}

/*
				int average1=getAverage(&distancesForProcessMessages);
				int deviation1=getStandardDeviation(&distancesForProcessMessages);
			
				cout<<"Rank "<<m_parameters.getRank()<<" distance between processMessages messages: average= "<<average1<<", stddev= "<<deviation1<<
					", n= "<<distancesForProcessMessages.size()<<endl;
				
*/
				#ifdef FULL_DISTRIBUTION
				map<int,int> distribution1;
				for(int i=0;i<(int)distancesForProcessMessages.size();i++){
					distribution1[distancesForProcessMessages[i]]++;
				}
				cout<<"Rank "<<m_parameters.getRank()<<" distribution: "<<endl;
				for(map<int,int>::iterator i=distribution1.begin();i!=distribution1.end();i++){
					cout<<i->first<<" "<<i->second<<endl;
				}
				#endif

			}

			distancesForProcessMessages.clear();

			if(sentTagsInProcessData.size() > 0 && profilerVerbose){
				cout<<"Rank "<<m_parameters.getRank()<<" sent in processData:"<<endl;
				for(map<int,int>::iterator i=sentTagsInProcessData.begin();i!=sentTagsInProcessData.end();i++){
					int tag=i->first;
					int count=i->second;
					cout<<"Rank "<<m_parameters.getRank()<<"        "<<MESSAGES[tag]<<"	"<<count<<endl;
				}
/*
				int average2=getAverage(&distancesForProcessData);
				int deviation2=getStandardDeviation(&distancesForProcessData);
	
				cout<<"Rank "<<m_parameters.getRank()<<" distance between processData messages: average= "<<average2<<", stddev= "<<deviation2<<
					", n= "<<distancesForProcessData.size()<<endl;
				
*/
				#ifdef FULL_DISTRIBUTION
				map<int,int> distribution2;
				for(int i=0;i<(int)distancesForProcessData.size();i++){
					distribution2[distancesForProcessData[i]]++;
				}
				cout<<"Rank "<<m_parameters.getRank()<<" distribution: "<<endl;
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
		
		m_profiler->addGranularity(currentSlaveMode,difference);

		/* threshold to say something is taking too long */
		/* in microseconds */
		int tooLong=m_profiler->getThreshold();

		if(difference >= tooLong){
			cout<<"Warning, SlaveMode= "<<SLAVE_MODES[currentSlaveMode]<<" GranularityInMicroseconds= "<<difference<<""<<endl;
			m_profiler->printStack();
		}

		m_profiler->resetStack();

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

	m_profiler->printAllGranularities();
}

int Machine::getRank(){
	return m_rank;
}

void Machine::processMessages(){
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

	// check if the tag is in the list of slave switches
	m_switchMan.openSlaveModeLocally(m_inbox[0]->getTag(),m_rank);

	// process the message as is
	m_mp.processMessage((m_inbox[0]));
}

void Machine::sendMessages(){
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
	if(messagesToSend>MAX_ALLOCATED_MESSAGES_IN_OUTBOX){
		cout<<"Fatal: "<<messagesToSend<<" messages to send, but max is "<<MAX_ALLOCATED_MESSAGES_IN_OUTBOX<<endl;
		cout<<"tags=";
		for(int i=0;i<(int)m_outbox.size();i++){
			uint8_t tag=m_outbox[i]->getTag();
			cout<<" "<<MESSAGES[tag]<<endl;
		}
		cout<<endl;
	}

	assert(messagesToSend<=MAX_ALLOCATED_MESSAGES_IN_OUTBOX);
	if(messagesToSend>MAX_ALLOCATED_MESSAGES_IN_OUTBOX){
		uint8_t tag=m_outbox[0]->getTag();
		cout<<"Tag="<<tag<<" n="<<messagesToSend<<" max="<<MAX_ALLOCATED_MESSAGES_IN_OUTBOX<<endl;
	}
	#endif

	if(m_router.isEnabled()){
		// if message routing is enabled,
		// generate routing tags.
		m_router.routeOutcomingMessages();
	}

	if(m_outbox.size() > 0 && m_parameters.showCommunicationEvents() /* && SlaveMode = RAY_SLAVE_MODE_EXTENSION*/){
		uint64_t microseconds=getMicroseconds() - m_startingTimeMicroseconds;
		for(int i=0;i<(int)m_outbox.size();i++){
			cout<<"[Communication] "<<microseconds<<" microseconds, SEND ";
			m_outbox[i]->print();
			cout<<endl;
		}
	}

	m_messagesHandler.sendMessages(&m_outbox);
}

/**
 * receivedMessages receives 0 or 1 messages.
 * If more messages are available to be pumped, they will wait until the
 * next Machine cycle.
 */
void Machine::receiveMessages(){
	m_inbox.clear();
	m_messagesHandler.receiveMessages(&m_inbox,&m_inboxAllocator);

	#ifdef ASSERT
	int receivedMessages=m_inbox.size();
	assert(receivedMessages<=MAX_ALLOCATED_MESSAGES_IN_INBOX);
	#endif

	if(m_inbox.size() > 0 && m_parameters.showCommunicationEvents()){
		uint64_t theTime=getMicroseconds();
		uint64_t microseconds=theTime - m_startingTimeMicroseconds;
		for(int i=0;i<(int)m_inbox.size();i++){
			cout<<"[Communication] "<<microseconds<<" microseconds, RECEIVE ";
			m_inbox[i]->print();
			cout<<endl;
		}
	}
}

void Machine::call_RAY_SLAVE_MODE_SEND_SEED_LENGTHS(){
	m_seedingData->call_RAY_SLAVE_MODE_SEND_SEED_LENGTHS();
}

void Machine::call_RAY_MASTER_MODE_LOAD_CONFIG(){

	if(m_argc==2 && m_argv[1][0]!='-'){
		ifstream f(m_argv[1]);
		if(!f){
			cout<<"Rank "<<getRank()<<" invalid input file."<<endl;
			m_parameters.showUsage();
			m_aborted=true;
			f.close();
			m_switchMan.setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
			return;
		}
	}

	if(m_parameters.getError()){
		m_aborted=true;
		m_switchMan.setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}

	uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(2*sizeof(uint64_t));
	message[0]=m_parameters.getWordSize();
	message[1]=m_parameters.getColorSpaceMode();

	for(int i=0;i<getSize();i++){
		Message aMessage(message,2,i,RAY_MPI_TAG_SET_WORD_SIZE,getRank());
		m_outbox.push_back(aMessage);
	}

	m_switchMan.setMasterMode(RAY_MASTER_MODE_TEST_NETWORK);
}

void Machine::call_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS(){
	m_searcher.call_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS();
}

void Machine::call_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS(){
	m_searcher.call_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS();
}

void Machine::call_RAY_SLAVE_MODE_COUNT_FILE_ENTRIES(){
	m_networkTest.writeData();

	m_partitioner.call_RAY_SLAVE_MODE_COUNT_FILE_ENTRIES();
}

void Machine::call_RAY_MASTER_MODE_COUNT_FILE_ENTRIES(){
	m_partitioner.call_RAY_MASTER_MODE_COUNT_FILE_ENTRIES();
}

/** actually, call_RAY_MASTER_MODE_LOAD_SEQUENCES 
 * writes the AMOS file */
void Machine::call_RAY_MASTER_MODE_LOAD_SEQUENCES(){

	m_timePrinter.printElapsedTime("Counting sequences to assemble");
	cout<<endl;

	/** this won't write anything if -amos was not provided */
	bool res=m_sl.writeSequencesToAMOSFile(getRank(),getSize(),
	&m_outbox,
	&m_outboxAllocator,
	&m_loadSequenceStep,
	m_bubbleData,
	&m_lastTime,
	&m_parameters,m_switchMan.getMasterModePointer(),m_switchMan.getSlaveModePointer()
);
	if(!res){
		m_aborted=true;
		m_switchMan.setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
		m_switchMan.setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}

	uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	uint32_t*messageInInts=(uint32_t*)message;
	messageInInts[0]=m_parameters.getNumberOfFiles();

	for(int i=0;i<(int)m_parameters.getNumberOfFiles();i++){
		messageInInts[1+i]=(uint64_t)m_parameters.getNumberOfSequences(i);
	}
	
	for(int i=0;i<getSize();i++){
		Message aMessage(message,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
		i,RAY_MPI_TAG_LOAD_SEQUENCES,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_SLAVE_MODE_LOAD_SEQUENCES(){
	// TODO: initialise this parameters in the constructor

	m_sl.call_RAY_SLAVE_MODE_LOAD_SEQUENCES(getRank(),getSize(),
	&m_outbox,
	&m_outboxAllocator,
	&m_loadSequenceStep,
	m_bubbleData,
	&m_lastTime,
	&m_parameters,m_switchMan.getMasterModePointer(),m_switchMan.getSlaveModePointer()
);
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION(){
	m_timePrinter.printElapsedTime("Sequence loading");
	cout<<endl;
	
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_MASTER_MODE_SEND_COVERAGE_VALUES(){
	if(m_parameters.hasCheckpoint("GenomeGraph")){
		cout<<"Rank "<<m_parameters.getRank()<<" is reading checkpoint CoverageDistribution"<<endl;
		m_coverageDistribution.clear();
		ifstream f(m_parameters.getCheckpointFile("CoverageDistribution").c_str());
		int n=0;
		f.read((char*)&n,sizeof(int));
		int coverage=0;
		uint64_t count=0;
		for(int i=0;i<n;i++){
			f.read((char*)&coverage,sizeof(int));
			f.read((char*)&count,sizeof(uint64_t));
			m_coverageDistribution[coverage]=count;
		}
		f.close();
	}

	if(m_parameters.writeCheckpoints() && !m_parameters.hasCheckpoint("CoverageDistribution")){
		cout<<"Rank "<<m_parameters.getRank()<<" is writing checkpoint CoverageDistribution"<<endl;
		ofstream f(m_parameters.getCheckpointFile("CoverageDistribution").c_str());
		int theSize=m_coverageDistribution.size();
		f.write((char*)&theSize,sizeof(int));
		for(map<int,uint64_t>::iterator i=m_coverageDistribution.begin();i!=m_coverageDistribution.end();i++){
			int coverage=i->first;
			uint64_t count=i->second;
			f.write((char*)&coverage,sizeof(int));
			f.write((char*)&count,sizeof(uint64_t));
		}
		f.close();
	}

	if(m_coverageDistribution.size()==0){
		cout<<endl;
		cout<<"Rank 0: Assembler panic: no k-mers found in reads."<<endl;
		cout<<"Rank 0: Perhaps reads are shorter than the k-mer length (change -k)."<<endl;
		m_aborted=true;
		m_switchMan.setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}
	m_numberOfMachinesDoneSendingCoverage=-1;
	string file=m_parameters.getCoverageDistributionFile();
	CoverageDistribution distribution(&m_coverageDistribution,&file);

	m_parameters.setMinimumCoverage(distribution.getMinimumCoverage());
	m_parameters.setPeakCoverage(distribution.getPeakCoverage());
	m_parameters.setRepeatCoverage(distribution.getRepeatCoverage());

	printf("\n");
	fflush(stdout);

	cout<<endl;
	cout<<"Rank "<<getRank()<<": the minimum coverage is "<<m_parameters.getMinimumCoverage()<<endl;
	cout<<"Rank "<<getRank()<<": the peak coverage is "<<m_parameters.getPeakCoverage()<<endl;

	uint64_t numberOfVertices=0;
	uint64_t verticesWith1Coverage=0;
	int lowestCoverage=9999;
	
	uint64_t genomeKmers=0;

	for(map<int,uint64_t>::iterator i=m_coverageDistribution.begin();
		i!=m_coverageDistribution.end();i++){
		int coverageValue=i->first;
		uint64_t vertices=i->second;
		if(coverageValue<lowestCoverage){
			verticesWith1Coverage=vertices;
			lowestCoverage=coverageValue;
		}
		if(coverageValue>=m_parameters.getMinimumCoverage()){
			genomeKmers+=vertices;
		}
		numberOfVertices+=vertices;
	}
	double percentageSeenOnce=(0.0+verticesWith1Coverage)/numberOfVertices*100.00;

	ostringstream g;
	g<<m_parameters.getPrefix();
	g<<"CoverageDistributionAnalysis.txt";
	ofstream outputFile(g.str().c_str());
	outputFile<<"k-mer length:\t"<<m_parameters.getWordSize()<<endl;
	outputFile<<"Lowest coverage observed:\t"<<lowestCoverage<<endl;
	outputFile<<"MinimumCoverage:\t"<<m_parameters.getMinimumCoverage()<<endl;
	outputFile<<"PeakCoverage:\t"<<m_parameters.getPeakCoverage()<<endl;
	outputFile<<"RepeatCoverage:\t"<<m_parameters.getRepeatCoverage()<<endl;
	outputFile<<"Number of k-mers with at least MinimumCoverage:\t"<<genomeKmers<<" k-mers"<<endl;
	outputFile<<"Estimated genome length:\t"<<genomeKmers/2<<" nucleotides"<<endl;
	outputFile<<"Percentage of vertices with coverage "<<lowestCoverage<<":\t"<<percentageSeenOnce<<" %"<<endl;
	outputFile<<"DistributionFile: "<<file<<endl;

	outputFile.close();

	m_coverageDistribution.clear();

	// display a warning
	// for RNA-Seq and for méta-genomes, this is not important
	//
	if(m_parameters.getMinimumCoverage()> m_parameters.getPeakCoverage()
	|| m_parameters.getPeakCoverage()==m_parameters.getRepeatCoverage()
	|| m_parameters.getPeakCoverage()==1){
		cout<<"Warning: no peak observed in the k-mer coverage distribution."<<endl;
		cout<<"to deal with the sequencing error rate, try to lower the k-mer length (-k)"<<endl;
		cout<<"If you are using RNA-Seq or metagenomic data, then you can ignore this warning."<<endl;
	}

	// see these values to everyone.
	uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(3*sizeof(uint64_t));
	buffer[0]=m_parameters.getMinimumCoverage();
	buffer[1]=m_parameters.getPeakCoverage();
	buffer[2]=m_parameters.getRepeatCoverage();

	m_numberOfRanksWithCoverageData=0;
	for(int i=0;i<getSize();i++){
		Message aMessage(buffer,3,i,RAY_MPI_TAG_SEND_COVERAGE_VALUES,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING(){
	(m_numberOfMachinesDoneSendingVertices)=0;
	m_timePrinter.printElapsedTime("Coverage distribution analysis");
	cout<<endl;

	cout<<endl;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_BUILD_GRAPH,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_SLAVE_MODE_BUILD_KMER_ACADEMY(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	if(!m_initialisedAcademy){
		m_kmerAcademyBuilder.constructor(m_size,&m_parameters,&m_subgraph);
		(m_mode_send_vertices_sequence_id)=0;
		m_initialisedAcademy=true;

		m_si.constructor(&m_parameters,&m_outboxAllocator,&m_inbox,&m_outbox,&m_virtualCommunicator);
	}
	
	// TODO: initialise these things in the constructor
	m_kmerAcademyBuilder.call_RAY_SLAVE_MODE_BUILD_KMER_ACADEMY(		&m_mode_send_vertices_sequence_id,
			&m_myReads,
			&m_reverseComplementVertex,
			getRank(),
			&m_outbox,
			&m_inbox,
			&m_mode_send_vertices,
			m_wordSize,
			getSize(),
			&m_outboxAllocator,
			m_switchMan.getSlaveModePointer()
		);


	MACRO_COLLECT_PROFILING_INFORMATION();
}

void Machine::call_RAY_SLAVE_MODE_EXTRACT_VERTICES(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	// TODO: initialise these in the constructor
	m_verticesExtractor.call_RAY_SLAVE_MODE_EXTRACT_VERTICES(		&m_mode_send_vertices_sequence_id,
			&m_myReads,
			&m_reverseComplementVertex,
			getRank(),
			&m_outbox,
			&m_mode_send_vertices,
			m_wordSize,
			getSize(),
			&m_outboxAllocator,
			m_switchMan.getSlaveModePointer()
		);

	MACRO_COLLECT_PROFILING_INFORMATION();
}

void Machine::call_RAY_MASTER_MODE_PURGE_NULL_EDGES(){
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
	m_timePrinter.printElapsedTime("Graph construction");
	cout<<endl;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_PURGE_NULL_EDGES,getRank());
		m_outbox.push_back(aMessage);
	}
}

void Machine::call_RAY_SLAVE_MODE_PURGE_NULL_EDGES(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	m_edgePurger.call_RAY_SLAVE_MODE_PURGE_NULL_EDGES();

	MACRO_COLLECT_PROFILING_INFORMATION();
}


void Machine::call_RAY_MASTER_MODE_WRITE_KMERS(){
	if(!m_writeKmerInitialised){
		m_writeKmerInitialised=true;
		m_coverageRank=0;
		m_numberOfRanksDone=0;
	}else if(m_inbox.size()>0&&m_inbox.at(0)->getTag()==RAY_MPI_TAG_WRITE_KMERS_REPLY){
		uint64_t*buffer=(uint64_t*)m_inbox.at(0)->getBuffer();
		int bufferPosition=0;
		for(int i=0;i<=4;i++){
			for(int j=0;j<=4;j++){
				m_edgeDistribution[i][j]+=buffer[bufferPosition++];
			}
		}
		m_numberOfRanksDone++;
	}else if(m_numberOfRanksDone==m_parameters.getSize()){
		if(m_parameters.writeKmers()){
			cout<<endl;
			cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getPrefix()<<"kmers.txt"<<endl;
		}

		m_switchMan.closeMasterMode();

		if(m_parameters.hasCheckpoint("GenomeGraph"))
			return;

		ostringstream edgeFile;
		edgeFile<<m_parameters.getPrefix()<<"degreeDistribution.txt";
		ofstream f(edgeFile.str().c_str());

		f<<"# Most of the vertices should have an ingoing degree of 1 and an outgoing degree of 1."<<endl;
		f<<"# These are the easy vertices."<<endl;
		f<<"# Then, the most abundant are those with an ingoing degree of 1 and an outgoing degree of 2."<<endl;
		f<<"# Note that vertices with a coverage of 1 are not considered."<<endl;
		f<<"# The option -write-kmers will actually write all the graph to a file if you need more precise data."<<endl;
		f<<"# IngoingDegree\tOutgoingDegree\tNumberOfVertices"<<endl;

		for(int i=0;i<=4;i++){
			for(int j=0;j<=4;j++){
				f<<i<<"\t"<<j<<"\t"<<m_edgeDistribution[i][j]<<endl;
			}
		}
		m_edgeDistribution.clear();
		f.close();
		cout<<"Rank "<<getRank()<<" wrote "<<edgeFile.str()<<endl;
	
	}else if(m_coverageRank==m_numberOfRanksDone){
		Message aMessage(NULL,0,m_coverageRank,RAY_MPI_TAG_WRITE_KMERS,getRank());
		m_outbox.push_back(aMessage);
		m_coverageRank++;
	}
}

void Machine::call_RAY_SLAVE_MODE_WRITE_KMERS(){
	if(m_parameters.writeKmers())
		m_coverageGatherer.writeKmers();
	
	/* send edge distribution */
	GridTableIterator iterator;
	iterator.constructor(&m_subgraph,m_parameters.getWordSize(),&m_parameters);

	map<int,map<int,uint64_t> > distribution;
	while(iterator.hasNext()){
		Vertex*node=iterator.next();
		Kmer key=*(iterator.getKey());
		int parents=node->getIngoingEdges(&key,m_parameters.getWordSize()).size();
		int children=node->getOutgoingEdges(&key,m_parameters.getWordSize()).size();
		distribution[parents][children]++;
	}

	uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int outputPosition=0;
	for(int i=0;i<=4;i++){
		for(int j=0;j<=4;j++){
			buffer[outputPosition++]=distribution[i][j];
		}
	}

	Message aMessage(buffer,outputPosition,MASTER_RANK,RAY_MPI_TAG_WRITE_KMERS_REPLY,getRank());
	m_outbox.push_back(aMessage);
	m_switchMan.setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_INDEXING(){
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
	
	m_timePrinter.printElapsedTime("Null edge purging");
	cout<<endl;

	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_START_INDEXING_SEQUENCES,getRank());
		m_outbox.push_back(aMessage);
	}
}

void Machine::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS(){
	cout<<endl;
	m_numberOfMachinesDoneSendingVertices=-1;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0, i, RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS(){
	if(!m_coverageInitialised){
		m_timePrinter.printElapsedTime("K-mer counting");
		cout<<endl;
		m_coverageInitialised=true;
		m_coverageRank=0;
	}

	for(m_coverageRank=0;m_coverageRank<m_parameters.getSize();m_coverageRank++){
		Message aMessage(NULL,0,m_coverageRank,
			RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_MASTER_MODE_PREPARE_SEEDING(){
	m_ranksDoneAttachingReads=-1;
	m_readyToSeed=getSize();
	
	m_switchMan.closeMasterMode();
}

void Machine::call_RAY_SLAVE_MODE_ASSEMBLE_WAVES(){
	// take each seed, and extend it in both direction using previously obtained information.
	if(m_seedingData->m_SEEDING_i==(uint64_t)m_seedingData->m_SEEDING_seeds.size()){
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE,getRank());
		m_outbox.push_back(aMessage);
	}else{
	}
}

void Machine::call_RAY_SLAVE_MODE_FINISH_FUSIONS(){
	m_joinerTaskCreator.call_RAY_SLAVE_MODE_FINISH_FUSIONS();
}

void Machine::call_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS(){

	// TODO: initialise these things in the constructor
	m_fusionData->call_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS(m_seedingData,m_ed,m_rank,&m_outboxAllocator,&m_outbox,getSize(),m_switchMan.getSlaveModePointer());
}

void Machine::call_RAY_SLAVE_MODE_SEND_DISTRIBUTION(){
	m_coverageGatherer.call_RAY_SLAVE_MODE_SEND_DISTRIBUTION();
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_SEEDING(){
	m_timePrinter.printElapsedTime("Selection of optimal read markers");
	cout<<endl;
	m_readyToSeed=-1;
	m_numberOfRanksDoneSeeding=0;

	// tell everyone to seed now.
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_START_SEEDING,getRank());
		m_outbox.push_back(aMessage);
	}

	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_SLAVE_MODE_START_SEEDING(){
	m_seedingData->call_RAY_SLAVE_MODE_START_SEEDING();
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_DETECTION(){
	m_timePrinter.printElapsedTime("Detection of assembly seeds");
	cout<<endl;
	m_numberOfRanksDoneSeeding=-1;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_numberOfRanksDoneDetectingDistances=0;
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_MASTER_MODE_ASK_DISTANCES(){
	m_numberOfRanksDoneDetectingDistances=-1;
	m_numberOfRanksDoneSendingDistances=0;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_MASTER_MODE_START_UPDATING_DISTANCES(){
	m_numberOfRanksDoneSendingDistances=-1;
	m_parameters.computeAverageDistances();
	m_switchMan.setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
	
	m_fileId=0;
	m_sequence_idInFile=0;
	m_sequence_id=0;

	m_switchMan.closeMasterMode();
}

void Machine::call_RAY_SLAVE_MODE_INDEX_SEQUENCES(){

	// TODO: initialise these things in the constructor
	m_si.call_RAY_SLAVE_MODE_INDEX_SEQUENCES(&m_myReads,&m_outboxAllocator,&m_outbox,m_switchMan.getSlaveModePointer(),m_wordSize,
	m_size,m_rank);
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_EXTENSIONS(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_ASK_EXTENSION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_SLAVE_MODE_SEND_EXTENSION_DATA(){
	/* clear eliminated paths */
	vector<uint64_t> newNames;
	vector<vector<Kmer> > newPaths;

	for(int i=0;i<(int)m_ed->m_EXTENSION_contigs.size();i++){
		uint64_t uniqueId=m_ed->m_EXTENSION_identifiers[i];
		if(m_fusionData->m_FUSION_eliminated.count(uniqueId)>0){
			continue;
		}
		newNames.push_back(uniqueId);
		newPaths.push_back(m_ed->m_EXTENSION_contigs[i]);
	}

	/* overwrite old paths */
	m_fusionData->m_FUSION_eliminated.clear();
	m_ed->m_EXTENSION_identifiers=newNames;
	m_ed->m_EXTENSION_contigs=newPaths;

	cout<<"Rank "<<m_rank<< " is appending its fusions"<<endl;
	string output=m_parameters.getOutputFile();
	ofstream fp;
	if(m_rank==0){
		fp.open(output.c_str());
	}else{
		fp.open(output.c_str(),ios_base::out|ios_base::app);
	}
	int total=0;

	m_scaffolder.setContigPaths(&(m_ed->m_EXTENSION_identifiers),&(m_ed->m_EXTENSION_contigs));
	m_searcher.setContigs(&(m_ed->m_EXTENSION_contigs),&(m_ed->m_EXTENSION_identifiers));

	for(int i=0;i<(int)m_ed->m_EXTENSION_contigs.size();i++){
		uint64_t uniqueId=m_ed->m_EXTENSION_identifiers[i];
		if(m_fusionData->m_FUSION_eliminated.count(uniqueId)>0){
			continue;
		}
		total++;
		string contig=convertToString(&(m_ed->m_EXTENSION_contigs[i]),m_parameters.getWordSize(),m_parameters.getColorSpaceMode());
		
		string withLineBreaks=addLineBreaks(contig,m_parameters.getColumns());
		fp<<">contig-"<<uniqueId<<" "<<contig.length()<<" nucleotides"<<endl<<withLineBreaks;

	}
	cout<<"Rank "<<m_rank<<" appended "<<total<<" elements"<<endl;
	fp.close();

	if(m_parameters.showMemoryUsage()){
		showMemoryUsage(getRank());
	}

	/** possibly write the checkpoint */
	if(m_parameters.writeCheckpoints() && !m_parameters.hasCheckpoint("ContigPaths")){
		cout<<"Rank "<<m_parameters.getRank()<<" is writing checkpoint ContigPaths"<<endl;
		ofstream f(m_parameters.getCheckpointFile("ContigPaths").c_str());
		int theSize=m_ed->m_EXTENSION_contigs.size();
		f.write((char*)&theSize,sizeof(int));

		/* write each path with its name and vertices */
		for(int i=0;i<theSize;i++){
			uint64_t name=m_ed->m_EXTENSION_identifiers[i];
			int vertices=m_ed->m_EXTENSION_contigs[i].size();
			f.write((char*)&name,sizeof(uint64_t));
			f.write((char*)&vertices,sizeof(int));
			for(int j=0;j<vertices;j++){
				m_ed->m_EXTENSION_contigs[i][j].write(&f);
			}
		}
		f.close();
	}

	m_switchMan.setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
	Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_EXTENSION_DATA_END,getRank());
	m_outbox.push_back(aMessage);
}

void Machine::call_RAY_SLAVE_MODE_FUSION(){
	m_fusionTaskCreator.call_RAY_SLAVE_MODE_FUSION();
}

void Machine::call_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION(){
	m_library.call_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION();
}

void Machine::call_RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES(){
	m_library.call_RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES();
}

void Machine::call_RAY_MASTER_MODE_UPDATE_DISTANCES(){
	m_library.call_RAY_MASTER_MODE_UPDATE_DISTANCES();
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_FUSIONS(){
	m_timePrinter.printElapsedTime("Bidirectional extension of seeds");
	cout<<endl;
	
	m_cycleNumber=0;

	m_switchMan.closeMasterMode();
}


void Machine::call_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS(){

	m_reductionOccured=true;
	m_cycleStarted=false;
	m_mustStop=false;
	
	m_switchMan.closeMasterMode();
}

void Machine::call_RAY_MASTER_MODE_START_FUSION_CYCLE(){
	/** this master method may require the whole outbox... */
	if(m_outbox.size()!=0)
		return;

	// the finishing is
	//
	//  * a clear cycle
	//  * a distribute cycle
	//  * a finish cycle
	//  * a clear cycle
	//  * a distribute cycle
	//  * a fusion cycle

	int lastAllowedCycleNumber=5;

	if(!m_cycleStarted){
		int count=0;
		if(m_mustStop){
			count=1;
		}

		uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		m_reductionOccured=false;
		m_cycleStarted=true;
		m_isFinalFusion=false;
		for(int i=0;i<getSize();i++){
			Message aMessage(buffer,count,i,RAY_MPI_TAG_CLEAR_DIRECTIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_currentCycleStep=1;
		m_CLEAR_n=0;

		cout<<"Rank 0: starting clear step. cycleNumber= "<<m_cycleNumber<<endl;

		/* change the regulators if this is the first cycle. */
		if(m_cycleNumber == 0){
			m_isFinalFusion = true;
			m_currentCycleStep = 4;
		}

	}else if(m_CLEAR_n==getSize() && !m_isFinalFusion && m_currentCycleStep==1){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_currentCycleStep++;
		m_CLEAR_n=-1;

		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_DISTRIBUTE_n=0;
	}else if(m_DISTRIBUTE_n==getSize() && !m_isFinalFusion && m_currentCycleStep==2){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_currentCycleStep++;
		m_DISTRIBUTE_n=-1;
		m_isFinalFusion=true;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_FINISH_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_FINISH_n=0;
	}else if(m_FINISH_n==getSize() && m_isFinalFusion && m_currentCycleStep==3){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_currentCycleStep++;
		int count=0;

		//cout<<"DEBUG m_reductionOccured= "<<m_reductionOccured<<endl;

		/* if paths were merged in RAY_MPI_TAG_FINISH_FUSIONS,
		then we want to continue these mergeing events */
		if(m_reductionOccured && m_cycleNumber < lastAllowedCycleNumber)
			m_mustStop = false;

		if(m_mustStop){
			count=1;
		}
		uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		for(int i=0;i<getSize();i++){
			Message aMessage(buffer,count,i,RAY_MPI_TAG_CLEAR_DIRECTIONS,getRank());
			m_outbox.push_back(aMessage);
		}

		m_FINISH_n=-1;
		m_CLEAR_n=0;
	}else if(m_CLEAR_n==getSize() && m_isFinalFusion && m_currentCycleStep==4){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_CLEAR_n=-1;
		m_currentCycleStep++;

		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_DISTRIBUTE_n=0;
	
		cout<<"Rank 0: starting distribution step"<<endl;
	}else if(m_DISTRIBUTE_n==getSize() && m_isFinalFusion && m_currentCycleStep==5){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_currentCycleStep++;

		/* if we have the checkpoint, we want to jump to the final step now */

		/* the other condition is that we have to stop */
		if(m_mustStop || m_parameters.hasCheckpoint("ContigPaths")){
			cout<<"Rank "<<m_parameters.getRank()<<" cycleNumber= "<<m_cycleNumber<<endl;
			m_timePrinter.printElapsedTime("Merging of redundant paths");
			cout<<endl;

			m_switchMan.setMasterMode(RAY_MASTER_MODE_ASK_EXTENSIONS);

			m_ed->m_EXTENSION_currentRankIsSet=false;
			m_ed->m_EXTENSION_rank=-1;
			return;
		}

		cout<<"Rank 0 tells others to compute fusions."<<endl;
		m_fusionData->m_FUSION_numberOfRanksDone=0;
		m_DISTRIBUTE_n=-1;
		for(int i=0;i<(int)getSize();i++){// start fusion.
			Message aMessage(NULL,0,i,RAY_MPI_TAG_START_FUSION,getRank());
			m_outbox.push_back(aMessage);
		}
		
	}else if(m_fusionData->m_FUSION_numberOfRanksDone==getSize() && m_isFinalFusion && m_currentCycleStep==6){

		/** always force cycle number 2 */
		if(m_cycleNumber == 0)
			m_reductionOccured = true;

		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_fusionData->m_FUSION_numberOfRanksDone=-1;

		//cout<<"DEBUG m_reductionOccured= "<<m_reductionOccured<<endl;

		if(!m_reductionOccured || m_cycleNumber == lastAllowedCycleNumber){ 
			m_mustStop=true;
		}

		// we continue now!
		m_cycleStarted=false;
		m_cycleNumber++;
	}
}

void Machine::call_RAY_MASTER_MODE_ASK_EXTENSIONS(){
	// ask ranks to send their extensions.
	if(!m_ed->m_EXTENSION_currentRankIsSet){
		m_ed->m_EXTENSION_currentRankIsSet=true;
		m_ed->m_EXTENSION_currentRankIsStarted=false;
		m_ed->m_EXTENSION_rank++;
	}
	if(m_ed->m_EXTENSION_rank==getSize()){
		m_timePrinter.printElapsedTime("Generation of contigs");
		if(m_parameters.useAmos()){
			m_switchMan.setMasterMode(RAY_MASTER_MODE_AMOS);

			m_ed->m_EXTENSION_currentRankIsStarted=false;
			m_ed->m_EXTENSION_currentPosition=0;
			m_ed->m_EXTENSION_rank=0;
			m_seedingData->m_SEEDING_i=0;
			m_mode_send_vertices_sequence_id_position=0;
			m_ed->m_EXTENSION_reads_requested=false;
			cout<<endl;
		}else{

			m_switchMan.closeMasterMode();

			m_scaffolder.m_numberOfRanksFinished=0;
		}
		
	}else if(!m_ed->m_EXTENSION_currentRankIsStarted){
		m_ed->m_EXTENSION_currentRankIsStarted=true;
		Message aMessage(NULL,0,m_ed->m_EXTENSION_rank,RAY_MPI_TAG_ASK_EXTENSION_DATA,getRank());
		m_outbox.push_back(aMessage);
		m_ed->m_EXTENSION_currentRankIsDone=false;
	}else if(m_ed->m_EXTENSION_currentRankIsDone){
		m_ed->m_EXTENSION_currentRankIsSet=false;
	}
}

void Machine::call_RAY_MASTER_MODE_AMOS(){
	m_amos.call_RAY_MASTER_MODE_AMOS();
}

void Machine::call_RAY_MASTER_MODE_SCAFFOLDER(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_START_SCAFFOLDER,getRank());
		m_outbox.push_back(aMessage);
	}
	m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void Machine::call_RAY_SLAVE_MODE_SCAFFOLDER(){
	m_scaffolder.call_RAY_SLAVE_MODE_SCAFFOLDER();
}

void Machine::call_RAY_SLAVE_MODE_AMOS(){
	m_amos.call_RAY_SLAVE_MODE_AMOS();
}

void Machine::call_RAY_SLAVE_MODE_EXTENSION(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	// TODO: initialise these things in the constructor

	m_seedExtender.call_RAY_SLAVE_MODE_EXTENSION(&(m_seedingData->m_SEEDING_seeds),m_ed,getRank(),&m_outbox,&(m_seedingData->m_SEEDING_currentVertex),
	m_fusionData,&m_outboxAllocator,&(m_seedingData->m_SEEDING_edgesRequested),&(m_seedingData->m_SEEDING_outgoingEdgeIndex),
	&m_last_value,&(m_seedingData->m_SEEDING_vertexCoverageRequested),m_wordSize,getSize(),&(m_seedingData->m_SEEDING_vertexCoverageReceived),
	&(m_seedingData->m_SEEDING_receivedVertexCoverage),&m_repeatedLength,&(m_seedingData->m_SEEDING_receivedOutgoingEdges),&m_c,
	m_bubbleData,
m_parameters.getMinimumCoverage(),&m_oa,&(m_seedingData->m_SEEDING_edgesReceived),m_switchMan.getSlaveModePointer());

	MACRO_COLLECT_PROFILING_INFORMATION();
}

/** process data my calling current slave and master methods */
void Machine::processData(){
	MachineMasterHandler masterMethod=m_master_methods[m_switchMan.getMasterMode()];
	(this->*masterMethod)();

	MachineSlaveHandler slaveMethod=m_slave_methods[m_switchMan.getSlaveMode()];
	(this->*slaveMethod)();
}

void Machine::call_RAY_MASTER_MODE_KILL_RANKS(){
	m_switchMan.closeMasterMode();
}

/** make the message-passing interface rank die */
void Machine::call_RAY_SLAVE_MODE_DIE(){

	/* write the network test data if not already written */
	m_networkTest.writeData();

	/** write message-passing interface file */
	ostringstream file;
	file<<m_parameters.getPrefix()<<"MessagePassingInterface.txt";

	string fileInString=file.str();
	m_messagesHandler.appendStatistics(fileInString.c_str());

	/** actually die */
	m_alive=false;

	/** tell master that the rank died 
 * 	obviously, this message won't be recorded in the MessagePassingInterface file...
 * 	Because of that, MessagesHandler will do it for us.
 * 	*/
	Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY,m_rank);
	m_outbox.push_back(aMessage);

	/** do nothing while dying 
 * 	the aging process takes a while -- 1024 cycles.
 * 	after that, it is death itself.
 * 	*/
	m_switchMan.setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
}

/**
 * test the network.
 * control logic
 */
void Machine::call_RAY_MASTER_MODE_TEST_NETWORK(){
	m_networkTest.call_RAY_MASTER_MODE_TEST_NETWORK();
}

void Machine::call_RAY_SLAVE_MODE_TEST_NETWORK(){
	m_networkTest.call_RAY_SLAVE_MODE_TEST_NETWORK();
}

/**
 * here we kill everyone because the computation is terminated.
 */
void Machine::call_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS(){
	if(!m_initialisedKiller){
		m_initialisedKiller=true;
		m_machineRank=m_parameters.getSize()-1;

		/** empty the file if it exists */
		ostringstream file;
		file<<m_parameters.getPrefix()<<"MessagePassingInterface.txt";
		
		FILE*fp=fopen(file.str().c_str(),"w+");
		if(fp==NULL){
			cout<<"Error: cannot create file "<<file<<endl;
		}
		fprintf(fp,"# Source\tDestination\tTag\tCount\n");
		fclose(fp);

		// activate the relay checker
		m_numberOfRanksDone=0;
		for(Rank i=0;i<m_parameters.getSize();i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER,getRank());
			m_outbox.push_back(aMessage);
		}

	// another rank activated its relay checker
	}else if(m_inbox.size()>0 && m_inbox[0]->getTag()==RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY){
		m_numberOfRanksDone++;

	// do nothing and wait
	}else if(m_numberOfRanksDone!=m_parameters.getSize()){

	/** for the first to process (getSize()-1) -- the last -- we directly send it
 * a message.
 * For the other ones, we wait for the response of the previous.
 */
	}else if(m_machineRank==m_parameters.getSize()-1 || 
	(m_inbox.size()>0 && m_inbox[0]->getTag()==RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY)){

		/**
 * 			Rank 0 is the last to kill
 */
		if(m_machineRank==0){
			m_switchMan.setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
		}

		/** send a killer message */
		Message aMessage(NULL,0,m_machineRank,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON,getRank());
		m_outbox.push_back(aMessage);

		/** change the next to kill */
		m_machineRank--;
	}
}

bool Machine::isMaster(){
	return getRank()==MASTER_RANK;
}

int Machine::getSize(){
	return m_size;
}

Machine::~Machine(){
	delete m_bubbleData;
	m_bubbleData=NULL;
}

void Machine::assignMasterHandlers(){
	vector<RayMasterMode> modes;
	vector<MachineMasterHandler> handlers;

	m_scriptEngine.configureMasterHandlers(&modes,&handlers);

	for(int i=0;i<(int)modes.size();i++){
		m_master_methods[modes[i]]=handlers[i];
	}
}

void Machine::assignSlaveHandlers(){

	vector<RaySlaveMode> modes;
	vector<MachineSlaveHandler> handlers;

	m_scriptEngine.configureSlaveHandlers(&modes,&handlers);

	for(int i=0;i<(int)modes.size();i++){
		m_slave_methods[modes[i]]=handlers[i];
	}
}

void Machine::call_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES(){
	m_searcher.call_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES();
}

void Machine::call_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES(){
	m_searcher.call_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES();
}

void Machine::call_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES(){
	m_searcher.call_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES();
}

void Machine::call_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES(){
	m_searcher.call_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES();
}


/* append empty implementations */
/* if one of them need to be implemented in an non-empty fashion, it must 
 * first be removed from scripting/empty_slave_handlers.txt 
 * or  scripting/empty_master_handlers.txt
 * and added here as a call_*
 */

#define ITEM(mode) \
void Machine::call_ ## mode (){}

#include <scripting/empty_slave_handlers.txt>
#include <scripting/empty_master_handlers.txt>

#undef ITEM


