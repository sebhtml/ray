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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#include <GridTableIterator.h>
#include<crypto.h>
#include<SplayNode.h>
#include<mpi.h>
#include<Machine.h>
#include<VerticesExtractor.h>
#include<sstream>
#include<Message.h>
#include<time.h>
#include<TipWatchdog.h>
#include<BubbleTool.h>
#include<assert.h>
#include<common_functions.h>
#include<iostream>
#include<fstream>
#include<CoverageDistribution.h>
#include<string.h>
#include<SplayTreeIterator.h>
#include<Read.h>
#include<Loader.h>
#include<MyAllocator.h>
#include<algorithm>
#include<unistd.h>

bool myComparator_sort(const vector<uint64_t>&a,const vector<uint64_t>&b){
	return a.size()>b.size();
}

using namespace std;

Machine::Machine(int argc,char**argv){
	m_argc=argc;
	m_argv=argv;
	m_bubbleData=new BubbleData();
	#ifdef SHOW_SENT_MESSAGES
	m_stats=new StatisticsData();
	#endif
	m_fusionData=new FusionData();
	m_seedingData=new SeedingData();

	m_ed=new ExtensionData();
	m_sd=new ScaffolderData();

	assignMasterHandlers();
	assignSlaveHandlers();
}

void Machine::start(){

	m_killed=false;
	m_ready=true;
	

	m_fusionData->m_fusionStarted=false;
	m_ed->m_EXTENSION_numberOfRanksDone=0;
	m_colorSpaceMode=false;
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

	MPI_Init(&m_argc,&m_argv);

	char serverName[1000];
	int len;
	MPI_Get_processor_name(serverName,&len);
	MPI_Comm_rank(MPI_COMM_WORLD,&m_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&m_size);
	
	MPI_Barrier(MPI_COMM_WORLD);

	if(isMaster()){
		cout<<endl<<"**************************************************"<<endl;
    		cout<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    		cout<<"This is free software, and you are welcome to redistribute it"<<endl;
    		cout<<"under certain conditions; see \"COPYING\" for details."<<endl;
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

	MPI_Barrier(MPI_COMM_WORLD);

	
	m_parameters.setSize(getSize());
	MAX_ALLOCATED_MESSAGES_IN_OUTBOX=getSize();
	MAX_ALLOCATED_MESSAGES_IN_INBOX=1;

	// this peak is attained in VerticesExtractor::deleteVertices
	m_maximumAllocatedOutputBuffers=17; 

	if(MAX_ALLOCATED_MESSAGES_IN_OUTBOX<m_maximumAllocatedOutputBuffers){
		MAX_ALLOCATED_MESSAGES_IN_OUTBOX=m_maximumAllocatedOutputBuffers;
	}

	m_inboxAllocator.constructor(MAX_ALLOCATED_MESSAGES_IN_INBOX,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_outboxAllocator.constructor(m_maximumAllocatedOutputBuffers,MAXIMUM_MESSAGE_SIZE_IN_BYTES);

	m_inbox.constructor(MAX_ALLOCATED_MESSAGES_IN_INBOX);
	m_outbox.constructor(MAX_ALLOCATED_MESSAGES_IN_OUTBOX);

	int PERSISTENT_ALLOCATOR_CHUNK_SIZE=4194304; // 4 MiB
	m_persistentAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);

	int directionAllocatorChunkSize=4194304; // 4 MiB
	m_directionsAllocator.constructor(directionAllocatorChunkSize);

	m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
	m_mode_AttachSequences=false;
	m_startEdgeDistribution=false;

	m_ranksDoneAttachingReads=0;
	m_reducer.constructor(getSize());

	int pid=getpid();
	printf("Rank %i is running as UNIX process %i on %s\n",getRank(),pid,serverName);
	fflush(stdout);

	MPI_Barrier(MPI_COMM_WORLD);

	m_si.constructor(&m_parameters,&m_outboxAllocator,&m_inbox,&m_outbox,&m_diskAllocator);

	int maximumNumberOfProcesses=65536;
	if(getSize()>maximumNumberOfProcesses){
		cout<<"The maximum number of processes is "<<maximumNumberOfProcesses<<" (this can be changed in the code)"<<endl;
	}
	assert(getSize()<=maximumNumberOfProcesses);

	int version;
	int subversion;
	MPI_Get_version(&version,&subversion);

	if(isMaster()){
		cout<<endl;
		cout<<"Rank "<<MASTER_RANK<<": Ray "<<RAY_VERSION<<endl;

		#ifdef __GNUC__
		cout<<"Rank "<<MASTER_RANK<<": GNU detected."<<endl;
		#endif

		#ifdef MPICH2
                cout<<"Rank "<<MASTER_RANK<<": compiled with MPICH2 "<<MPICH2_VERSION<<endl;
		#endif

		#ifdef OMPI_MPI_H
                cout<<"Rank "<<MASTER_RANK<<": compiled with Open-MPI "<<OMPI_MAJOR_VERSION<<"."<<OMPI_MINOR_VERSION<<"."<<OMPI_RELEASE_VERSION<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": MPI library implements the standard MPI "<<version<<"."<<subversion<<""<<endl;

		// show libraries
		#ifdef HAVE_ZLIB
		cout<<"Rank "<<MASTER_RANK<<": compiled with GZIP"<<endl;
		#endif

		#ifdef HAVE_LIBBZ2
		cout<<"Rank "<<MASTER_RANK<<": compiled with BZIP2"<<endl;
		#endif

		// show OS, only Linux

		#ifdef linux
		cout<<"Rank "<<MASTER_RANK<<": compiled for GNU/Linux, using /proc for memory usage data"<<endl;
		#endif

		#ifdef ASSERT
		cout<<"Rank "<<MASTER_RANK<<": compiled with assertions"<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": the maximum size of a message is "<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<" bytes"<<endl;
		cout<<"Rank "<<MASTER_RANK<<": align addresses on 8 bytes: ";
		#ifndef FORCE_PACKING
		cout<<" yes (FORCE_PACKING is undefined)";
		#else
		cout<<" no (FORCE_PACKING is defined)";
		#endif
		cout<<endl;
		cout<<endl;

		cout<<" sizeof(Vertex)="<<sizeof(Vertex)<<endl;
		cout<<" sizeof(VertexData)="<<sizeof(VertexData)<<endl;
		cout<<" sizeof(Direction)="<<sizeof(Direction)<<endl;
		cout<<" sizeof(ReadAnnotation)="<<sizeof(ReadAnnotation)<<endl;
		cout<<" sizeof(Read)="<<sizeof(Read)<<endl;
		cout<<" sizeof(PairedRead)="<<sizeof(PairedRead)<<endl;
		
		cout<<endl;
		size_t page_size = (size_t) sysconf (_SC_PAGESIZE);
		cout<<"PageSize: "<<page_size<<" bytes"<<endl;
		cout<<endl;
		m_timePrinter.printElapsedTime("Beginning of computation");
		cout<<endl;
	}

	m_parameters.constructor(m_argc,m_argv,getRank());

	m_seedExtender.constructor(&m_parameters,&m_directionsAllocator,m_ed,&m_subgraph,&m_inbox);
	ostringstream prefixFull;
	prefixFull<<m_parameters.getMemoryPrefix()<<"_Main";
	int chunkSize=16777216;
	m_diskAllocator.constructor(chunkSize);

	m_sl.constructor(m_size,&m_diskAllocator,&m_myReads);

	m_fusionData->constructor(getSize(),MAXIMUM_MESSAGE_SIZE_IN_BYTES,getRank(),&m_outbox,&m_outboxAllocator,m_parameters.getWordSize(),
	m_parameters.getColorSpaceMode(),
		m_ed,m_seedingData,&m_slave_mode,&m_parameters);

	m_virtualCommunicator.constructor(m_rank,m_size,&m_outboxAllocator,&m_inbox,&m_outbox);
	m_virtualCommunicator.setReplyType(RAY_MPI_TAG_REQUEST_VERTEX_READS,RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY);
	m_virtualCommunicator.setElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_READS,5);

	m_virtualCommunicator.setReplyType(RAY_MPI_TAG_GET_READ_MATE,RAY_MPI_TAG_GET_READ_MATE_REPLY);
	m_virtualCommunicator.setElementsPerQuery(RAY_MPI_TAG_GET_READ_MATE,4);

	m_library.constructor(getRank(),&m_outbox,&m_outboxAllocator,&m_sequence_id,&m_sequence_idInFile,
		m_ed,getSize(),&m_timePrinter,&m_slave_mode,&m_master_mode,
	&m_parameters,&m_fileId,m_seedingData,&m_inbox,&m_virtualCommunicator);

	m_subgraph.constructor(getRank(),&m_diskAllocator);
	
	m_seedingData->constructor(&m_seedExtender,getRank(),getSize(),&m_outbox,&m_outboxAllocator,&m_seedCoverage,&m_slave_mode,&m_parameters,&m_wordSize,&m_subgraph,
		&m_colorSpaceMode,&m_inbox);

	m_alive=true;
	m_loadSequenceStep=false;
	m_totalLetters=0;

	MPI_Barrier(MPI_COMM_WORLD);
	showMemoryUsage(getRank());

	MPI_Barrier(MPI_COMM_WORLD);

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
	&m_colorSpaceMode,
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
	&m_sd->m_allIdentifiers,&m_oa,
	&m_numberOfRanksWithCoverageData,&m_seedExtender,
	&m_master_mode,&m_isFinalFusion,&m_si);

	m_messagesHandler.constructor(getRank(),getSize());

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
		m_timePrinter.printElapsedTime("Collection of fusions");
		m_timePrinter.printDurations();

		cout<<endl;
		#ifdef COUNT_MESSAGES
		string file=m_parameters.getReceivedMessagesFile();
		const char*tmp=file.c_str();
		m_messagesHandler.writeStats(tmp);
		#endif

	}

	MPI_Barrier(MPI_COMM_WORLD);

	showMemoryUsage(getRank());

	MPI_Barrier(MPI_COMM_WORLD);

	if(isMaster() && !m_aborted){
		cout<<endl;
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getCoverageDistributionFile()<<" (how redundant are the k-mers)"<<endl;
		m_parameters.printFinalMessage();
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getOutputFile()<<" (contiguous sequences in FASTA format) "<<endl;
		if(m_parameters.useAmos()){
			cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getAmosFile()<<" (reads mapped onto contiguous sequences in AMOS format)"<<endl;

		}
		#ifdef COUNT_MESSAGES
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getReceivedMessagesFile()<<" (MPI communication matrix; rows=destinations, columns=sources) "<<endl;
		#endif
		cout<<endl;
	}

	MPI_Barrier(MPI_COMM_WORLD);
	m_messagesHandler.freeLeftovers();
	m_persistentAllocator.clear();
	m_directionsAllocator.clear();
	m_inboxAllocator.clear();
	m_outboxAllocator.clear();

	m_diskAllocator.clear();

	MPI_Finalize();
}

