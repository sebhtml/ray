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

#include <memory/malloc_types.h>
#include <graph/GridTableIterator.h>
#include <cryptography/crypto.h>
#include <structures/SplayNode.h>
#include <core/Machine.h>
#include <assembler/VerticesExtractor.h>
#include <sstream>
#include <communication/Message.h>
#include <time.h>
#include <assembler/TipWatchdog.h>
#include <assembler/BubbleTool.h>
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
#include <mpi.h>

using namespace std;

Machine::Machine(int argc,char**argv){
	m_messagesHandler.constructor(&argc,&argv);
	m_rank=m_messagesHandler.getRank();
	m_size=m_messagesHandler.getSize();
	if(isMaster() && argc==1){
		m_parameters.showUsage();
		exit(EXIT_NEEDS_ARGUMENTS);
	}else if(argc==2){
		string param=argv[1];
		if(param.find("help")!=string::npos){
			m_parameters.showUsage();
			exit(EXIT_NEEDS_ARGUMENTS);
		}else if(param.find("usage")!=string::npos){
			m_parameters.showUsage();
			exit(EXIT_NEEDS_ARGUMENTS);
		}else if(param.find("man")!=string::npos){
			m_parameters.showUsage();
			exit(EXIT_NEEDS_ARGUMENTS);
		}
	}

	cout<<"Rank "<<m_rank<<": Rank= "<<m_rank<<" Size= "<<m_size<<" ProcessIdentifier= "<<portableProcessId()<<" ProcessorName= "<<m_messagesHandler.getName()<<endl;

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
	m_coverageInitialised=false;
	m_timePrinter.constructor();

	m_killed=false;
	m_ready=true;
	
	m_fusionData->m_fusionStarted=false;
	m_ed->m_EXTENSION_numberOfRanksDone=0;
	m_messageSentForEdgesDistribution=false;
	m_numberOfRanksDoneSeeding=0;
	m_numberOfMachinesReadyForEdgesDistribution=0;
	m_ed->m_mode_EXTENSION=false;
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
	m_machineRank=0;
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
	m_parameters.setSlaveModePointer(&m_slave_mode);
	m_parameters.setMasterModePointer(&m_master_mode);
	MAX_ALLOCATED_MESSAGES_IN_OUTBOX=getSize();
	MAX_ALLOCATED_MESSAGES_IN_INBOX=1;

	// this peak is attained in VerticesExtractor::deleteVertices
	m_maximumAllocatedOutputBuffers=17; 

	if(MAX_ALLOCATED_MESSAGES_IN_OUTBOX<m_maximumAllocatedOutputBuffers){
		MAX_ALLOCATED_MESSAGES_IN_OUTBOX=m_maximumAllocatedOutputBuffers;
	}

	m_inboxAllocator.constructor(MAX_ALLOCATED_MESSAGES_IN_INBOX,MAXIMUM_MESSAGE_SIZE_IN_BYTES,
		RAY_MALLOC_TYPE_INBOX_ALLOCATOR,m_parameters.showMemoryAllocations());
	m_outboxAllocator.constructor(m_maximumAllocatedOutputBuffers,MAXIMUM_MESSAGE_SIZE_IN_BYTES,
		RAY_MALLOC_TYPE_OUTBOX_ALLOCATOR,m_parameters.showMemoryAllocations());

	m_inbox.constructor(MAX_ALLOCATED_MESSAGES_IN_INBOX,RAY_MALLOC_TYPE_INBOX_VECTOR,m_parameters.showMemoryAllocations());
	m_outbox.constructor(MAX_ALLOCATED_MESSAGES_IN_OUTBOX,RAY_MALLOC_TYPE_OUTBOX_VECTOR,m_parameters.showMemoryAllocations());

	m_scaffolder.constructor(&m_outbox,&m_inbox,&m_outboxAllocator,&m_parameters,&m_slave_mode,
	&m_virtualCommunicator);
	m_coverageGatherer.constructor(&m_parameters,&m_inbox,&m_outbox,&m_slave_mode,&m_subgraph,
		&m_outboxAllocator);

	m_amos.constructor(&m_parameters,&m_outboxAllocator,&m_outbox,m_fusionData,m_ed,&m_master_mode,&m_slave_mode,&m_scaffolder,
		&m_inbox,&m_virtualCommunicator);

	m_mp.setScaffolder(&m_scaffolder);
	m_mp.setVirtualCommunicator(&m_virtualCommunicator);

	int PERSISTENT_ALLOCATOR_CHUNK_SIZE=4194304; // 4 MiB
	m_persistentAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE,RAY_MALLOC_TYPE_PERSISTENT_DATA_ALLOCATOR,
		m_parameters.showMemoryAllocations());

	int directionAllocatorChunkSize=4194304; // 4 MiB
	m_directionsAllocator.constructor(directionAllocatorChunkSize,RAY_MALLOC_TYPE_WAVE_ALLOCATOR,
		m_parameters.showMemoryAllocations());

	m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
	m_mode_AttachSequences=false;
	m_startEdgeDistribution=false;

	m_ranksDoneAttachingReads=0;
	m_reducer.constructor(getSize(),&m_parameters);

	m_messagesHandler.barrier();

	m_si.constructor(&m_parameters,&m_outboxAllocator,&m_inbox,&m_outbox,&m_diskAllocator,&m_virtualCommunicator);

	int maximumNumberOfProcesses=65536;
	if(getSize()>maximumNumberOfProcesses){
		cout<<"The maximum number of processes is "<<maximumNumberOfProcesses<<" (this can be changed in the code)"<<endl;
	}
	assert(getSize()<=maximumNumberOfProcesses);

	int version;
	int subversion;
	m_messagesHandler.version(&version,&subversion);

	if(isMaster()){

		cout<<endl;
		cout<<"Rank "<<MASTER_RANK<<": Ray version: "<<RAY_VERSION<<endl;
		cout<<"Rank "<<MASTER_RANK<<" MAXKMERLENGTH: "<<MAXKMERLENGTH<<" (KMER_U64_ARRAY_SIZE: "<<KMER_U64_ARRAY_SIZE<<")"<<endl;
		cout<<"Rank "<<MASTER_RANK<<": GNU system (__GNUC__): ";
		#ifdef __GNUC__
		cout<<"yes"<<endl;
		#else
		cout<<"no"<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": Operating System: ";
		#ifdef OS_WIN
		cout<<"Microsoft Windows (OS_WIN)"<<endl;
		#else
		cout<<"POSIX (OS_POSIX)"<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": real-time Operating System (HAVE_CLOCK_GETTIME): ";
		#ifdef HAVE_CLOCK_GETTIME
		cout<<"yes"<<endl;
		#else
		cout<<"no"<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": Message-Passing Interface implementation: ";
		#ifdef MPICH2
                cout<<"MPICH2 (MPICH2)"<<MPICH2_VERSION<<endl;
		#endif
		#ifdef OMPI_MPI_H
                cout<<"Open-MPI (OMPI_MPI_H) "<<OMPI_MAJOR_VERSION<<"."<<OMPI_MINOR_VERSION<<"."<<OMPI_RELEASE_VERSION<<endl;
		#endif
		#ifndef MPICH2
		#ifndef OMPI_MPI_H
		cout<<"Unknown"<<endl;
		#endif
		#endif

		cout<<"Rank "<<MASTER_RANK<<": Message-Passing Interface standard version: "<<version<<"."<<subversion<<""<<endl;

		// show libraries
		cout<<"Rank "<<MASTER_RANK<<": GZIP (HAVE_LIBZ): ";
		#ifdef HAVE_LIBZ
		cout<<"yes"<<endl;
		#else
		cout<<"no"<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": BZIP2 (HAVE_LIBBZ2): ";
		#ifdef HAVE_LIBBZ2
		cout<<"yes"<<endl;
		#else
		cout<<"no"<<endl;
		#endif

		// show OS, only Linux

		cout<<"Rank "<<MASTER_RANK<<": GNU/Linux (__linux__): ";
		#ifdef __linux__
		cout<<"yes"<<endl;
		#else
		cout<<"no"<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": assertions (ASSERT): ";
		#ifdef ASSERT
		cout<<"yes"<<endl;
		#else
		cout<<"no"<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": maximum size of a message (MAXIMUM_MESSAGE_SIZE_IN_BYTES): "<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<" bytes"<<endl;
		cout<<"Rank "<<MASTER_RANK<<": don't align addresses on 8 bytes (FORCE_PACKING): ";
		#ifdef FORCE_PACKING
		cout<<"yes";
		#else
		cout<<"no";
		#endif
		cout<<endl;
		cout<<endl;
	
		#ifdef SHOW_SIZEOF

		cout<<"KMER_BYTES "<<KMER_BYTES<<endl;
		cout<<"KMER_UINT64_T "<<KMER_UINT64_T<<endl;
		cout<<"KMER_UINT64_T_MODULO "<<KMER_UINT64_T_MODULO<<endl;

		cout<<" sizeof(Vertex)="<<sizeof(Vertex)<<endl;
		cout<<" sizeof(VertexData)="<<sizeof(VertexData)<<endl;
		cout<<" sizeof(Direction)="<<sizeof(Direction)<<endl;
		cout<<" sizeof(ReadAnnotation)="<<sizeof(ReadAnnotation)<<endl;
		cout<<" sizeof(Read)="<<sizeof(Read)<<endl;
		cout<<" sizeof(PairedRead)="<<sizeof(PairedRead)<<endl;
		#endif

		cout<<endl;

		cout<<"Number of MPI ranks: "<<getSize()<<endl;
		cout<<"Ray master MPI rank: "<<MASTER_RANK<<endl;
		cout<<"Ray slave MPI ranks: 0-"<<getSize()-1<<endl;
		cout<<endl;

		#ifdef SHOW_ITEMS
		int count=0;
		#define MACRO_LIST_ITEM(x) count++;
		#include <master_mode_macros.h>
		#undef MACRO_LIST_ITEM
		cout<<"Ray master modes ( "<<count<<" )"<<endl;
		#define MACRO_LIST_ITEM(x) printf(" %i %s\n",x,#x);fflush(stdout);
		#include <master_mode_macros.h>
		#undef MACRO_LIST_ITEM
		cout<<endl;
		count=0;
		#define MACRO_LIST_ITEM(x) count++;
		#include <slave_mode_macros.h>
		#undef MACRO_LIST_ITEM
		cout<<"Ray slave modes ( "<<count<<" )"<<endl;
		#define MACRO_LIST_ITEM(x) printf(" %i %s\n",x,#x);fflush(stdout);
		#include <slave_mode_macros.h>
		#undef MACRO_LIST_ITEM
		cout<<endl;
		count=0;
		#define MACRO_LIST_ITEM(x) count++;
		#include <mpi_tag_macros.h>
		#undef MACRO_LIST_ITEM
		cout<<"Ray MPI tags ( "<<count<<" )"<<endl;
		#define MACRO_LIST_ITEM(x) printf(" %i %s\n",x,#x);fflush(stdout);
		#include <mpi_tag_macros.h>
		#undef MACRO_LIST_ITEM
		#endif
		cout<<endl;

		m_timePrinter.printElapsedTime("Beginning of computation");
		cout<<endl;
	}

	m_parameters.constructor(m_argc,m_argv,getRank());

	m_seedExtender.constructor(&m_parameters,&m_directionsAllocator,m_ed,&m_subgraph,&m_inbox);
	ostringstream prefixFull;
	prefixFull<<m_parameters.getMemoryPrefix()<<"_Main";
	int chunkSize=16777216;
	m_diskAllocator.constructor(chunkSize,RAY_MALLOC_TYPE_DATA_ALLOCATOR,
		m_parameters.showMemoryAllocations());

	m_sl.constructor(m_size,&m_diskAllocator,&m_myReads);

	m_fusionData->constructor(getSize(),MAXIMUM_MESSAGE_SIZE_IN_BYTES,getRank(),&m_outbox,&m_outboxAllocator,m_parameters.getWordSize(),
		m_ed,m_seedingData,&m_slave_mode,&m_parameters);

	m_virtualCommunicator.constructor(m_rank,m_size,&m_outboxAllocator,&m_inbox,&m_outbox);

	#define Set(x,y) m_virtualCommunicator.setReplyType( x, x ## _REPLY );
	#include <communication/VirtualCommunicatorConfiguration.h>
	#undef Set

	#define Set(x,y) m_virtualCommunicator.setElementsPerQuery( x, y );
	#include <communication/VirtualCommunicatorConfiguration.h>
	#undef Set

	m_library.constructor(getRank(),&m_outbox,&m_outboxAllocator,&m_sequence_id,&m_sequence_idInFile,
		m_ed,getSize(),&m_timePrinter,&m_slave_mode,&m_master_mode,
	&m_parameters,&m_fileId,m_seedingData,&m_inbox,&m_virtualCommunicator);

	m_subgraph.constructor(getRank(),&m_diskAllocator,&m_parameters);
	
	m_seedingData->constructor(&m_seedExtender,getRank(),getSize(),&m_outbox,&m_outboxAllocator,&m_seedCoverage,&m_slave_mode,&m_parameters,&m_wordSize,&m_subgraph,&m_inbox,&m_virtualCommunicator);

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
	m_mp.setReducer(&m_reducer);

	m_mp.constructor(
&m_messagesHandler,
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
			&m_minimumCoverage,
			&m_seedCoverage,
			&m_peakCoverage,
			&m_myReads,
		getSize(),
	&m_inboxAllocator,
	&m_persistentAllocator,
	&m_identifiers,
	&m_mode_sendDistribution,
	&m_alive,
	&m_slave_mode,
	&m_allPaths,
	&m_last_value,
	&m_ranksDoneAttachingReads,
	&m_DISTRIBUTE_n,
	&m_numberOfRanksDoneSeeding,
	&m_CLEAR_n,
	&m_readyToSeed,
	&m_FINISH_n,
	&m_nextReductionOccured,
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
	&m_master_mode,&m_isFinalFusion,&m_si);

	m_timePrinter.constructor();

	if(m_argc==1||((string)m_argv[1])=="--help"){
		if(isMaster()){
			m_aborted=true;
			m_parameters.showUsage();
		}
	}else{
		if(isMaster()){
			m_master_mode=RAY_MASTER_MODE_LOAD_CONFIG;
			m_sl.constructor(getSize(),&m_diskAllocator,
				&m_myReads);
		}

		m_lastTime=time(NULL);
		run();
	}

	if(isMaster() && !m_aborted){
		m_timePrinter.printDurations();

		cout<<endl;
		#ifdef COUNT_MESSAGES
		string file=m_parameters.getReceivedMessagesFile();
		const char*tmp=file.c_str();
		m_messagesHandler.writeStats(tmp);
		#endif

	}

	m_messagesHandler.barrier();

	if(m_parameters.showMemoryUsage()){
		showMemoryUsage(getRank());
	}

	m_messagesHandler.barrier();

	if(isMaster() && !m_aborted){
		m_scaffolder.printFinalMessage();
		cout<<endl;
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getOutputFile()<<endl;
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getScaffoldFile()<<endl;
		cout<<"Check for "<<m_parameters.getPrefix()<<".*"<<endl;
		cout<<endl;
		if(m_parameters.useAmos()){
			cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getAmosFile()<<" (reads mapped onto contiguous sequences in AMOS format)"<<endl;

		}
		#ifdef COUNT_MESSAGES
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getReceivedMessagesFile()<<" (MPI communication matrix; rows=destinations, columns=sources) "<<endl;
		#endif
		cout<<endl;
	}

	m_messagesHandler.barrier();
	m_messagesHandler.freeLeftovers();
	m_persistentAllocator.clear();
	m_directionsAllocator.clear();
	m_inboxAllocator.clear();
	m_outboxAllocator.clear();

	m_diskAllocator.clear();

	MPI_Finalize();
}

