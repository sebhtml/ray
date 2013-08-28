/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert
    Copyright (C) 2013 Charles Joly Beauparlant

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt)
	see <http://www.gnu.org/licenses/>
*/

// TODO -- there should not be any plugin construction in this file

#include "Machine.h"

#include <code/VerticesExtractor/GridTableIterator.h>
#include <code/VerticesExtractor/VerticesExtractor.h>
#include <code/SeedExtender/TipWatchdog.h>
#include <code/SeedExtender/BubbleTool.h>
#include <code/Mock/common_functions.h>
#include <code/Mock/constants.h>
#include <code/CoverageGatherer/CoverageDistribution.h>
#include <code/SequencesLoader/Read.h>
#include <code/SequencesLoader/Loader.h>

#include <RayPlatform/profiling/Profiler.h>
#include <RayPlatform/communication/Message.h>
#include <RayPlatform/structures/SplayTreeIterator.h>
#include <RayPlatform/structures/SplayNode.h>
#include <RayPlatform/core/statistics.h>
#include <RayPlatform/core/OperatingSystem.h>
#include <RayPlatform/memory/MyAllocator.h>
#include <RayPlatform/cryptography/crypto.h>
#include <RayPlatform/core/types.h>

#include <iostream>
#include <fstream>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <algorithm>
#include <sstream>
using namespace std;

/* Pick-up the option -show-communication-events */

/**
 * called before Machine::start()
 */
Machine::Machine(int argc,char**argv){

	m_argc=argc;
	m_argv=argv;

}

void Machine::init(int argc,char**argv){

	m_rank=m_computeCore.getRank();
	m_size=m_computeCore.getSize();
	
	m_alive=m_computeCore.getLife();

	m_inbox=m_computeCore.getInbox();
	m_outbox=m_computeCore.getOutbox();
	m_router=m_computeCore.getRouter();
	m_switchMan=m_computeCore.getSwitchMan();
	m_tickLogger=m_computeCore.getTickLogger();
	m_inboxAllocator=m_computeCore.getInboxAllocator();
	m_outboxAllocator=m_computeCore.getOutboxAllocator();
	m_virtualCommunicator=m_computeCore.getVirtualCommunicator();
	m_virtualProcessor=m_computeCore.getVirtualProcessor();

	if(argc==1){
		if(isMaster())
			m_parameters.showUsage();

		m_computeCore.destructor();

		m_aborted=true;

	}else if(argc==2){
		string param=argv[1];
		if(param.find("help")!=string::npos){
			if(isMaster())
				m_parameters.showUsage();

			m_computeCore.destructor();
			m_aborted=true;
		}else if(param.find("usage")!=string::npos){
			if(isMaster())
				m_parameters.showUsage();

			m_computeCore.destructor();
			m_aborted=true;
		}else if(param.find("man")!=string::npos){
			if(isMaster())
				m_parameters.showUsage();

			m_computeCore.destructor();
			m_aborted=true;
		}else if(param.find("-version")!=string::npos){
			if(isMaster())
				showRayVersionShort();

			m_computeCore.destructor();
			m_aborted=true;
		}
	}
	
	if(m_aborted)
		return;

	#if 0
	" ProcessorName= "<<*(m_messagesHandler->getName())<<endl;
	#endif

	m_argc=argc;
	m_argv=argv;
	m_bubbleData=new BubbleData();

	#ifdef SHOW_SENT_MESSAGES
	m_stats=new StatisticsData();
	#endif

	m_fusionData=new FusionData();
	m_seedingData=new SeedingData();

	m_ed=new ExtensionData();

	// construct titans
	m_helper.constructor(argc,argv,&m_parameters,m_switchMan,m_outboxAllocator,m_outbox,&m_aborted,
		&m_coverageDistribution,&m_numberOfMachinesDoneSendingCoverage,&m_numberOfRanksWithCoverageData,
	&m_reductionOccured,m_ed,m_fusionData,m_profiler,&m_networkTest,m_seedingData,
	&m_timePrinter,&m_seedExtender,&m_scaffolder,m_inbox,
	&m_oa,&m_isFinalFusion,m_bubbleData,m_alive,
	&m_CLEAR_n,&m_DISTRIBUTE_n,&m_FINISH_n,&m_searcher,
	&m_numberOfRanksDoneSeeding,&m_numberOfRanksDoneDetectingDistances,&m_numberOfRanksDoneSendingDistances,
	&m_myReads,&m_last_value,&m_verticesExtractor,&m_edgePurger,
&m_mode_send_vertices_sequence_id,&m_coverageGatherer,&m_subgraph,&m_si,
m_virtualCommunicator,&m_kmerAcademyBuilder,
&m_numberOfMachinesDoneSendingVertices,&m_initialisedAcademy,&m_repeatedLength,&m_readyToSeed,&m_ranksDoneAttachingReads,
&m_sl,&m_lastTime,&m_writeKmerInitialised,&m_partitioner

);

}

