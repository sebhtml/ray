/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<crypto.h>
#include<SplayNode.h>
#include<mpi.h>
#include<Machine.h>
#include<VerticesExtractor.h>
#include<sstream>
#include<Message.h>
#include<time.h>
#include<RepeatedVertexWatchdog.h>
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
	m_dfsData=new DepthFirstSearchData();
	m_fusionData=new FusionData();
	m_seedingData=new SeedingData();

	m_ed=new ExtensionData();
	m_sd=new ScaffolderData();
	m_cd=new ChooserData();

	assignMasterHandlers();
	assignSlaveHandlers();
}

void Machine::start(){
	m_ready=true;
	int numberOfTrees=_FOREST_SIZE;

	m_seedExtender.constructor(&m_parameters);

	srand(m_lastTime);
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

	//

	int MAX_ALLOCATED_MESSAGES_IN_OUTBOX=5*getSize();
	m_outboxAllocator.constructor(MAX_ALLOCATED_MESSAGES_IN_OUTBOX,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	
	int MAX_ALLOCATED_MESSAGES_IN_INBOX=1;
	m_inboxAllocator.constructor(MAX_ALLOCATED_MESSAGES_IN_INBOX,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_persistentAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);
	m_directionsAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);
	m_treeAllocator.constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);

	//
	
	m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
	m_mode_AttachSequences=false;
	m_startEdgeDistribution=false;

	m_ranksDoneAttachingReads=0;

	m_reducer.constructor(getSize());

	int pid=getpid();
	printf("Rank %i is running as UNIX process %i on %s\n",getRank(),pid,serverName);
	fflush(stdout);

	MPI_Barrier(MPI_COMM_WORLD);

	if(isMaster()){
		cout<<endl<<"**************************************************"<<endl;
    		cout<<"This program comes with ABSOLUTELY NO WARRANTY."<<endl;
    		cout<<"This is free software, and you are welcome to redistribute it"<<endl;
    		cout<<"under certain conditions; see \"COPYING\" for details."<<endl;
		cout<<"**************************************************"<<endl;
		cout<<endl;
		cout<<"Ray Copyright (C) 2010  Sébastien Boisvert, Jacques Corbeil, François Laviolette"<<endl;
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
	m_sl.constructor(m_size);

	assert(getSize()<=MAX_NUMBER_OF_MPI_PROCESSES);
	

	int version;
	int subversion;
	MPI_Get_version(&version,&subversion);

	if(isMaster()){
		cout<<endl;
		cout<<"Bienvenue !"<<endl;
		cout<<endl;

		cout<<"Rank "<<MASTER_RANK<<": Ray "<<RAY_VERSION<<endl;
		cout<<endl;


		#ifdef MPICH2
                cout<<"Rank "<<MASTER_RANK<<": compiled with MPICH2 "<<MPICH2_VERSION<<endl;
		#endif

		#ifdef OMPI_MPI_H
                cout<<"Rank "<<MASTER_RANK<<": compiled with Open-MPI "<<OMPI_MAJOR_VERSION<<"."<<OMPI_MINOR_VERSION<<"."<<OMPI_RELEASE_VERSION<<endl;
		#endif

		cout<<"Rank "<<MASTER_RANK<<": MPI library implements the standard MPI "<<version<<"."<<subversion<<""<<endl;

		cout<<endl;

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

		cout<<endl;
		m_timePrinter.printElapsedTime("Beginning of computation");
		cout<<endl;
	}

	m_parameters.constructor(m_argc,m_argv,getRank());
	m_parameters.setSize(getSize());

	m_fusionData->constructor(getSize(),MAXIMUM_MESSAGE_SIZE_IN_BYTES,getRank(),&m_outbox,&m_outboxAllocator,m_parameters.getWordSize(),
	m_parameters.getColorSpaceMode(),
		m_ed,m_seedingData,&m_slave_mode);


	m_library.constructor(getRank(),&m_outbox,&m_outboxAllocator,&m_sequence_id,&m_sequence_idInFile,
		m_ed,&m_readsPositions,getSize(),&m_timePrinter,&m_slave_mode,&m_master_mode,
	&m_parameters,&m_fileId,m_seedingData);


	m_subgraph.constructor(numberOfTrees,&m_treeAllocator);
	
	m_seedingData->constructor(&m_seedExtender,getRank(),getSize(),&m_outbox,&m_outboxAllocator,&m_seedCoverage,&m_slave_mode,&m_parameters,&m_wordSize,&m_subgraph,
		&m_colorSpaceMode);

	if(isMaster()){

		cout<<"Rank "<<getRank()<<" welcomes you to the MPI_COMM_WORLD"<<endl<<endl;
	}

	m_alive=true;
	m_welcomeStep=true;
	m_loadSequenceStep=false;
	m_totalLetters=0;

	MPI_Barrier(MPI_COMM_WORLD);
	printf("Rank %i initiates its communication buffers\n",m_rank);

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
				&m_outbox,
	&m_sd->m_allIdentifiers,&m_oa,
	&m_numberOfRanksWithCoverageData,&m_seedExtender,
	&m_master_mode,&m_isFinalFusion,&m_si);

	m_messagesHandler.constructor(getRank(),getSize());
	if(m_argc==1 or ((string)m_argv[1])=="--help"){
		if(isMaster()){
			m_aborted=true;
			m_parameters.showUsage();
		}
	}else{
		if(isMaster()){
			m_master_mode=RAY_MASTER_MODE_LOAD_CONFIG;
			m_sl.constructor(getSize());
		}


		run();
	}



	if(isMaster() && !m_aborted){
		m_timePrinter.printElapsedTime("Collection of fusions");
		m_timePrinter.printDurations();

		cout<<endl;
		string file=m_parameters.getReceivedMessagesFile();
		const char*tmp=file.c_str();
		m_messagesHandler.writeStats(tmp);

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
		cout<<"Rank "<<getRank()<<" wrote "<<m_parameters.getReceivedMessagesFile()<<" (MPI communication matrix) "<<endl;
		cout<<endl;
		cout<<"Au revoir !"<<endl;
		cout<<endl;
	}



	MPI_Barrier(MPI_COMM_WORLD);
	m_messagesHandler.freeLeftovers();

	MPI_Finalize();
}