void Machine::run(){
	while(isAlive()){
	#ifdef SHOW_ALIVE
		time_t t=time(NULL);
		if(t!=m_lastTime&&t%30==0){
			time_t m_endingTime=time(NULL);
			struct tm * timeinfo;
			timeinfo=localtime(&m_endingTime);
			cout<<"Date: ";
			printf("Rank %i is alive %s",m_rank,asctime(timeinfo));
			showMemoryUsage(m_rank);
			fflush(stdout);
			m_lastTime=t;
		}
	#endif
		receiveMessages(); 
		processMessages();
		processData();
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
			cout<<" "<<m_outbox[i]->getTag();
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
		messageInInts[1+i]=m_parameters.getNumberOfSequences(i);
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
	m_timePrinter.printElapsedTime("Distribution of sequence reads");
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
	m_numberOfMachinesDoneSendingCoverage=-1;
	string file=m_parameters.getCoverageDistributionFile();
	CoverageDistribution distribution(&m_coverageDistribution,&file);

	m_minimumCoverage=distribution.getMinimumCoverage();
	m_peakCoverage=distribution.getPeakCoverage();

	printf("\n");
	fflush(stdout);

	cout<<"Rank "<<getRank()<<": the minimum coverage is "<<m_minimumCoverage<<endl;
	cout<<"Rank "<<getRank()<<": the peak coverage is "<<m_peakCoverage<<endl;

	//int diff=m_peakCoverage-m_minimumCoverage;
	//m_seedCoverage=m_minimumCoverage+(3*diff)/4;
	m_seedCoverage=(m_minimumCoverage+m_peakCoverage)/2;

	m_coverageDistribution.clear();

	if(m_minimumCoverage > m_peakCoverage or m_peakCoverage==m_parameters.getMaxCoverage()){
		killRanks();
		cout<<"Error: no enrichment observed."<<endl;
		return;
	}

	// see these values to everyone.
	uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(3*sizeof(uint64_t));
	buffer[0]=m_minimumCoverage;
	buffer[1]=m_seedCoverage;
	buffer[2]=m_peakCoverage;
	m_numberOfRanksWithCoverageData=0;
	for(int i=0;i<getSize();i++){
		Message aMessage(buffer,3,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_SEND_COVERAGE_VALUES,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_DO_NOTHING(){
}

void Machine::call_RAY_SLAVE_MODE_DO_NOTHING(){
}

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
			m_colorSpaceMode,&m_slave_mode
		);
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_INDEXING(){
	m_numberOfMachinesDoneSendingEdges=-9;
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
	m_timePrinter.printElapsedTime("Calculation of coverage distribution");
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
	m_numberOfMachinesReadyToSendDistribution=-1;
	m_timePrinter.printElapsedTime("Distribution of vertices & edges");
	cout<<endl;

	for(int i=0;i<getSize();i++){
		Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
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
	if(m_distributionOfCoverage.size()==0){
		#ifdef ASSERT
		uint64_t n=0;
		#endif
		GridTableIterator iterator;
		//MyForestIterator iterator;
		iterator.constructor(&m_subgraph,m_wordSize);
		while(iterator.hasNext()){
			Vertex*node=iterator.next();
			uint64_t key=iterator.getKey();
			//cout<<idToWord(key,m_wordSize)<<endl;
			//SplayNode<uint64_t,Vertex>*node=iterator.next();
			int coverage=node->getCoverage(key);
			//cout<<coverage<<endl;
			m_distributionOfCoverage[coverage]++;
			#ifdef ASSERT
			n++;
			#endif
		}
		#ifdef ASSERT
		if(n!=m_subgraph.size()){
			cout<<"n="<<n<<" size="<<m_subgraph.size()<<endl;
		}
		assert(n==m_subgraph.size());
		#endif
		m_waiting=false;
		m_coverageIterator=m_distributionOfCoverage.begin();
	}else if(m_waiting){
		if(m_inbox.size()>0&&m_inbox[0]->getTag()==RAY_MPI_TAG_COVERAGE_DATA_REPLY){
			m_waiting=false;
		}
	}else{
		uint64_t*messageContent=(uint64_t*)m_outboxAllocator.allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int count=0;
		int maximumElements=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);
		while(count<maximumElements && m_coverageIterator!=m_distributionOfCoverage.end()){
			int coverage=m_coverageIterator->first;
			uint64_t numberOfVertices=m_coverageIterator->second;
			messageContent[count]=coverage;
			messageContent[count+1]=numberOfVertices;
			count+=2;
			m_coverageIterator++;
		}

		if(count!=0){
			Message aMessage(messageContent,count,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_COVERAGE_DATA,getRank());
			
			m_outbox.push_back(aMessage);
			m_waiting=true;
		}else{
			m_distributionOfCoverage.clear();
			m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
			m_subgraph.buildData();
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_COVERAGE_END,getRank());
			m_outbox.push_back(aMessage);
		}
	}
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_SEEDING(){
	m_timePrinter.printElapsedTime("Indexing of sequence reads");
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
	m_timePrinter.printElapsedTime("Computation of seeds");
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
	m_size,m_rank,m_colorSpaceMode);
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
	FILE*fp;
	if(m_rank==0){
		fp=fopen(output.c_str(),"w+");
	}else{
		fp=fopen(output.c_str(),"a+");
	}
	int total=0;
	for(int i=0;i<(int)m_ed->m_EXTENSION_contigs.size();i++){
		uint64_t uniqueId=m_ed->m_EXTENSION_identifiers[i];
		if(m_fusionData->m_FUSION_eliminated.count(uniqueId)>0){
			continue;
		}
		total++;
		string contig=convertToString(&(m_ed->m_EXTENSION_contigs[i]),m_parameters.getWordSize());
		string withLineBreaks=addLineBreaks(contig);
		fprintf(fp,">contig-%lu %i nucleotides\n%s",uniqueId,(int)contig.length(),withLineBreaks.c_str());
	}
	cout<<"Rank "<<m_rank<<" appended "<<total<<" elements"<<endl;
	fclose(fp);
	showMemoryUsage(getRank());

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
	m_timePrinter.printElapsedTime("Extension of seeds");
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
		#ifdef SHOW_PROGRESS
		#endif
		m_nextReductionOccured=false;
		m_cycleStarted=true;
		m_isFinalFusion=false;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_CLEAR_DIRECTIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		//cout<<"Cycle "<<m_cycleNumber<<" sending 1) RAY_MPI_TAG_CLEAR_DIRECTIONS"<<endl;
		m_currentCycleStep=1;
		m_CLEAR_n=0;
	}else if(m_CLEAR_n==getSize() and !m_isFinalFusion and m_currentCycleStep==1){
		#ifdef SHOW_PROGRESS
		#endif
		m_currentCycleStep++;
		m_CLEAR_n=-1;

		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_DISTRIBUTE_n=0;
		//cout<<"Cycle "<<m_cycleNumber<<" sending 2) RAY_MPI_TAG_DISTRIBUTE_FUSIONS"<<endl;
	}else if(m_DISTRIBUTE_n==getSize() and !m_isFinalFusion and m_currentCycleStep==2){
		#ifdef SHOW_PROGRESS
		#endif
		m_currentCycleStep++;
		m_DISTRIBUTE_n=-1;
		m_isFinalFusion=true;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_FINISH_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		//cout<<"Cycle "<<m_cycleNumber<<" sending 3) RAY_MPI_TAG_FINISH_FUSIONS"<<endl;
		m_FINISH_n=0;
	}else if(m_FINISH_n==getSize() and m_isFinalFusion and m_currentCycleStep==3){
		#ifdef SHOW_PROGRESS
		#endif
		m_currentCycleStep++;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_CLEAR_DIRECTIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		//cout<<"Cycle "<<m_cycleNumber<<" sending 4) RAY_MPI_TAG_CLEAR_DIRECTIONS"<<endl;
		m_FINISH_n=-1;
		m_CLEAR_n=0;
	}else if(m_CLEAR_n==getSize() and m_isFinalFusion && m_currentCycleStep==4){
		m_CLEAR_n=-1;
		m_currentCycleStep++;
		#ifdef SHOW_PROGRESS
		#endif

		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_DISTRIBUTE_n=0;
	}else if(m_DISTRIBUTE_n==getSize() and m_isFinalFusion && m_currentCycleStep==5){
		m_currentCycleStep++;
		#ifdef SHOW_PROGRESS
		cout<<"Rank 0 tells others to compute fusions."<<endl;

		#endif
		m_fusionData->m_FUSION_numberOfRanksDone=0;
		m_DISTRIBUTE_n=-1;
		for(int i=0;i<(int)getSize();i++){// start fusion.
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_FUSION,getRank());
			m_outbox.push_back(aMessage);
		}
		
	}else if(m_fusionData->m_FUSION_numberOfRanksDone==getSize() && m_isFinalFusion && m_currentCycleStep==6){
		
		m_reductionOccured=m_nextReductionOccured;
		m_fusionData->m_FUSION_numberOfRanksDone=-1;
		if(!m_reductionOccured or m_cycleNumber ==5){ 
			m_timePrinter.printElapsedTime("Computation of fusions");
			cout<<endl;
			m_master_mode=RAY_MASTER_MODE_ASK_EXTENSIONS;

			m_sd->m_computedTopology=false;

			m_sd->m_pathId=0;
			m_sd->m_visitedVertices.clear();
			while(!m_sd->m_verticesToVisit.empty())
				m_sd->m_verticesToVisit.pop();
			while(!m_sd->m_depthsToVisit.empty())
				m_sd->m_depthsToVisit.pop();
			m_sd->m_processedLastVertex=false;
			m_ed->m_EXTENSION_currentRankIsSet=false;
			m_ed->m_EXTENSION_rank=-1;
		}else{
			// we continue now!
			m_cycleStarted=false;
			m_cycleNumber++;
		}
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
		#ifdef SHOW_SCAFFOLDER
		#endif
		int minimumLength=500;
		if(!m_sd->m_computedTopology){ // in development.
			// for each contig path, take the last vertex, and search for other contig paths 
			// reachable from it.
			if(false and m_sd->m_pathId<(int)m_allPaths.size()){
				int currentPathId=m_identifiers[m_sd->m_pathId];
				if(!m_sd->m_processedLastVertex){
					if((int)m_allPaths[m_sd->m_pathId].size()<minimumLength){
						m_sd->m_pathId++;
						return;
					}
				
					#ifdef SHOW_SCAFFOLDER
					//cout<<"push last vertex."<<endl;
					#endif
					m_sd->m_processedLastVertex=true;
					int theLength=m_allPaths[m_sd->m_pathId].size();
					uint64_t lastVertex=m_allPaths[m_sd->m_pathId][theLength-1];
					#ifdef SHOW_SCAFFOLDER
					cout<<"contig-"<<currentPathId<<" Last="<<idToWord(lastVertex,m_wordSize)<<" "<<theLength<<" vertices"<<endl;
	
					#endif
					m_sd->m_verticesToVisit.push(lastVertex);
					m_sd->m_depthsToVisit.push(0);
					m_seedingData->m_SEEDING_edgesRequested=false;
					m_sd->m_visitedVertices.clear();
				}else if(!m_sd->m_verticesToVisit.empty()){
					uint64_t theVertex=m_sd->m_verticesToVisit.top();
					int theDepth=m_sd->m_depthsToVisit.top();
					if(!m_seedingData->m_SEEDING_edgesRequested){
						#ifdef SHOW_SCAFFOLDER
						//cout<<"Asking for arcs. "<<theVertex<<endl;
						#endif
						m_seedingData->m_SEEDING_edgesReceived=false;
						m_seedingData->m_SEEDING_edgesRequested=true;
						uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
						message[0]=(uint64_t)theVertex;
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],getSize(),m_wordSize),RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES,getRank());
						m_outbox.push_back(aMessage);
						m_fusionData->m_Machine_getPaths_DONE=false;
						m_fusionData->m_Machine_getPaths_INITIALIZED=false;
						m_fusionData->m_Machine_getPaths_result.clear();
						m_sd->m_visitedVertices.insert(theVertex);
					}else if(m_seedingData->m_SEEDING_edgesReceived){
						if(!m_fusionData->m_Machine_getPaths_DONE){
							getPaths(theVertex);
						}else{
							vector<Direction> nextPaths;
							#ifdef SHOW_SCAFFOLDER
							if(nextPaths.size()>0){
								cout<<"We have "<<nextPaths.size()<<" paths with "<<idToWord(theVertex,m_wordSize)<<endl;	
							}
							#endif
							for(int i=0;i<(int)m_fusionData->m_Machine_getPaths_result.size();i++){
								int pathId=m_fusionData->m_Machine_getPaths_result[i].getWave();
								if(pathId==currentPathId)
									continue;
								// this one is discarded.
								if(m_sd->m_allIdentifiers.count(pathId)==0){
									continue;
								}
								// not at the front.
								if(m_fusionData->m_Machine_getPaths_result[i].getProgression()>0)
									continue;
								int index=m_sd->m_allIdentifiers[pathId];

								// too small to be relevant.
								if((int)m_allPaths[index].size()<minimumLength)
									continue;
								
								nextPaths.push_back(m_fusionData->m_Machine_getPaths_result[i]);
							}

							m_sd->m_verticesToVisit.pop();
							m_sd->m_depthsToVisit.pop();
							m_seedingData->m_SEEDING_edgesRequested=false;

							if(nextPaths.size()>0){// we found a path
								for(int i=0;i<(int)nextPaths.size();i++){
									cout<<"contig-"<<m_identifiers[m_sd->m_pathId]<<" -> "<<"contig-"<<nextPaths[i].getWave()<<" ("<<theDepth<<","<<nextPaths[i].getProgression()<<") via "<<idToWord(theVertex,m_wordSize)<<endl;
									#ifdef ASSERT
									assert(m_sd->m_allIdentifiers.count(nextPaths[i].getWave())>0);
									#endif
								}
							}else{// continue the visit.
								for(int i=0;i<(int)m_seedingData->m_SEEDING_receivedOutgoingEdges.size();i++){
									uint64_t newVertex=m_seedingData->m_SEEDING_receivedOutgoingEdges[i];
									if(m_sd->m_visitedVertices.count(newVertex)>0)
										continue;
									int d=theDepth+1;
									if(d>3000)
										continue;
									m_sd->m_verticesToVisit.push(newVertex);
									m_sd->m_depthsToVisit.push(d);
								}
							}

						}
					}
				}else{
					m_sd->m_processedLastVertex=false;
					m_sd->m_pathId++;
					#ifdef SHOW_SCAFFOLDER
					//cout<<"Processing next."<<endl;
					#endif
				}
			}else{
				m_sd->m_computedTopology=true;
			}
			return;
		}

		m_master_mode=RAY_MASTER_MODE_DO_NOTHING;

		if(m_parameters.useAmos()){
			m_master_mode=RAY_MASTER_MODE_AMOS;
			m_ed->m_EXTENSION_currentRankIsStarted=false;
			m_ed->m_EXTENSION_currentPosition=0;
			m_ed->m_EXTENSION_rank=0;
			m_seedingData->m_SEEDING_i=0;
			m_mode_send_vertices_sequence_id_position=0;
			m_ed->m_EXTENSION_reads_requested=false;
			cout<<endl;
		}else{// we are done.
			killRanks();
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
	if(!m_ed->m_EXTENSION_currentRankIsStarted){
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
		message[0]=m_ed->m_EXTENSION_currentPosition;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_ed->m_EXTENSION_rank,RAY_MPI_TAG_WRITE_AMOS,getRank());
		m_outbox.push_back(aMessage);
		m_ed->m_EXTENSION_rank++;
		m_ed->m_EXTENSION_currentRankIsDone=false;
		m_ed->m_EXTENSION_currentRankIsStarted=true;
	}else if(m_ed->m_EXTENSION_currentRankIsDone){
		if(m_ed->m_EXTENSION_rank<getSize()){
			m_ed->m_EXTENSION_currentRankIsStarted=false;
		}else{
			killRanks();
			m_master_mode=RAY_MASTER_MODE_DO_NOTHING;
		}
	}
}