/**
 * start the software
 * this method does these things:
 *
 *  1. create plugins
 *  2. register plugins
 *  3. resolve symbols (not necessary)
 *  4. hit the push-button device to start the computation distributed cores (ComputeCore::run())
 */
void Machine::start(){

	init(m_argc,m_argv);


	if(m_aborted)
		return;

	// TODO: move this in plugin Searcher
	m_searcher.constructor(&m_parameters,m_outbox,&m_timePrinter,m_switchMan,m_virtualCommunicator,m_inbox,
		m_outboxAllocator,&m_subgraph);

	// set legacy states
	// some of them are not used anymore
	m_initialisedAcademy=false;
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
	m_last_value=0;
	m_mode_send_ingoing_edges=false;
	m_mode_send_vertices=false;
	m_mode_sendDistribution=false;
	m_mode_send_outgoing_edges=false;
	m_mode_send_vertices_sequence_id=0;
	m_numberOfMachinesDoneSendingVertices=0;
	m_numberOfMachinesDoneSendingEdges=0;
	m_numberOfMachinesReadyToSendDistribution=0;
	m_numberOfMachinesDoneSendingCoverage=0;
	m_messageSentForVerticesDistribution=false;
	m_sequence_ready_machines=0;
	m_isFinalFusion=false;


	if(isMaster()){
		cout<<endl<<"**************************************************"<<endl;
    		cout<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    		cout<<"This is free software, and you are welcome to redistribute it"<<endl;
		cout<<"under certain conditions; see \"LICENSE.txt\" for details."<<endl;
		cout<<"**************************************************"<<endl;
		cout<<endl;
		cout<<"Ray Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert"<<endl;
		cout<<"Centre de recherche en infectiologie de l'Université Laval"<<endl;
		cout<<"Project funded by the Canadian Institutes of Health Research (Doctoral award 200902CGM-204212-172830 to S.B.)"<<endl;
 		cout<<"http://denovoassembler.sf.net/"<<endl<<endl;

		cout<<"References to cite: "<<endl<<endl;

		cout << "Sébastien Boisvert, Frédéric Raymond, Élénie Godzaridis, François Laviolette and Jacques Corbeil." <<endl;
		cout << "Ray Meta: scalable de novo metagenome assembly and profiling."<<endl;
		cout<<"Genome Biology (BioMed Central Ltd)."<<endl;
		cout<<"13:R122, Published: 22 December 2012"<<endl;
		cout<<"http://dx.doi.org/doi:10.1186/gb-2012-13-12-r122"<<endl;
		cout << endl;

		cout<<"Sébastien Boisvert, François Laviolette & Jacques Corbeil."<<endl;
		cout<<"Ray: simultaneous assembly of reads from a mix of high-throughput sequencing technologies."<<endl;
		cout<<"Journal of Computational Biology (Mary Ann Liebert, Inc. publishers, New York, U.S.A.)."<<endl;
		cout<<"November 2010, Volume 17, Issue 11, Pages 1519-1533."<<endl;
		cout<<"http://dx.doi.org/doi:10.1089/cmb.2009.0238"<<endl;
		cout<<endl;
	}

	m_parameters.setSize(getSize());

	// this peak is attained in VerticesExtractor::deleteVertices
	// 2012-03-20: which peaks ?
	// 2012-11-06: probably memory usage (Virtual Memory)

	/**
 * 		build the plugins now */
	m_scaffolder.constructor(m_outbox,m_inbox,m_outboxAllocator,&m_parameters,
	m_virtualCommunicator,m_switchMan);
	m_scaffolder.setTimePrinter(&m_timePrinter);

	m_edgePurger.constructor(m_outbox,m_inbox,m_outboxAllocator,&m_parameters,m_switchMan->getSlaveModePointer(),m_switchMan->getMasterModePointer(),
	m_virtualCommunicator,&m_subgraph,m_virtualProcessor);

	m_coverageGatherer.constructor(&m_parameters,m_inbox,m_outbox,m_switchMan->getSlaveModePointer(),&m_subgraph,
		m_outboxAllocator);

	m_fusionTaskCreator.constructor(m_virtualProcessor,m_outbox,
		m_outboxAllocator,m_switchMan->getSlaveModePointer(),&m_parameters,&(m_ed->m_EXTENSION_contigs),
		&(m_ed->m_EXTENSION_identifiers),&(m_fusionData->m_FUSION_eliminated),
		m_virtualCommunicator);

	m_joinerTaskCreator.constructor(m_virtualProcessor,m_outbox,
		m_outboxAllocator,m_switchMan->getSlaveModePointer(),&m_parameters,&(m_ed->m_EXTENSION_contigs),
		&(m_ed->m_EXTENSION_identifiers),&(m_fusionData->m_FUSION_eliminated),
		m_virtualCommunicator,&(m_fusionData->m_FINISH_newFusions));

	m_amos.constructor(&m_parameters,m_outboxAllocator,m_outbox,m_fusionData,m_ed,m_switchMan->getMasterModePointer(),m_switchMan->getSlaveModePointer(),&m_scaffolder,
		m_inbox,m_virtualCommunicator);

	m_mp.setScaffolder(&m_scaffolder);
	m_mp.setVirtualCommunicator(m_virtualCommunicator);
	m_mp.setSwitchMan(m_switchMan);

	m_mode_AttachSequences=false;
	m_startEdgeDistribution=false;

	m_ranksDoneAttachingReads=0;

	// TODO: check if 65536 is really a limit.
	// the limit is probably in a header somewhere in application_core or in RayPlatform.
	// with routing enabled, the limit is 4096 compute cores.
	
	int maximumNumberOfProcesses=65536;
	if(getSize()>maximumNumberOfProcesses){
		cout<<"Error: The maximum number of processes is "<<maximumNumberOfProcesses<<" (this can be changed in the code)"<<endl;
	}

	assert(getSize()<=maximumNumberOfProcesses);

	bool fullReport=false;

	if(m_argc==2){
		string argument=m_argv[1];
		if(argument=="-version" || argument=="--version")
			fullReport=true;
	}

	if(isMaster()){
		showRayVersion(fullReport);

		if(!fullReport){
			cout<<endl;
		}
	}

	int miniRanksPerRank=m_computeCore.getMiniRanksPerRank();

	/** only show the version. */
	if(fullReport){
		m_computeCore.destructor();
		return;
	}

	m_parameters.constructor(m_argc,m_argv,getRank(),m_size,miniRanksPerRank);

	m_partitioner.constructor(m_outboxAllocator,m_inbox,m_outbox,&m_parameters,m_switchMan);

	if(m_parameters.runProfiler())
		m_computeCore.enableProfiler();

	if(m_parameters.hasOption("-with-profiler-details"))
		m_computeCore.enableProfilerVerbosity();

	if(m_parameters.showCommunicationEvents())
		m_computeCore.showCommunicationEvents();
	
	string*name=m_computeCore.getMessagesHandler()->getName();

	// initiate the network test.
	m_networkTest.constructor(m_rank,m_size,m_inbox,m_outbox,&m_parameters,m_outboxAllocator,name,
		&m_timePrinter);

	m_networkTest.setSwitchMan(m_switchMan);

	int PERSISTENT_ALLOCATOR_CHUNK_SIZE=4194304; // 4 MiB
	m_persistentAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE,"RAY_MALLOC_TYPE_PERSISTENT_DATA_ALLOCATOR",
		m_parameters.showMemoryAllocations());

	// options are loaded from here
	// plus the directory exists now
	if(m_parameters.hasOption("-route-messages")){

		ostringstream routingPrefix;
		routingPrefix<<m_parameters.getPrefix()<<"/Routing/";

		m_router->enable(m_inbox,m_outbox,m_outboxAllocator,m_parameters.getRank(),
			routingPrefix.str(),m_parameters.getSize(),
			m_parameters.getConnectionType(),m_parameters.getRoutingDegree());

		// update the connections
		vector<int> connections;
		m_router->getGraph()->getIncomingConnections(m_parameters.getRank(),&connections);
		
	}

	if(m_parameters.hasOption("-verify-message-integrity")) {
		m_computeCore.enableCheckSums();
	}