void Machine::call_RAY_MASTER_MODE_WRITE_SCAFFOLDS(){
	m_scaffolder.writeScaffolds();
}

void Machine::run(){
	if(m_parameters.runProfiler()){
		runWithProfiler();
	}else{
		runVanilla();
	}
}

void Machine::runVanilla(){
	while(m_alive){
		// 1. receive the message (0 or 1 message is received)
		receiveMessages(); 
		// 2. process the received message, if any
		processMessages();
		// 3. process data according to current slave and master modes
		processData();
		// 4. send messages
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
	int receivedMessages=0;
	map<int,int> messageTypes;
	
	int resolution=100;// milliseconds
	int parts=1000/resolution;

	int lastTime=getMilliSeconds();

	while(m_alive){
		int t=getMilliSeconds();
		if(t>=(lastTime+resolution)/parts*parts){
			int toPrint=t;
			double seconds=toPrint/(1000.0);
			int balance=sentMessages-receivedMessages;
			printf("Rank %i: %s Time= %.2f s Speed= %i Sent= %i Received= %i Balance= %i\n",m_rank,SLAVE_MODES[m_slave_mode],
				seconds,ticks,sentMessages,receivedMessages,balance);
			fflush(stdout);
			ticks=0;
			sentMessages=0;
			receivedMessages=0;
			messageTypes.clear();
			lastTime=t;
		}
		// 1. receive the message (0 or 1 message is received)
		receiveMessages(); 
		receivedMessages+=m_inbox.size();
		
		// 2. process the received message, if any
		processMessages();

		// 3. process data according to current slave and master modes
		processData();

		sentMessages+=m_outbox.size();
		ticks++;
		for(int i=0;i<(int)m_outbox.size();i++){
			messageTypes[m_outbox[i]->getTag()]++;
		}

		// 4. send messages
		sendMessages();
	}
}

int Machine::getRank(){
	return m_rank;
}

void Machine::processMessages(){
	#ifdef ASSERT
	assert(m_inbox.size()>=0&&m_inbox.size()<=1);
	#endif

	if(m_inbox.size()>0){
		m_mp.processMessage((m_inbox[0]));
	}
}

void Machine::sendMessages(){
	#ifdef ASSERT
	assert(m_outboxAllocator.getCount()<=m_maximumAllocatedOutputBuffers);
	m_outboxAllocator.resetCount();
	int messagesToSend=m_outbox.size();
	if(messagesToSend>MAX_ALLOCATED_MESSAGES_IN_OUTBOX){
		cout<<"Fatal: "<<messagesToSend<<" messages to send, but max is "<<MAX_ALLOCATED_MESSAGES_IN_OUTBOX<<endl;
		cout<<"tags=";
		for(int i=0;i<(int)m_outbox.size();i++){
			int tag=m_outbox[i]->getTag();
			cout<<" "<<MESSAGES[tag]<<endl;
		}
		cout<<endl;
	}

	assert(messagesToSend<=MAX_ALLOCATED_MESSAGES_IN_OUTBOX);
	if(messagesToSend>MAX_ALLOCATED_MESSAGES_IN_OUTBOX){
		cout<<"Tag="<<m_outbox[0]->getTag()<<" n="<<messagesToSend<<" max="<<MAX_ALLOCATED_MESSAGES_IN_OUTBOX<<endl;
	}
	#endif

	m_messagesHandler.sendMessages(&m_outbox,getRank());
}

void Machine::receiveMessages(){
	m_inbox.clear();
	m_messagesHandler.receiveMessages(&m_inbox,&m_inboxAllocator,getRank());

	#ifdef ASSERT
	int receivedMessages=m_inbox.size();
	assert(receivedMessages<=MAX_ALLOCATED_MESSAGES_IN_INBOX);
	#endif
}

void Machine::call_RAY_SLAVE_MODE_SEND_SEED_LENGTHS(){
	m_seedingData->sendSeedLengths();
}

void Machine::call_RAY_MASTER_MODE_LOAD_CONFIG(){
	if(m_argc==2){
		ifstream f(m_argv[1]);
		if(!f){
			cout<<"Rank "<<getRank()<<" invalid input file."<<endl;
			m_parameters.showUsage();
			m_aborted=true;
			f.close();
			killRanks();
			m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
			return;
		}
	}

	if(m_parameters.getError()){
		m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
		m_aborted=true;
		killRanks();
		return;
	}

	uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(2*sizeof(uint64_t));
	message[0]=m_parameters.getWordSize();
	message[1]=m_parameters.getColorSpaceMode();

	for(int i=0;i<getSize();i++){
		Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_SET_WORD_SIZE,getRank());
		m_outbox.push_back(aMessage);
	}

	m_master_mode=RAY_MASTER_MODE_LOAD_SEQUENCES;
}