/*
 * this is the function that runs a lots
 *
 * it
 * 	1) receives messages
 * 	3) process message. The function that deals with a message is selected with the message's tag
 * 	4) process data, this depends on the master-mode and slave-mode states.
 * 	5) send messages
 */
void Machine::run(){
	while(isAlive()){
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
	assert(m_inbox.size()<=1);
	#endif

	for(int i=0;i<(int)m_inbox.size();i++){
		m_mp.processMessage((m_inbox[i]));
	}
	m_inbox.clear();
}

void Machine::sendMessages(){
	m_messagesHandler.sendMessages(&m_outbox,getRank());
}

void Machine::receiveMessages(){
	m_messagesHandler.receiveMessages(&m_inbox,&m_inboxAllocator,getRank());
}

void Machine::call_RAY_MASTER_MODE_LOAD_CONFIG(){
	if(m_argc==2){
		ifstream f(m_argv[1]);
		if(!f){
			cout<<"Rank "<<getRank()<<" invalid input file."<<endl;
			m_alive=false;
			m_aborted=true;
			f.close();
			killRanks();
			return;
		}
	}



	if(m_parameters.getError()){
		m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
		m_aborted=true;
		killRanks();
		return;
	}
	if(m_parameters.useAmos()){
		// empty the file.
		cout<<"Rank "<<getRank()<<" is preparing "<<m_parameters.getAmosFile()<<endl<<endl;
		m_bubbleData->m_amos=fopen(m_parameters.getAmosFile().c_str(),"w");
	}


	uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
	message[0]=m_parameters.getWordSize();
	uint64_t*message2=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
	message2[0]=m_parameters.getColorSpaceMode();
	for(int i=0;i<getSize();i++){
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_SET_WORD_SIZE,getRank());
		m_outbox.push_back(aMessage);
		
		Message aMessage2(message2,1,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_SET_COLOR_MODE,getRank());
		m_outbox.push_back(aMessage2);
	}
	m_master_mode=RAY_MASTER_MODE_LOAD_SEQUENCES;
}