/*
 * We need to know if we must abort. All the RayPlatform
 * engine will start anyway on each core, so we need to stop
 * them if necessary.
 */
	bool oldDirectoryExists=false;

	string directory=m_parameters.getPrefix();
	if(fileExists(directory.c_str())){
		if(m_parameters.getRank() == MASTER_RANK){
			cout<<"Error, "<<directory<<" already exists, change the -o parameter to another value."<<endl;
			cout<<endl;

/*
 * Tell the first plugin that the job will not run.
 */
			m_helper.notifyThatOldDirectoryExists();

			oldDirectoryExists=true;
		}
	}
	
	// create the directory
	/** create the directory for the assembly */
	if(!oldDirectoryExists && m_parameters.getRank() == MASTER_RANK){
		createDirectory(directory.c_str());
		m_parameters.writeCommandFile();

		m_timePrinter.setFile(m_parameters.getPrefix());

/*
 * Write routing information only if the directory is sane.
 */
		if(m_router->isEnabled())
			m_router->writeFiles();

	}

	// register the plugins.
	registerPlugins();

	// write a report about plugins
	if(!oldDirectoryExists && m_parameters.getRank()==MASTER_RANK){

		bool mustWritePluginData = m_parameters.hasOption("-write-plugin-data");

		if(mustWritePluginData) {
			ostringstream directory;
			directory<<m_parameters.getPrefix()<<"/Plugins";

			string file=directory.str();
			createDirectory(file.c_str());
	
			m_computeCore.printPlugins(file);
		}

		// write the version of RayPlatform
		
		ostringstream versionFile;
		versionFile<<m_parameters.getPrefix()<<"/RayPlatform_Version.txt";

		ofstream f7(versionFile.str().c_str());
		f7<<"RayPlatform "<<m_computeCore.getRayPlatformVersion()<<endl;
		f7.close();
	}

	// set the attributes of the seed extender.
	m_seedExtender.constructor(&m_parameters,m_ed,&m_subgraph,m_inbox,m_profiler,
		m_outbox,m_seedingData,m_switchMan->getSlaveModePointer(),&(m_seedingData->m_SEEDING_vertexCoverageRequested),
		&(m_seedingData->m_SEEDING_vertexCoverageReceived),m_outboxAllocator,m_fusionData,
		&(m_seedingData->m_SEEDING_seeds),m_bubbleData,&(m_seedingData->m_SEEDING_edgesRequested),
		&(m_seedingData->m_SEEDING_edgesReceived),&(m_seedingData->m_SEEDING_outgoingEdgeIndex),
		&(m_seedingData->m_SEEDING_currentVertex),&(m_seedingData->m_SEEDING_receivedVertexCoverage),
		&(m_seedingData->m_SEEDING_receivedOutgoingEdges),&m_c,&m_oa);

	m_kmerAcademyBuilder.constructor(m_parameters.getSize(),&m_parameters,&m_subgraph,
		&m_myReads,m_inbox,m_outbox,m_switchMan->getSlaveModePointer(),m_outboxAllocator);

	m_si.constructor(&m_parameters,m_outboxAllocator,m_inbox,m_outbox,m_virtualCommunicator,
		m_switchMan->getSlaveModePointer(),&m_myReads);

	m_profiler = m_computeCore.getProfiler();
	m_profiler->constructor(m_parameters.runProfiler());

	m_verticesExtractor.setProfiler(m_profiler);
	m_kmerAcademyBuilder.setProfiler(m_profiler);
	m_edgePurger.setProfiler(m_profiler);

	m_verticesExtractor.constructor(m_parameters.getSize(),&m_parameters,&m_subgraph,
		m_outbox,m_outboxAllocator,&m_myReads);

	ostringstream prefixFull;
	prefixFull<<m_parameters.getMemoryPrefix()<<"_Main";
	int chunkSize=16777216;
	m_diskAllocator.constructor(chunkSize,"RAY_MALLOC_TYPE_DATA_ALLOCATOR",
		m_parameters.showMemoryAllocations());

	m_sl.constructor(m_size,&m_diskAllocator,&m_myReads,&m_parameters,m_outbox,
		m_switchMan->getSlaveModePointer());

	m_fusionData->constructor(getSize(),MAXIMUM_MESSAGE_SIZE_IN_BYTES,getRank(),m_outbox,m_outboxAllocator,m_parameters.getWordSize(),
		m_ed,m_seedingData,m_switchMan->getSlaveModePointer(),&m_parameters);