void Machine::call_RAY_MASTER_MODE_LOAD_SEQUENCES(){
	bool res=m_sl.computePartition(getRank(),getSize(),
	&m_outbox,
	&m_outboxAllocator,
	&m_loadSequenceStep,
	m_bubbleData,
	&m_lastTime,
	&m_parameters,&m_master_mode,&m_slave_mode
);
	if(!res){
		m_aborted=true;
		killRanks();
		m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
		m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
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
			MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_LOAD_SEQUENCES,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_SLAVE_MODE_LOAD_SEQUENCES(){
	m_sl.loadSequences(getRank(),getSize(),
	&m_outbox,
	&m_outboxAllocator,
	&m_loadSequenceStep,
	m_bubbleData,
	&m_lastTime,
	&m_parameters,&m_master_mode,&m_slave_mode
);
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION(){
	m_timePrinter.printElapsedTime("Sequence partitioning");
	cout<<endl;
	
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_messageSentForVerticesDistribution=true;
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_START_EDGES_DISTRIBUTION(){
	m_numberOfMachinesReadyForEdgesDistribution=-1;
}

void Machine::call_RAY_MASTER_MODE_SEND_COVERAGE_VALUES(){
	if(m_coverageDistribution.size()==0){
		cout<<endl;
		cout<<"Rank 0: Assembler panic: no k-mers found in reads."<<endl;
		cout<<"Rank 0: Perhaps reads are shorter than the k-mer length (change -k)."<<endl;
		m_aborted=true;
		m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
		killRanks();
		return;
	}
	m_numberOfMachinesDoneSendingCoverage=-1;
	string file=m_parameters.getCoverageDistributionFile();
	CoverageDistribution distribution(&m_coverageDistribution,&file);

	m_minimumCoverage=distribution.getMinimumCoverage();
	m_peakCoverage=distribution.getPeakCoverage();
	int repeatCoverage=distribution.getRepeatCoverage();
	printf("\n");
	fflush(stdout);

	cout<<endl;
	cout<<"Rank "<<getRank()<<": the minimum coverage is "<<m_minimumCoverage<<endl;
	cout<<"Rank "<<getRank()<<": the peak coverage is "<<m_peakCoverage<<endl;

	if(m_parameters.writeKmers()){
		cout<<endl;
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getPrefix()<<".kmers.txt"<<endl;
	}

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
		if(coverageValue>=m_minimumCoverage){
			genomeKmers+=vertices;
		}
		numberOfVertices+=vertices;
	}
	double percentageSeenOnce=(0.0+verticesWith1Coverage)/numberOfVertices*100.00;

	ostringstream g;
	g<<m_parameters.getPrefix();
	g<<".CoverageDistributionAnalysis.txt";
	ofstream outputFile(g.str().c_str());
	outputFile<<"k-mer length:\t"<<m_parameters.getWordSize()<<endl;
	outputFile<<"Lowest coverage observed:\t"<<lowestCoverage<<endl;
	outputFile<<"MinimumCoverage:\t"<<m_minimumCoverage<<endl;
	outputFile<<"PeakCoverage:\t"<<m_peakCoverage<<endl;
	outputFile<<"RepeatCoverage:\t"<<repeatCoverage<<endl;
	outputFile<<"Number of k-mers with at least MinimumCoverage:\t"<<genomeKmers<<" k-mers"<<endl;
	outputFile<<"Estimated genome length:\t"<<genomeKmers/2<<" nucleotides"<<endl;
	outputFile<<"Percentage of vertices with coverage "<<lowestCoverage<<":\t"<<percentageSeenOnce<<" %"<<endl;
	outputFile<<"DistributionFile: "<<file<<endl;

	outputFile.close();

	m_coverageDistribution.clear();

	if(m_minimumCoverage > m_peakCoverage || m_peakCoverage==m_parameters.getRepeatCoverage()
	|| m_peakCoverage==1){
		killRanks();
		m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
		m_aborted=true;
		cout<<"Rank 0: Assembler panic: no peak observed in the k-mer coverage distribution."<<endl;
		cout<<"Rank 0: to deal with the sequencing error rate, try to lower the k-mer length (-k)"<<endl;
		return;
	}

	// see these values to everyone.
	uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(3*sizeof(uint64_t));
	buffer[0]=m_minimumCoverage;
	buffer[1]=m_peakCoverage;
	buffer[2]=repeatCoverage;
	m_numberOfRanksWithCoverageData=0;
	for(int i=0;i<getSize();i++){
		Message aMessage(buffer,3,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_SEND_COVERAGE_VALUES,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_DO_NOTHING(){}

void Machine::call_RAY_SLAVE_MODE_DO_NOTHING(){}

void Machine::call_RAY_SLAVE_MODE_EXTRACT_VERTICES(){
	m_verticesExtractor.process(		&m_mode_send_vertices_sequence_id,
			&m_myReads,
			&m_reverseComplementVertex,
			getRank(),
			&m_outbox,
			&m_mode_send_vertices,
			m_wordSize,
			getSize(),
			&m_outboxAllocator,
			&m_slave_mode
		);
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_INDEXING(){
	m_numberOfMachinesDoneSendingEdges=-9;
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
	m_timePrinter.printElapsedTime("Coverage distribution analysis");
	cout<<endl;

	cout<<endl;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_INDEXING_SEQUENCES,getRank());
		m_outbox.push_back(aMessage);
	}
}

void Machine::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS(){
	cout<<endl;
	m_numberOfMachinesDoneSendingVertices=-1;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG, i, RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS(){
	if(!m_coverageInitialised){
		m_timePrinter.printElapsedTime("Graph construction");
		cout<<endl;
		m_coverageInitialised=true;
		m_coverageRank=0;
	}

	if(m_parameters.writeKmers()){
		if(m_coverageRank==m_numberOfMachinesDoneSendingCoverage){
			Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG,m_coverageRank,
				RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
			m_coverageRank++;
		}

		if(m_coverageRank==m_parameters.getSize())
			m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
	}else{
		for(m_coverageRank=0;m_coverageRank<m_parameters.getSize();m_coverageRank++){
			Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG,m_coverageRank,
				RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
	}

}

void Machine::call_RAY_MASTER_MODE_PREPARE_SEEDING(){
	m_ranksDoneAttachingReads=-1;
	m_readyToSeed=getSize();
	m_master_mode=RAY_MASTER_MODE_TRIGGER_SEEDING;
}

void Machine::call_RAY_SLAVE_MODE_ASSEMBLE_WAVES(){
	// take each seed, and extend it in both direction using previously obtained information.
	if(m_seedingData->m_SEEDING_i==(uint64_t)m_seedingData->m_SEEDING_seeds.size()){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE,getRank());
		m_outbox.push_back(aMessage);
	}else{
	}
}

void Machine::call_RAY_SLAVE_MODE_FINISH_FUSIONS(){
	m_fusionData->finishFusions();
}

void Machine::call_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS(){
	m_fusionData->distribute(m_seedingData,m_ed,m_rank,&m_outboxAllocator,&m_outbox,getSize(),&m_slave_mode);
}

void Machine::call_RAY_SLAVE_MODE_SEND_DISTRIBUTION(){
	m_coverageGatherer.work();
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_SEEDING(){
	m_timePrinter.printElapsedTime("Selection of optimal read markers");
	cout<<endl;
	m_readyToSeed=-1;
	m_numberOfRanksDoneSeeding=0;
	// tell everyone to seed now.
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_SEEDING,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_SLAVE_MODE_START_SEEDING(){

	#ifdef ASSERT
	assert(m_subgraph.frozen());
	#endif

	m_seedingData->computeSeeds();
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_DETECTION(){
	m_timePrinter.printElapsedTime("Detection of assembly seeds");
	cout<<endl;
	m_numberOfRanksDoneSeeding=-1;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_numberOfRanksDoneDetectingDistances=0;
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_ASK_DISTANCES(){
	m_numberOfRanksDoneDetectingDistances=-1;
	m_numberOfRanksDoneSendingDistances=0;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_START_UPDATING_DISTANCES(){
	m_numberOfRanksDoneSendingDistances=-1;
	m_parameters.computeAverageDistances();
	m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
	m_master_mode=RAY_MASTER_MODE_UPDATE_DISTANCES;
	m_fileId=0;
	m_sequence_idInFile=0;
	m_sequence_id=0;
}

void Machine::call_RAY_MASTER_MODE_INDEX_SEQUENCES(){
}

void Machine::call_RAY_SLAVE_MODE_INDEX_SEQUENCES(){
	m_si.attachReads(&m_myReads,&m_outboxAllocator,&m_outbox,&m_slave_mode,m_wordSize,
	m_size,m_rank);
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_EXTENSIONS(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_ASK_EXTENSION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_SLAVE_MODE_SEND_EXTENSION_DATA(){
	cout<<"Rank "<<m_rank<< " is appending its fusions"<<endl;
	string output=m_parameters.getOutputFile();
	ofstream fp;
	if(m_rank==0){
		fp.open(output.c_str());
	}else{
		fp.open(output.c_str(),ios_base::out|ios_base::app);
	}
	int total=0;
	for(int i=0;i<(int)m_ed->m_EXTENSION_contigs.size();i++){
		uint64_t uniqueId=m_ed->m_EXTENSION_identifiers[i];
		if(m_fusionData->m_FUSION_eliminated.count(uniqueId)>0){
			continue;
		}
		total++;
		string contig=convertToString(&(m_ed->m_EXTENSION_contigs[i]),m_parameters.getWordSize(),m_parameters.getColorSpaceMode());
		
		m_scaffolder.addContig(uniqueId,&(m_ed->m_EXTENSION_contigs[i]));

		string withLineBreaks=addLineBreaks(contig,m_parameters.getColumns());
		fp<<">contig-"<<uniqueId<<" "<<contig.length()<<" nucleotides"<<endl<<withLineBreaks<<endl;
	}
	cout<<"Rank "<<m_rank<<" appended "<<total<<" elements"<<endl;
	fp.close();

	if(m_parameters.showMemoryUsage()){
		showMemoryUsage(getRank());
	}

	m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_EXTENSION_DATA_END,getRank());
	m_outbox.push_back(aMessage);
}

void Machine::call_RAY_SLAVE_MODE_FUSION(){
	m_fusionData->makeFusions();
}

void Machine::call_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION(){
	m_library.detectDistances();
}

void Machine::call_RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES(){
	m_library.sendLibraryDistances();
}

void Machine::call_RAY_MASTER_MODE_UPDATE_DISTANCES(){
	m_library.updateDistances();
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_FUSIONS(){
	m_timePrinter.printElapsedTime("Bidirectional extension of seeds");
	cout<<endl;

	// ask one at once to do the fusion
	// because otherwise it may lead to hanging of the program for unknown reasons
	m_ed->m_EXTENSION_numberOfRanksDone=-1;
	m_fusionData->m_FUSION_numberOfRanksDone=0;
	for(int i=0;i<(int)getSize();i++){// start fusion.
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_FUSION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_fusionData->m_fusionStarted=true;
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS(){

	m_reductionOccured=true;
	m_master_mode=RAY_MASTER_MODE_START_FUSION_CYCLE;
	m_cycleStarted=false;
	m_cycleNumber=0;
	m_mustStop=false;
}

void Machine::call_RAY_MASTER_MODE_START_FUSION_CYCLE(){
	// the finishing is
	//
	//  * a clear cycle
	//  * a distribute cycle
	//  * a finish cycle
	//  * a clear cycle
	//  * a distribute cycle
	//  * a fusion cycle

	if(!m_cycleStarted){
		int count=0;
		if(m_mustStop){
			count=1;
		}

		uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		m_nextReductionOccured=false;
		m_cycleStarted=true;
		m_isFinalFusion=false;
		for(int i=0;i<getSize();i++){
			Message aMessage(buffer,count,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_CLEAR_DIRECTIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_currentCycleStep=1;
		m_CLEAR_n=0;
	}else if(m_CLEAR_n==getSize() && !m_isFinalFusion && m_currentCycleStep==1){
		m_currentCycleStep++;
		m_CLEAR_n=-1;

		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_DISTRIBUTE_n=0;
	}else if(m_DISTRIBUTE_n==getSize() && !m_isFinalFusion && m_currentCycleStep==2){
		m_currentCycleStep++;
		m_DISTRIBUTE_n=-1;
		m_isFinalFusion=true;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_FINISH_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_FINISH_n=0;
	}else if(m_FINISH_n==getSize() && m_isFinalFusion && m_currentCycleStep==3){
		m_currentCycleStep++;
		int count=0;
		if(m_mustStop){
			count=1;
		}
		uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		for(int i=0;i<getSize();i++){
			Message aMessage(buffer,count,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_CLEAR_DIRECTIONS,getRank());
			m_outbox.push_back(aMessage);
		}

		m_FINISH_n=-1;
		m_CLEAR_n=0;
	}else if(m_CLEAR_n==getSize() && m_isFinalFusion && m_currentCycleStep==4){
		m_CLEAR_n=-1;
		m_currentCycleStep++;

		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_DISTRIBUTE_n=0;
	}else if(m_DISTRIBUTE_n==getSize() && m_isFinalFusion && m_currentCycleStep==5){
		m_currentCycleStep++;

		if(m_mustStop){
			m_timePrinter.printElapsedTime("Merging of redundant contigs");
			cout<<endl;
			m_master_mode=RAY_MASTER_MODE_ASK_EXTENSIONS;

			m_ed->m_EXTENSION_currentRankIsSet=false;
			m_ed->m_EXTENSION_rank=-1;
			return;
		}

		cout<<"Rank 0 tells others to compute fusions."<<endl;
		m_fusionData->m_FUSION_numberOfRanksDone=0;
		m_DISTRIBUTE_n=-1;
		for(int i=0;i<(int)getSize();i++){// start fusion.
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_FUSION,getRank());
			m_outbox.push_back(aMessage);
		}
		
	}else if(m_fusionData->m_FUSION_numberOfRanksDone==getSize() && m_isFinalFusion && m_currentCycleStep==6){
		m_reductionOccured=m_nextReductionOccured;
		m_fusionData->m_FUSION_numberOfRanksDone=-1;
		if(!m_reductionOccured || m_cycleNumber ==5){ 
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
			m_master_mode=RAY_MASTER_MODE_AMOS;
			m_ed->m_EXTENSION_currentRankIsStarted=false;
			m_ed->m_EXTENSION_currentPosition=0;
			m_ed->m_EXTENSION_rank=0;
			m_seedingData->m_SEEDING_i=0;
			m_mode_send_vertices_sequence_id_position=0;
			m_ed->m_EXTENSION_reads_requested=false;
			cout<<endl;
		}else{
			m_master_mode=RAY_MASTER_MODE_SCAFFOLDER;
			m_scaffolder.m_numberOfRanksFinished=0;
		}
		
	}else if(!m_ed->m_EXTENSION_currentRankIsStarted){
		m_ed->m_EXTENSION_currentRankIsStarted=true;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_ed->m_EXTENSION_rank,RAY_MPI_TAG_ASK_EXTENSION_DATA,getRank());
		m_outbox.push_back(aMessage);
		m_ed->m_EXTENSION_currentRankIsDone=false;
	}else if(m_ed->m_EXTENSION_currentRankIsDone){
		m_ed->m_EXTENSION_currentRankIsSet=false;
	}
}

void Machine::call_RAY_MASTER_MODE_AMOS(){
	m_amos.masterMode();
}

void Machine::call_RAY_MASTER_MODE_SCAFFOLDER(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_SCAFFOLDER,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_SLAVE_MODE_SCAFFOLDER(){
	m_scaffolder.run();
}

void Machine::call_RAY_SLAVE_MODE_AMOS(){
	m_amos.slaveMode();
}

void Machine::call_RAY_SLAVE_MODE_EXTENSION(){
	int maxCoverage=m_parameters.getRepeatCoverage();
	m_seedExtender.extendSeeds(&(m_seedingData->m_SEEDING_seeds),m_ed,getRank(),&m_outbox,&(m_seedingData->m_SEEDING_currentVertex),
	m_fusionData,&m_outboxAllocator,&(m_seedingData->m_SEEDING_edgesRequested),&(m_seedingData->m_SEEDING_outgoingEdgeIndex),
	&m_last_value,&(m_seedingData->m_SEEDING_vertexCoverageRequested),m_wordSize,getSize(),&(m_seedingData->m_SEEDING_vertexCoverageReceived),
	&(m_seedingData->m_SEEDING_receivedVertexCoverage),&m_repeatedLength,&maxCoverage,&(m_seedingData->m_SEEDING_receivedOutgoingEdges),&m_c,
	m_bubbleData,
m_minimumCoverage,&m_oa,&(m_seedingData->m_SEEDING_edgesReceived),&m_slave_mode);
}

void Machine::call_RAY_SLAVE_MODE_DELETE_VERTICES(){
	if(m_verticesExtractor.deleteVertices(m_reducer.getVerticesToRemove(),&m_subgraph,
&m_parameters,&m_outboxAllocator,&m_outbox,
m_reducer.getIngoingEdges(),m_reducer.getOutgoingEdges()
)){
		// flush

		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_DELETE_VERTICES_DONE,getRank());
		m_outbox.push_back(aMessage);
		m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
		m_reducer.destructor();
	}
}

void Machine::processData(){
	MachineMethod masterMethod=m_master_methods[m_master_mode];
	(this->*masterMethod)();

	MachineMethod slaveMethod=m_slave_methods[m_slave_mode];
	(this->*slaveMethod)();
}

void Machine::call_RAY_MASTER_MODE_KILL_RANKS(){
	if(m_scaffolder.m_numberOfRanksFinished==getSize()){
		m_timePrinter.printElapsedTime("Scaffolding of contigs");
	}

	killRanks();
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::killRanks(){
	if(m_killed){
		return;
	}

	m_killed=true;
	for(int i=getSize()-1;i>=0;i--){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON,getRank());
		m_outbox.push_back(aMessage);
	}
}

bool Machine::isMaster(){
	return getRank()==MASTER_RANK;
}

int Machine::getSize(){
	return m_size;
}

bool Machine::isAlive(){
	return m_alive;
}

Machine::~Machine(){
	delete m_bubbleData;
	m_bubbleData=NULL;
}

void Machine::call_RAY_MASTER_MODE_ASK_BEGIN_REDUCTION(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_ASK_BEGIN_REDUCTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_START_REDUCTION(){
	printf("\nRank %i asks all ranks to reduce memory consumption\n",getRank());
	fflush(stdout);
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_REDUCTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_SLAVE_MODE_REDUCE_MEMORY_CONSUMPTION(){
	if(m_reducer.reduce(&m_subgraph,&m_parameters,
&(m_seedingData->m_SEEDING_edgesRequested),&(m_seedingData->m_SEEDING_vertexCoverageRequested),&(m_seedingData->m_SEEDING_vertexCoverageReceived),
	&m_outboxAllocator,getSize(),getRank(),&m_outbox,
&(m_seedingData->m_SEEDING_receivedVertexCoverage),m_seedingData,
m_minimumCoverage,&(m_seedingData->m_SEEDING_edgesReceived)
)){
		m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_REDUCE_MEMORY_CONSUMPTION_DONE,getRank());
		m_outbox.push_back(aMessage);
		m_verticesExtractor.prepareDeletions();
	}
}

void Machine::call_RAY_MASTER_MODE_RESUME_VERTEX_DISTRIBUTION(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_RESUME_VERTEX_DISTRIBUTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::assignMasterHandlers(){
	#define MACRO_LIST_ITEM(x) m_master_methods[x]=&Machine::call_ ## x ;
	#include <core/master_mode_macros.h>
	#undef MACRO_LIST_ITEM
}

void Machine::assignSlaveHandlers(){
	#define MACRO_LIST_ITEM(x) m_slave_methods[x]=&Machine::call_ ## x ;
	#include <core/slave_mode_macros.h>
	#undef MACRO_LIST_ITEM
}