void Machine::call_RAY_MASTER_MODE_LOAD_SEQUENCES(){
	bool res=m_sl.loadSequences(getRank(),getSize(),
	&m_outbox,
	&m_outboxAllocator,
	&m_loadSequenceStep,
	m_bubbleData,
	&m_lastTime,
	&m_parameters,&m_master_mode
);
	if(!res){
		m_aborted=true;
		killRanks();
		m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
		m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
	}
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION(){
	m_timePrinter.printElapsedTime("Distribution of sequence reads");
	cout<<endl;
	
	cout<<"Rank "<<getRank()<<" tells others to compute vertices"<<endl;

	for(int i=0;i<getSize();i++){
		Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_messageSentForVerticesDistribution=true;
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
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

	cout<<"Rank "<<getRank()<<" informs you that the minimum coverage is "<<m_minimumCoverage<<endl;
	cout<<"Rank "<<getRank()<<" informs you that the peak coverage is "<<m_peakCoverage<<endl;

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
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING(){
}

void Machine::call_RAY_SLAVE_MODE_DO_NOTHING(){
}

void Machine::call_RAY_SLAVE_MODE_EXTRACT_VERTICES(){
	m_verticesExtractor.process(		&m_mode_send_vertices_sequence_id,
			&m_myReads,
			&m_reverseComplementVertex,
			&m_mode_send_vertices_sequence_id_position,
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
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
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
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS(){
	m_numberOfMachinesReadyToSendDistribution=-1;
	m_timePrinter.printElapsedTime("Distribution of vertices & edges");
	cout<<endl;
	cout<<"Rank 0 computes the coverage distribution."<<endl;


	for(int i=0;i<getSize();i++){
		Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_PREPARE_SEEDING(){
	m_ranksDoneAttachingReads=-1;
	m_readyToSeed=getSize();
	m_master_mode=RAY_MASTER_MODE_TRIGGER_SEEDING;
}

void Machine::call_RAY_SLAVE_MODE_ASSEMBLE_WAVES(){
	// take each seed, and extend it in both direction using previously obtained information.
	if(m_seedingData->m_SEEDING_i==(int)m_seedingData->m_SEEDING_seeds.size()){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE,getRank());
		m_outbox.push_back(aMessage);
	}else{
	}
}

void Machine::call_RAY_SLAVE_MODE_PERFORM_CALIBRATION(){
	int rank=rand()%getSize();
	uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
	Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,rank,RAY_MPI_TAG_CALIBRATION_MESSAGE,getRank());
	m_outbox.push_back(aMessage);
}

void Machine::call_RAY_SLAVE_MODE_FINISH_FUSIONS(){
	m_fusionData->finishFusions();
}

void Machine::call_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS(){
	m_fusionData->distribute(m_seedingData,m_ed,m_rank,&m_outboxAllocator,&m_outbox,getSize(),&m_slave_mode);
}

void Machine::call_RAY_SLAVE_MODE_SEND_DISTRIBUTION(){
	if(m_distributionOfCoverage.size()==0){
		for(int i=0;i<m_subgraph.getNumberOfTrees();i++){
			SplayTreeIterator<uint64_t,Vertex> iterator(m_subgraph.getTree(i));
			while(iterator.hasNext()){
				int coverage=iterator.next()->getValue()->getCoverage();
				m_distributionOfCoverage[coverage]++;
			}
		}
	}

	int*data=(int*)m_outboxAllocator.allocate(sizeof(int)*2*m_parameters.getMaxCoverage());
	int j=0;
	data[j++]=m_distributionOfCoverage.size();
	for(map<int,uint64_t>::iterator i=m_distributionOfCoverage.begin();i!=m_distributionOfCoverage.end();i++){
		int coverage=i->first;
		uint64_t count=i->second;
		data[j++]=coverage;
		data[j++]=count;
	}
	Message aMessage(data,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t), MPI_UNSIGNED_LONG_LONG, MASTER_RANK, RAY_MPI_TAG_COVERAGE_DATA,getRank());
	m_outbox.push_back(aMessage);

	m_distributionOfCoverage.clear();
	m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_SEEDING(){
	m_timePrinter.printElapsedTime("Indexing of sequence reads");
	cout<<endl;
	cout<<"Rank 0 tells other ranks to calculate their seeds."<<endl;
	m_readyToSeed=-1;
	m_numberOfRanksDoneSeeding=0;
	// tell everyone to seed now.
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_SEEDING,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_SLAVE_MODE_START_SEEDING(){
	m_seedingData->computeSeeds();
}

void Machine::call_RAY_MASTER_MODE_TRIGGER_DETECTION(){
	m_timePrinter.printElapsedTime("Computation of seeds");
	cout<<endl;
	cout<<"Rank 0 asks others to approximate library sizes."<<endl;
	m_numberOfRanksDoneSeeding=-1;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_numberOfRanksDoneDetectingDistances=0;
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_ASK_DISTANCES(){
	m_numberOfRanksDoneDetectingDistances=-1;
	m_numberOfRanksDoneSendingDistances=0;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_START_UPDATING_DISTANCES(){
	m_numberOfRanksDoneSendingDistances=-1;
	m_parameters.computeAverageDistances();
	m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_UPDATE_DISTANCES;
	m_fileId=0;
	m_sequence_idInFile=0;
	m_sequence_id=0;
}

void Machine::call_RAY_MASTER_RAY_SLAVE_MODE_INDEX_SEQUENCES(){
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
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_SLAVE_MODE_SEND_EXTENSION_DATA(){
	if(!m_ready){
		return;
	}
	if(m_seedingData->m_SEEDING_i==(int)m_ed->m_EXTENSION_contigs.size()){
		m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_EXTENSION_DATA_END,getRank());
		m_outbox.push_back(aMessage);
	}else{
		if(m_fusionData->m_FUSION_eliminated.count(m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i])>0){ // skip merged paths.
			m_seedingData->m_SEEDING_i++;
			m_ed->m_EXTENSION_currentPosition=0;
		}else{
			if(m_ed->m_EXTENSION_currentPosition==0){
				uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(sizeof(uint64_t)*1);
				int theId=m_ed->m_EXTENSION_identifiers[m_seedingData->m_SEEDING_i];
				message[0]=theId;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_EXTENSION_START,getRank());
				m_outbox.push_back(aMessage);
			}
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

			int count=0;
			for(int i=0;i<(int)(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t));i++){
				if(m_ed->m_EXTENSION_currentPosition==(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
					break;
				}
				message[i+0]=m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
				m_ed->m_EXTENSION_currentPosition++;
				count++;
			}
			
			Message aMessage(message,count,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_EXTENSION_DATA,getRank());
			m_outbox.push_back(aMessage);
			m_ready=false;
			if(m_ed->m_EXTENSION_currentPosition==(int)m_ed->m_EXTENSION_contigs[m_seedingData->m_SEEDING_i].size()){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_EXTENSION_END,getRank());
				m_outbox.push_back(aMessage);
				m_seedingData->m_SEEDING_i++;
				m_ed->m_EXTENSION_currentPosition=0;
			}
		}
	}
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

void Machine::call_RAY_MASTER_RAY_SLAVE_MODE_UPDATE_DISTANCES(){
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
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
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
		//cout<<"Cycle "<<m_cycleNumber<<" sending 5) RAY_MPI_TAG_DISTRIBUTE_FUSIONS"<<endl;

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
			cout<<"Rank 0 is "<<"collecting fusions"<<endl;
			m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_ASK_EXTENSIONS;

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

void Machine::call_RAY_MASTER_RAY_SLAVE_MODE_ASK_EXTENSIONS(){
	#ifndef SHOW_PROGRESS
	time_t tmp=time(NULL);
	if(tmp>m_lastTime){
		m_lastTime=tmp;
		showProgress(m_lastTime);
	}
	#endif

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
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES,getRank());
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

		m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;

		int totalLength=0;
		
		#ifdef ASSERT
		assert(m_allPaths.size()==m_identifiers.size());
		#endif
		ofstream f(m_parameters.getOutputFile().c_str());
		for(int i=0;i<(int)m_allPaths.size();i++){
			string contig=convertToString(&(m_allPaths[i]),m_wordSize);
			#ifdef ASSERT
			assert(i<(int)m_identifiers.size());
			#endif
			int id=m_identifiers[i];
			#ifdef ASSERT
			int theRank=id%MAX_NUMBER_OF_MPI_PROCESSES;
			assert(theRank<getSize());
			#endif
			f<<">contig-"<<id<<" "<<contig.length()<<" nucleotides"<<endl<<addLineBreaks(contig);
			totalLength+=contig.length();
		}
		f.close();
		#ifdef SHOW_PROGRESS
		#else
		cout<<"\r"<<"              "<<endl<<"Writing "<<m_parameters.getOutputFile()<<endl;
		#endif
		cout<<endl<<"Rank 0: "<<m_allPaths.size()<<" contigs/"<<totalLength<<" nucleotides"<<endl;
		if(m_parameters.useAmos()){
			m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_AMOS;
			m_seedingData->m_SEEDING_i=0;
			m_mode_send_vertices_sequence_id_position=0;
			m_ed->m_EXTENSION_reads_requested=false;
			cout<<"Rank "<<getRank()<<" is completing "<<m_parameters.getAmosFile()<<endl;
		}else{// we are done.
			killRanks();
		}
		
	}else if(!m_ed->m_EXTENSION_currentRankIsStarted){
		m_ed->m_EXTENSION_currentRankIsStarted=true;
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<getRank()<<" asks "<<m_ed->m_EXTENSION_rank<<" its fusions"<<endl;
		#endif
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_ed->m_EXTENSION_rank,RAY_MPI_TAG_ASK_EXTENSION_DATA,getRank());
		m_outbox.push_back(aMessage);
		m_ed->m_EXTENSION_currentRankIsDone=false;
	}else if(m_ed->m_EXTENSION_currentRankIsDone){
		m_ed->m_EXTENSION_currentRankIsSet=false;
	}
}

void Machine::call_RAY_MASTER_RAY_SLAVE_MODE_AMOS(){
	// in development.
	/*
	* use m_allPaths and m_identifiers
	*
	* iterators: m_SEEDING_i: for the current contig
	*            m_mode_send_vertices_sequence_id_position: for the current position in the current contig.
	*/
	if(m_seedingData->m_SEEDING_i==(int)m_allPaths.size()){// all contigs are processed
		killRanks();
		m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
		fclose(m_bubbleData->m_amos);
	}else if(m_mode_send_vertices_sequence_id_position==(int)m_allPaths[m_seedingData->m_SEEDING_i].size()){// iterate over the next one
		m_seedingData->m_SEEDING_i++;
		m_mode_send_vertices_sequence_id_position=0;
		m_ed->m_EXTENSION_reads_requested=false;
		
		FILE*fp=m_bubbleData->m_amos;
		fprintf(fp,"}\n");
	}else{
		if(!m_ed->m_EXTENSION_reads_requested){
			if(m_mode_send_vertices_sequence_id_position==0){
				FILE*fp=m_bubbleData->m_amos;
				string seq=convertToString(&(m_allPaths[m_seedingData->m_SEEDING_i]),m_wordSize);
				char*qlt=(char*)__Malloc(seq.length()+1);
				strcpy(qlt,seq.c_str());
				for(int i=0;i<(int)strlen(qlt);i++)
					qlt[i]='D';
				fprintf(fp,"{CTG\niid:%i\neid:contig-%i\ncom:\nRay\n.\nseq:\n%s\n.\nqlt:\n%s\n.\n",
					m_seedingData->m_SEEDING_i+1,
					m_identifiers[m_seedingData->m_SEEDING_i],
					seq.c_str(),
					qlt
					);
				__Free(qlt);
			}

			m_ed->m_EXTENSION_reads_requested=true;
			m_ed->m_EXTENSION_reads_received=false;
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			message[0]=m_allPaths[m_seedingData->m_SEEDING_i][m_mode_send_vertices_sequence_id_position];
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0]),RAY_MPI_TAG_REQUEST_READS,getRank());
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
					int globalIdentifier=idOnRank*getSize()+readRank;
					FILE*fp=m_bubbleData->m_amos;
					int start=0;
					int theEnd=readLength-1;
					int offset=m_mode_send_vertices_sequence_id_position;
					if(strand=='R'){
						int t=start;
						start=theEnd;
						theEnd=t;
						offset++;
					}
					fprintf(fp,"{TLE\nsrc:%i\noff:%i\nclr:%i,%i\n}\n",globalIdentifier+1,offset,
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
	m_cd,m_bubbleData,m_dfsData,
m_minimumCoverage,&m_oa,&(m_seedingData->m_SEEDING_edgesReceived),&m_slave_mode);
}

void Machine::call_RAY_SLAVE_MODE_DELETE_VERTICES(){
	if(m_verticesExtractor.deleteVertices(m_reducer.getVerticesToRemove(),&m_subgraph,
&m_parameters,&m_outboxAllocator,&m_outbox
)){
		// flush


		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_DELETE_VERTICES_DONE,getRank());
		m_outbox.push_back(aMessage);
		m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
	}
}

void Machine::processData(){
	MachineMethod masterMethod=m_master_methods[m_master_mode];
	(this->*masterMethod)();
	MachineMethod slaveMethod=m_slave_methods[m_slave_mode];
	(this->*slaveMethod)();
}

void Machine::killRanks(){
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

void Machine::printStatus(){
	cout<<"********"<<endl;
	cout<<"Rank: "<<getRank()<<endl;
	cout<<"Reads: "<<m_myReads.size()<<endl;
	cout<<"Inbox: "<<m_inbox.size()<<endl;
	cout<<"Outbox: "<<m_outbox.size()<<endl;
}

Machine::~Machine(){
	// do nothing.
	delete m_dfsData;
	delete m_bubbleData;
	m_dfsData=NULL;
	m_bubbleData=NULL;
}

int Machine::vertexRank(uint64_t a){
	return uniform_hashing_function_1_64_64(a)%getSize();
}

void Machine::call_RAY_MASTER_MODE_ASK_BEGIN_REDUCTION(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_ASK_BEGIN_REDUCTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_MASTER_MODE_START_REDUCTION(){
	printf("Rank %i asks all ranks to reduce memory consumption\n",getRank());
	fflush(stdout);
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_START_REDUCTION,getRank());
		m_outbox.push_back(aMessage);
	}
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::call_RAY_SLAVE_MODE_REDUCE_MEMORY_CONSUMPTION(){
	if(m_reducer.reduce(&m_subgraph,&m_parameters,
&(m_seedingData->m_SEEDING_edgesRequested),&(m_seedingData->m_SEEDING_vertexCoverageRequested),&(m_seedingData->m_SEEDING_vertexCoverageReceived),
	&m_outboxAllocator,getSize(),getRank(),&m_outbox,
&(m_seedingData->m_SEEDING_receivedVertexCoverage),&(m_seedingData->m_SEEDING_receivedOutgoingEdges),
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
	m_master_mode=RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
}

void Machine::assignMasterHandlers(){
	m_master_methods[RAY_MASTER_MODE_LOAD_CONFIG]=&Machine::call_RAY_MASTER_MODE_LOAD_CONFIG;
	m_master_methods[RAY_MASTER_MODE_LOAD_SEQUENCES]=&Machine::call_RAY_MASTER_MODE_LOAD_SEQUENCES;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION]=&Machine::call_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION;
	m_master_methods[RAY_MASTER_MODE_SEND_COVERAGE_VALUES]=&Machine::call_RAY_MASTER_MODE_SEND_COVERAGE_VALUES;
	m_master_methods[RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING]=&Machine::call_RAY_MASTER_RAY_SLAVE_MODE_DO_NOTHING;
	m_master_methods[RAY_MASTER_RAY_SLAVE_MODE_UPDATE_DISTANCES]=&Machine::call_RAY_MASTER_RAY_SLAVE_MODE_UPDATE_DISTANCES;
	m_master_methods[RAY_MASTER_RAY_SLAVE_MODE_ASK_EXTENSIONS]=&Machine::call_RAY_MASTER_RAY_SLAVE_MODE_ASK_EXTENSIONS;
	m_master_methods[RAY_MASTER_RAY_SLAVE_MODE_AMOS]=&Machine::call_RAY_MASTER_RAY_SLAVE_MODE_AMOS;
	m_master_methods[RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS]=&Machine::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS;
	m_master_methods[RAY_MASTER_MODE_TRIGGER_INDEXING]=&Machine::call_RAY_MASTER_MODE_TRIGGER_INDEXING;
	m_master_methods[RAY_MASTER_RAY_SLAVE_MODE_INDEX_SEQUENCES]=&Machine::call_RAY_MASTER_RAY_SLAVE_MODE_INDEX_SEQUENCES;
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
	m_slave_methods[RAY_SLAVE_MODE_PERFORM_CALIBRATION]=&Machine::call_RAY_SLAVE_MODE_PERFORM_CALIBRATION;
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
}