/*
 m_seedingData,m_ed,m_parameters->getRank(),
		m_outboxAllocator,m_outbox,getSize(),m_switchMan->getSlaveModePointer()
*/

	m_library.constructor(getRank(),m_outbox,m_outboxAllocator,
		m_ed,getSize(),&m_timePrinter,m_switchMan->getSlaveModePointer(),m_switchMan->getMasterModePointer(),
	&m_parameters,m_seedingData,m_inbox,m_virtualCommunicator);

	m_subgraph.constructor(getRank(),&m_parameters);

	if(!oldDirectoryExists)
		m_subgraph.printStatus();
	
	m_seedingData->constructor(&m_seedExtender,getRank(),getSize(),m_outbox,m_outboxAllocator,m_switchMan->getSlaveModePointer(),&m_parameters,&m_wordSize,&m_subgraph,m_inbox,m_virtualCommunicator);

	(*m_alive)=true;
	m_totalLetters=0;

/*
 * Build the monstruous object.
 */
	m_mp.constructor(
m_router,
m_seedingData,
&m_library,&m_ready,
&m_verticesExtractor,
&m_sl,
			m_ed,
			&m_numberOfRanksDoneDetectingDistances,
			&m_numberOfRanksDoneSendingDistances,
			&m_parameters,
			&m_subgraph,
			m_outboxAllocator,
			getRank(),
			&m_numberOfMachinesDoneSendingEdges,
			m_fusionData,
			&m_wordSize,
			&m_myReads,
		getSize(),
	m_inboxAllocator,
	&m_persistentAllocator,
	&m_identifiers,
	&m_mode_sendDistribution,
	m_alive,
	m_switchMan->getSlaveModePointer(),
	&m_allPaths,
	&m_last_value,
	&m_ranksDoneAttachingReads,
	&m_DISTRIBUTE_n,
	&m_numberOfRanksDoneSeeding,
	&m_CLEAR_n,
	&m_readyToSeed,
	&m_FINISH_n,
	&m_reductionOccured,
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
				m_outbox,m_inbox,
	&m_oa,
	&m_numberOfRanksWithCoverageData,&m_seedExtender,
	m_switchMan->getMasterModePointer(),&m_isFinalFusion,&m_si);

	bool mustWriteSchedulingInformation = m_parameters.hasOption("-write-scheduling-data");