void Machine::call_RAY_SLAVE_MODE_AMOS(){
	if(!m_ed->m_EXTENSION_initiated){
		cout<<"Rank "<<m_rank<<" is appending positions to "<<m_parameters.getAmosFile()<<endl;
		m_amosFile=fopen(m_parameters.getAmosFile().c_str(),"a+");
		m_seedingData->m_SEEDING_i=0;
		m_mode_send_vertices_sequence_id_position=0;
		m_ed->m_EXTENSION_initiated=true;
		m_ed->m_EXTENSION_reads_requested=false;
		m_sequence_id=0;
	}
	/*
	* use m_allPaths and m_identifiers
	*
	* iterators: m_SEEDING_i: for the current contig
	*            m_mode_send_vertices_sequence_id_position: for the current position in the current contig.
	*/

	if(m_seedingData->m_SEEDING_i==(uint64_t)m_ed->m_EXTENSION_contigs.size()){// all contigs are processed
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
		message[0]=m_ed->m_EXTENSION_currentPosition;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_WRITE_AMOS_REPLY,getRank());
		m_outbox.push_back(aMessage);
		fclose(m_amosFile);
		m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
		cout<<"Rank "<<m_rank<<" appended "<<m_sequence_id<<" elements"<<endl;
	// iterate over the next one
	}else if(m_fusionData->m_FUSION_eliminated.count(m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i])>0){
		m_seedingData->m_SEEDING_i++;
		m_mode_send_vertices_sequence_id_position=0;
		m_ed->m_EXTENSION_reads_requested=false;
	}else if(m_mode_send_vertices_sequence_id_position==(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
		m_seedingData->m_SEEDING_i++;
		m_mode_send_vertices_sequence_id_position=0;
		m_ed->m_EXTENSION_reads_requested=false;
		
		fprintf(m_amosFile,"}\n");
	}else{
		if(!m_ed->m_EXTENSION_reads_requested){
			if(m_mode_send_vertices_sequence_id_position==0){
				string seq=convertToString(&(m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i]),m_wordSize);
				char*qlt=(char*)__Malloc(seq.length()+1,RAY_MASTER_MODE_AMOS);
				strcpy(qlt,seq.c_str());
				for(int i=0;i<(int)strlen(qlt);i++){
					qlt[i]='D';
				}
				m_sequence_id++;
				fprintf(m_amosFile,"{CTG\niid:%u\neid:contig-%lu\ncom:\nSoftware: Ray, MPI rank: %i\n.\nseq:\n%s\n.\nqlt:\n%s\n.\n",
					m_ed->m_EXTENSION_currentPosition+1,
					m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i],
					m_rank,
					seq.c_str(),
					qlt
					);

				m_ed->m_EXTENSION_currentPosition++;
				__Free(qlt,RAY_MALLOC_TYPE_AMOS);
			}

			m_ed->m_EXTENSION_reads_requested=true;
			m_ed->m_EXTENSION_reads_received=false;
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			uint64_t vertex=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_mode_send_vertices_sequence_id_position];
			message[0]=vertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(vertex,getSize(),m_wordSize),RAY_MPI_TAG_REQUEST_READS,getRank());
			m_outbox.push_back(aMessage);

			// iterator on reads
			m_fusionData->m_FUSION_path_id=0;
			m_ed->m_EXTENSION_readLength_requested=false;
		}else if(m_ed->m_EXTENSION_reads_received){
			if(m_fusionData->m_FUSION_path_id<(int)m_ed->m_EXTENSION_receivedReads.size()){
				int readRank=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getRank();
				char strand=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getStrand();
				int idOnRank=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getReadIndex();
				if(!m_ed->m_EXTENSION_readLength_requested){
					m_ed->m_EXTENSION_readLength_requested=true;
					m_ed->m_EXTENSION_readLength_received=false;
					uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
					message[0]=idOnRank;
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,readRank,RAY_MPI_TAG_ASK_READ_LENGTH,getRank());
					m_outbox.push_back(aMessage);
				}else if(m_ed->m_EXTENSION_readLength_received){
					int readLength=m_ed->m_EXTENSION_receivedLength;
					int globalIdentifier=m_parameters.getGlobalIdFromRankAndLocalId(readRank,idOnRank);
					int start=0;
					int theEnd=readLength-1;
					int offset=m_mode_send_vertices_sequence_id_position;
					if(strand=='R'){
						int t=start;
						start=theEnd;
						theEnd=t;
						offset++;
					}
					fprintf(m_amosFile,"{TLE\nsrc:%i\noff:%i\nclr:%i,%i\n}\n",globalIdentifier+1,offset,
						start,theEnd);
		
					// increment to get the next read.
					m_fusionData->m_FUSION_path_id++;
					m_ed->m_EXTENSION_readLength_requested=false;
				}
			}else{
				// continue.
				m_mode_send_vertices_sequence_id_position++;
				m_ed->m_EXTENSION_reads_requested=false;
			}
		}

	}
}