/*
 * Create the Scheduling before calling run().
 * run() is like a barrier.
 */
	// log ticks
	if(!oldDirectoryExists && m_parameters.getRank()==MASTER_RANK
			&& mustWriteSchedulingInformation){

		ostringstream scheduling;
		scheduling<<m_parameters.getPrefix()<<"/Scheduling/";
		createDirectory(scheduling.str().c_str());
	}

	cout<<"Rank "<<m_rank<<": Rank= "<<m_rank<<" Size= "<<m_size<<" ProcessIdentifier= "<<portableProcessId()<<endl;

	if(m_argc==1||((string)m_argv[1])=="--help"){
		if(isMaster()){
			m_aborted=true;
			m_parameters.showUsage();
		}
	}else{
		m_lastTime=time(NULL);
		m_computeCore.run();
	}

	/*if(isMaster() && !m_aborted){
		m_timePrinter.printDurations();
		cout<<endl;
	}*/

/*
 * TODO: the code above should go in a plugin, not here.
 */
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

	if(!oldDirectoryExists && !m_aborted
			&& mustWriteSchedulingInformation){

		ostringstream masterTicks;
		masterTicks<<m_parameters.getPrefix()<<"/Scheduling/"<<m_parameters.getRank()<<".MasterTicks.txt";
		ofstream f1(masterTicks.str().c_str());
		m_tickLogger->printMasterTicks(&f1);
		f1.close();

		ostringstream slaveTicks;
		slaveTicks<<m_parameters.getPrefix()<<"/Scheduling/"<<m_parameters.getRank()<<".SlaveTicks.txt";
		ofstream f2(slaveTicks.str().c_str());
		m_tickLogger->printSlaveTicks(&f2);
		f2.close();
	}

	m_persistentAllocator.clear();

	m_diskAllocator.clear();