void Machine::call_RAY_SLAVE_MODE_EXTENSION(){
	int maxCoverage=m_parameters.getMaxCoverage();
	m_seedExtender.extendSeeds(&(m_seedingData->m_SEEDING_seeds),m_ed,getRank(),&m_outbox,&(m_seedingData->m_SEEDING_currentVertex),
	m_fusionData,&m_outboxAllocator,&(m_seedingData->m_SEEDING_edgesRequested),&(m_seedingData->m_SEEDING_outgoingEdgeIndex),
	&m_last_value,&(m_seedingData->m_SEEDING_vertexCoverageRequested),m_wordSize,&m_colorSpaceMode,getSize(),&(m_seedingData->m_SEEDING_vertexCoverageReceived),
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
	m_master_methods[RAY_MASTER_MODE_LOAD_CONFIG]=&Machine::call_RAY_MASTER_MODE_LOAD_CONFIG;
	m_master_methods[RAY_MASTER_MODE_LOAD_SEQUENCES]=&Machine::call_RAY_MASTER_MODE_LOAD_SEQUENCES;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION]=&Machine::call_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION;
	m_master_methods[RAY_MASTER_MODE_SEND_COVERAGE_VALUES]=&Machine::call_RAY_MASTER_MODE_SEND_COVERAGE_VALUES;
	m_master_methods[RAY_MASTER_MODE_DO_NOTHING]=&Machine::call_RAY_MASTER_MODE_DO_NOTHING;
	m_master_methods[RAY_MASTER_MODE_UPDATE_DISTANCES]=&Machine::call_RAY_MASTER_MODE_UPDATE_DISTANCES;
	m_master_methods[RAY_MASTER_MODE_ASK_EXTENSIONS]=&Machine::call_RAY_MASTER_MODE_ASK_EXTENSIONS;
	m_master_methods[RAY_MASTER_MODE_AMOS]=&Machine::call_RAY_MASTER_MODE_AMOS;
	m_master_methods[RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS]=&Machine::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_INDEXING]=&Machine::call_RAY_MASTER_MODE_TRIGGER_INDEXING;
	m_master_methods[RAY_MASTER_MODE_INDEX_SEQUENCES]=&Machine::call_RAY_MASTER_MODE_INDEX_SEQUENCES;
	m_master_methods[RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS]=&Machine::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS;
	m_master_methods[RAY_MASTER_MODE_PREPARE_SEEDING]=&Machine::call_RAY_MASTER_MODE_PREPARE_SEEDING;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_SEEDING]=&Machine::call_RAY_MASTER_MODE_TRIGGER_SEEDING;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_DETECTION]=&Machine::call_RAY_MASTER_MODE_TRIGGER_DETECTION;
	m_master_methods[RAY_MASTER_MODE_ASK_DISTANCES]=&Machine::call_RAY_MASTER_MODE_ASK_DISTANCES;
	m_master_methods[RAY_MASTER_MODE_START_UPDATING_DISTANCES]=&Machine::call_RAY_MASTER_MODE_START_UPDATING_DISTANCES;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_EXTENSIONS]=&Machine::call_RAY_MASTER_MODE_TRIGGER_EXTENSIONS;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_FUSIONS]=&Machine::call_RAY_MASTER_MODE_TRIGGER_FUSIONS;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS]=&Machine::call_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS;
	m_master_methods[RAY_MASTER_MODE_START_FUSION_CYCLE]=&Machine::call_RAY_MASTER_MODE_START_FUSION_CYCLE;
	m_master_methods[RAY_MASTER_MODE_ASK_BEGIN_REDUCTION]=&Machine::call_RAY_MASTER_MODE_ASK_BEGIN_REDUCTION;
	m_master_methods[RAY_MASTER_MODE_RESUME_VERTEX_DISTRIBUTION]=&Machine::call_RAY_MASTER_MODE_RESUME_VERTEX_DISTRIBUTION;
	m_master_methods[RAY_MASTER_MODE_START_REDUCTION]=&Machine::call_RAY_MASTER_MODE_START_REDUCTION;
}

void Machine::assignSlaveHandlers(){
	m_slave_methods[RAY_SLAVE_MODE_START_SEEDING]=&Machine::call_RAY_SLAVE_MODE_START_SEEDING;
	m_slave_methods[RAY_SLAVE_MODE_DO_NOTHING]=&Machine::call_RAY_SLAVE_MODE_DO_NOTHING;
	m_slave_methods[RAY_SLAVE_MODE_SEND_EXTENSION_DATA]=&Machine::call_RAY_SLAVE_MODE_SEND_EXTENSION_DATA;
	m_slave_methods[RAY_SLAVE_MODE_ASSEMBLE_WAVES]=&Machine::call_RAY_SLAVE_MODE_ASSEMBLE_WAVES;
	m_slave_methods[RAY_SLAVE_MODE_FUSION]=&Machine::call_RAY_SLAVE_MODE_FUSION;
	m_slave_methods[RAY_SLAVE_MODE_INDEX_SEQUENCES]=&Machine::call_RAY_SLAVE_MODE_INDEX_SEQUENCES;
	m_slave_methods[RAY_SLAVE_MODE_FINISH_FUSIONS]=&Machine::call_RAY_SLAVE_MODE_FINISH_FUSIONS;
	m_slave_methods[RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS]=&Machine::call_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS;
	m_slave_methods[RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION]=&Machine::call_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION;
	m_slave_methods[RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES]=&Machine::call_RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES;
	m_slave_methods[RAY_SLAVE_MODE_EXTRACT_VERTICES]=&Machine::call_RAY_SLAVE_MODE_EXTRACT_VERTICES;
	m_slave_methods[RAY_SLAVE_MODE_SEND_DISTRIBUTION]=&Machine::call_RAY_SLAVE_MODE_SEND_DISTRIBUTION;
	m_slave_methods[RAY_SLAVE_MODE_EXTENSION]=&Machine::call_RAY_SLAVE_MODE_EXTENSION;
	m_slave_methods[RAY_SLAVE_MODE_REDUCE_MEMORY_CONSUMPTION]=&Machine::call_RAY_SLAVE_MODE_REDUCE_MEMORY_CONSUMPTION;
	m_slave_methods[RAY_SLAVE_MODE_DELETE_VERTICES]=&Machine::call_RAY_SLAVE_MODE_DELETE_VERTICES;
	m_slave_methods[RAY_SLAVE_MODE_LOAD_SEQUENCES]=&Machine::call_RAY_SLAVE_MODE_LOAD_SEQUENCES;
	m_slave_methods[RAY_SLAVE_MODE_AMOS]=&Machine::call_RAY_SLAVE_MODE_AMOS;
}