/*
 * The app is responsible for destroying the core.
 * It should work also without this call because it
 * is RayPlatform that destroys the MessagesHandler.
 */

// TODO: it would be nice to have a callback to deregister plugins

	m_computeCore.destructor();
}

int Machine::getRank(){
	return m_rank;
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


void Machine::showRayVersionShort(){
	cout<<"Ray version "<<CONFIG_RAY_VERSION<<endl;
	cout<<"License for Ray: GNU General Public License version 3"<<endl;

	cout<<"RayPlatform version: "<<m_computeCore.getRayPlatformVersion()<<endl;
	cout<<"License for RayPlatform: GNU Lesser General Public License version 3"<<endl;
	cout<<endl;

	CoverageDepth maximumCoverageDepth=0;
	maximumCoverageDepth--; // underflow it

	cout<<"MAXKMERLENGTH: "<<CONFIG_MAXKMERLENGTH<<endl;
	cout<<"Maximum coverage depth stored by CoverageDepth: "<<maximumCoverageDepth<<endl;
	cout<<"MAXIMUM_MESSAGE_SIZE_IN_BYTES: "<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<" bytes"<<endl;

/*
content
	cout<<"option";
	#ifdef option
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

list
 FORCE_PACKING
 ASSERT
 CONFIG_HAVE_LIBZ
 HAVE_LIBBZ2
 CONFIG_PROFILER_COLLECT
 CONFIG_CLOCK_GETTIME
 __linux__
 _MSC_VER
 __GNUC__
 RAY_32_BITS
 RAY_64_BITS

for i in $(cat list ); do exp="s/option/$i/g"; sed $exp content; done > list2
*/

	/* generated code */

	cout<<"FORCE_PACKING = ";
	#ifdef CONFIG_FORCE_PACKING
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"ASSERT = ";
	#ifdef ASSERT
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"HAVE_LIBZ = ";
	#ifdef CONFIG_HAVE_LIBZ
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"HAVE_LIBBZ2 = ";
	#ifdef CONFIG_HAVE_LIBBZ2
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"PROFILER_COLLECT = ";
	#ifdef CONFIG_PROFILER_COLLECT
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"MINIRANKS = ";
#ifdef CONFIG_MINI_RANKS
	cout<<"y";
#else
	cout<<"n";
#endif
	cout<<endl;

	cout<<"CLOCK_GETTIME = ";
	#ifdef CONFIG_CLOCK_GETTIME
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"__linux__ = ";
	#ifdef __linux__
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"_MSC_VER = ";
	#ifdef _MSC_VER
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"__GNUC__ = ";
	#ifdef __GNUC__
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"RAY_32_BITS = ";
	#ifdef RAY_32_BITS
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"RAY_64_BITS = ";
	#ifdef RAY_64_BITS
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	#if defined(MPI_VERSION) && defined(MPI_SUBVERSION)
	cout<<"MPI standard version: MPI "<<MPI_VERSION<<"."<<MPI_SUBVERSION<<endl;
	#endif

	#if defined(MPICH2) && defined(MPICH2_VERSION)
        cout<<"MPI library: MPICH2 "<<MPICH2_VERSION;
	cout<<endl;
	#endif

	#if defined(OMPI_MPI_H) && defined(OMPI_MAJOR_VERSION) && defined(OMPI_MINOR_VERSION) && defined(OMPI_RELEASE_VERSION)
        cout<<"MPI library: Open-MPI "<<OMPI_MAJOR_VERSION;
	cout<<"."<<OMPI_MINOR_VERSION<<"."<<OMPI_RELEASE_VERSION;
	cout<<endl;
	#endif

	#if defined(__VERSION__) && defined(__GNUC_PATCHLEVEL__)
	cout<<"Compiler: GNU gcc/g++ "<<__VERSION__<<endl;
	#endif

/**
 * Report availability of MPI I/O
 */
	cout<<"MPI I/O: ";

#ifdef CONFIG_MPI_IO
	cout<<"y";
#else
	cout<<"n";
#endif
	cout<<endl;
	
	#ifdef __SSE4_2__
	cout<<"With SSE 4.2"<<endl;
	#endif

	#ifdef __POPCNT__
	cout<<"With hardware pop count"<<endl;
	#endif
}

void Machine::showRayVersion(bool fullReport){
	showRayVersionShort();

}

void Machine::registerPlugins(){

	m_computeCore.registerPlugin(&m_helper);
	m_computeCore.registerPlugin(&m_mp);
	m_computeCore.registerPlugin(&m_networkTest);
	m_computeCore.registerPlugin(&m_partitioner);
	m_computeCore.registerPlugin(&m_sl);
	m_computeCore.registerPlugin(&m_kmerAcademyBuilder);
	m_computeCore.registerPlugin(&m_coverageGatherer);
	m_computeCore.registerPlugin(&m_verticesExtractor);
	m_computeCore.registerPlugin(&m_edgePurger);
	m_computeCore.registerPlugin(&m_si);
	m_computeCore.registerPlugin(m_seedingData);
	m_computeCore.registerPlugin(&m_library);
	m_computeCore.registerPlugin(&m_seedExtender);
	m_computeCore.registerPlugin(m_fusionData);
	m_computeCore.registerPlugin(&m_fusionTaskCreator);
	m_computeCore.registerPlugin(&m_joinerTaskCreator);
	m_computeCore.registerPlugin(&m_amos);
	m_computeCore.registerPlugin(&m_scaffolder);
	m_computeCore.registerPlugin(&m_searcher);
	m_computeCore.registerPlugin(&m_phylogeny);
	m_computeCore.registerPlugin(&m_genomeNeighbourhood);
	m_computeCore.registerPlugin(&m_ontologyPlugin);
	m_computeCore.registerPlugin(&m_mock);
	m_computeCore.registerPlugin(&m_example);
	m_computeCore.registerPlugin(&m_pathEvaluator);
	m_computeCore.registerPlugin(&m_spuriousSeedAnnihilator);

	// resolve the symbols
	// this is done here because we want to write a summary for
	// each plugin
	// otherwise, symbols are resolved when ComputeCore::run() is called.
	m_computeCore.resolveSymbols();
}

void Machine::run(){

	start();
}




