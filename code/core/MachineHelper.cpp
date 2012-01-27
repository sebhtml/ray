/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#include <core/MachineHelper.h>
#include <communication/mpi_tags.h>
#include <communication/Message.h>
#include <graph/CoverageDistribution.h>
#include <profiling/Profiler.h>
#include <heuristics/Chooser.h>

#include <map>
#include <sstream>
using namespace std;

void MachineHelper::call_RAY_MASTER_MODE_LOAD_CONFIG(){

	if(m_argc==2 && m_argv[1][0]!='-'){
		ifstream f(m_argv[1]);
		if(!f){
			cout<<"Rank "<<m_parameters->getRank()<<" invalid input file."<<endl;
			m_parameters->showUsage();
			(*m_aborted)=true;
			f.close();
			m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
			return;
		}
	}

	if(m_parameters->getError()){
		(*m_aborted)=true;
		m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}

	uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message[0]=m_parameters->getWordSize();
	message[1]=m_parameters->getColorSpaceMode();

	for(int i=0;i<m_parameters->getSize();i++){
		Message aMessage(message,2,i,RAY_MPI_TAG_SET_WORD_SIZE,m_parameters->getRank());
		m_outbox->push_back(aMessage);
	}

	m_switchMan->setMasterMode(RAY_MASTER_MODE_TEST_NETWORK);
}

void MachineHelper::constructor(int argc,char**argv,Parameters*parameters,
SwitchMan*switchMan,RingAllocator*outboxAllocator,
		StaticVector*outbox,bool*aborted,
	map<int,uint64_t>*coverageDistribution,
	int*numberOfMachinesDoneSendingCoverage,
	int*numberOfRanksWithCoverageData,bool*reductionOccured,
	ExtensionData*ed,FusionData*fusionData,
Profiler*profiler,NetworkTest*networkTest,SeedingData*seedingData,
TimePrinter*timePrinter,SeedExtender*seedExtender,Scaffolder*scaffolder,MessagesHandler*messagesHandler,
	StaticVector*inbox,
OpenAssemblerChooser*oa,	bool*isFinalFusion,	BubbleData*bubbleData,bool*alive,
 int*CLEAR_n,int*DISTRIBUTE_n,int*FINISH_n,Searcher*searcher,
	int*numberOfRanksDoneSeeding,	int*numberOfRanksDoneDetectingDistances,	int*numberOfRanksDoneSendingDistances,
	ArrayOfReads*myReads,	int*last_value,	VerticesExtractor*verticesExtractor,	EdgePurger*edgePurger,
int*mode_send_vertices_sequence_id,CoverageGatherer*coverageGatherer,GridTable*subgraph,SequencesIndexer*si,
VirtualCommunicator*virtualCommunicator,KmerAcademyBuilder*kmerAcademyBuilder,

	int*numberOfMachinesDoneSendingVertices,
	bool*initialisedAcademy,
	int*repeatedLength,
	int*readyToSeed,
	int*ranksDoneAttachingReads,
SequencesLoader*sl,time_t*lastTime,bool*writeKmerInitialised,Partitioner*partitioner
){
	m_sl=sl;
	m_lastTime=lastTime;
	m_writeKmerInitialised=writeKmerInitialised;
	m_partitioner=partitioner;

	m_reverseComplementVertex=false;
	m_loadSequenceStep=false;

	m_numberOfMachinesDoneSendingVertices=numberOfMachinesDoneSendingVertices;
	m_initialisedAcademy=initialisedAcademy;
	m_repeatedLength=repeatedLength;
	m_readyToSeed=readyToSeed;
	m_ranksDoneAttachingReads=ranksDoneAttachingReads;

	m_virtualCommunicator=virtualCommunicator;
	m_kmerAcademyBuilder=kmerAcademyBuilder;

	m_coverageInitialised=false;
	m_mode_send_vertices_sequence_id=mode_send_vertices_sequence_id;
	m_coverageGatherer=coverageGatherer;
	m_subgraph=subgraph;
	m_si=si;

	m_myReads=myReads;
	m_last_value=last_value;
	m_verticesExtractor=verticesExtractor;
	m_edgePurger=edgePurger;

	m_numberOfRanksDoneSeeding=numberOfRanksDoneSeeding;
	m_numberOfRanksDoneDetectingDistances=numberOfRanksDoneDetectingDistances;
	m_numberOfRanksDoneSendingDistances=numberOfRanksDoneSendingDistances;
	m_searcher=searcher;
	m_inbox=inbox;
	m_CLEAR_n=CLEAR_n;
	m_DISTRIBUTE_n=DISTRIBUTE_n;
	m_FINISH_n=FINISH_n;
	m_oa=oa;
	m_alive=alive;
	m_isFinalFusion=isFinalFusion;
	m_bubbleData=bubbleData;
	m_timePrinter=timePrinter;
	m_seedExtender=seedExtender;
	m_scaffolder=scaffolder;
	m_messagesHandler=messagesHandler;
	m_profiler=profiler;
	m_networkTest=networkTest;
	m_seedingData=seedingData;

	m_fusionData=fusionData;
	m_ed=ed;
	m_reductionOccured=reductionOccured;

	m_argc=argc;
	m_argv=argv;
	m_parameters=parameters;

	m_switchMan=switchMan;
	m_outboxAllocator=outboxAllocator;
	m_outbox=outbox;

	m_aborted=aborted;

	m_coverageDistribution=coverageDistribution;
	m_numberOfMachinesDoneSendingCoverage=numberOfMachinesDoneSendingCoverage;

	m_numberOfRanksWithCoverageData=numberOfRanksWithCoverageData;

	m_initialisedKiller=false;

}

void MachineHelper::call_RAY_MASTER_MODE_SEND_COVERAGE_VALUES (){
	if(m_parameters->hasCheckpoint("GenomeGraph")){
		cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint CoverageDistribution"<<endl;
		m_coverageDistribution->clear();
		ifstream f(m_parameters->getCheckpointFile("CoverageDistribution").c_str());
		int n=0;
		f.read((char*)&n,sizeof(int));
		int coverage=0;
		uint64_t count=0;
		for(int i=0;i<n;i++){
			f.read((char*)&coverage,sizeof(int));
			f.read((char*)&count,sizeof(uint64_t));
			(*m_coverageDistribution)[coverage]=count;
		}
		f.close();
	}

	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("CoverageDistribution")){
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint CoverageDistribution"<<endl;
		ofstream f(m_parameters->getCheckpointFile("CoverageDistribution").c_str());
		int theSize=m_coverageDistribution->size();
		f.write((char*)&theSize,sizeof(int));
		for(map<int,uint64_t>::iterator i=m_coverageDistribution->begin();i!=m_coverageDistribution->end();i++){
			int coverage=i->first;
			uint64_t count=i->second;
			f.write((char*)&coverage,sizeof(int));
			f.write((char*)&count,sizeof(uint64_t));
		}
		f.close();
	}

	if(m_coverageDistribution->size()==0){
		cout<<endl;
		cout<<"Rank 0: Assembler panic: no k-mers found in reads."<<endl;
		cout<<"Rank 0: Perhaps reads are shorter than the k-mer length (change -k)."<<endl;
		(*m_aborted)=true;
		m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}
	(*m_numberOfMachinesDoneSendingCoverage)=-1;
	string file=m_parameters->getCoverageDistributionFile();
	CoverageDistribution distribution(m_coverageDistribution,&file);

	m_parameters->setMinimumCoverage(distribution.getMinimumCoverage());
	m_parameters->setPeakCoverage(distribution.getPeakCoverage());
	m_parameters->setRepeatCoverage(distribution.getRepeatCoverage());

	printf("\n");
	fflush(stdout);

	cout<<endl;
	cout<<"Rank "<<getRank()<<": the minimum coverage is "<<m_parameters->getMinimumCoverage()<<endl;
	cout<<"Rank "<<getRank()<<": the peak coverage is "<<m_parameters->getPeakCoverage()<<endl;

	uint64_t numberOfVertices=0;
	uint64_t verticesWith1Coverage=0;
	int lowestCoverage=9999;
	
	uint64_t genomeKmers=0;

	for(map<int,uint64_t>::iterator i=m_coverageDistribution->begin();
		i!=m_coverageDistribution->end();i++){
		int coverageValue=i->first;
		uint64_t vertices=i->second;
		if(coverageValue<lowestCoverage){
			verticesWith1Coverage=vertices;
			lowestCoverage=coverageValue;
		}
		if(coverageValue>=m_parameters->getMinimumCoverage()){
			genomeKmers+=vertices;
		}
		numberOfVertices+=vertices;
	}
	double percentageSeenOnce=(0.0+verticesWith1Coverage)/numberOfVertices*100.00;

	ostringstream g;
	g<<m_parameters->getPrefix();
	g<<"CoverageDistributionAnalysis.txt";
	ofstream outputFile(g.str().c_str());
	outputFile<<"k-mer length:\t"<<m_parameters->getWordSize()<<endl;
	outputFile<<"Lowest coverage observed:\t"<<lowestCoverage<<endl;
	outputFile<<"MinimumCoverage:\t"<<m_parameters->getMinimumCoverage()<<endl;
	outputFile<<"PeakCoverage:\t"<<m_parameters->getPeakCoverage()<<endl;
	outputFile<<"RepeatCoverage:\t"<<m_parameters->getRepeatCoverage()<<endl;
	outputFile<<"Number of k-mers with at least MinimumCoverage:\t"<<genomeKmers<<" k-mers"<<endl;
	outputFile<<"Estimated genome length:\t"<<genomeKmers/2<<" nucleotides"<<endl;
	outputFile<<"Percentage of vertices with coverage "<<lowestCoverage<<":\t"<<percentageSeenOnce<<" %"<<endl;
	outputFile<<"DistributionFile: "<<file<<endl;

	outputFile.close();

	m_coverageDistribution->clear();

	// display a warning
	// for RNA-Seq and for méta-genomes, this is not important
	//
	if(m_parameters->getMinimumCoverage()> m_parameters->getPeakCoverage()
	|| m_parameters->getPeakCoverage()==m_parameters->getRepeatCoverage()
	|| m_parameters->getPeakCoverage()==1){
		cout<<"Warning: no peak observed in the k-mer coverage distribution."<<endl;
		cout<<"to deal with the sequencing error rate, try to lower the k-mer length (-k)"<<endl;
		cout<<"If you are using RNA-Seq or metagenomic data, then you can ignore this warning."<<endl;
	}

	// see these values to everyone.
	uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(3*sizeof(uint64_t));
	buffer[0]=m_parameters->getMinimumCoverage();
	buffer[1]=m_parameters->getPeakCoverage();
	buffer[2]=m_parameters->getRepeatCoverage();

	(*m_numberOfRanksWithCoverageData)=0;
	for(int i=0;i<m_parameters->getSize();i++){
		Message aMessage(buffer,3,i,RAY_MPI_TAG_SEND_COVERAGE_VALUES,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

int MachineHelper::getRank(){
	return m_parameters->getRank();
}

void MachineHelper::call_RAY_SLAVE_MODE_COUNT_FILE_ENTRIES(){
	// we only write the files, if any, when everyone is done with it
	// otherwise, the measured latency would be higher...
	m_networkTest->writeData();

	m_partitioner->call_RAY_SLAVE_MODE_COUNT_FILE_ENTRIES();
}

/** actually, call_RAY_MASTER_MODE_LOAD_SEQUENCES 
 * writes the AMOS file */
void MachineHelper::call_RAY_MASTER_MODE_LOAD_SEQUENCES(){

	m_timePrinter->printElapsedTime("Counting sequences to assemble");
	cout<<endl;

	/** this won't write anything if -amos was not provided */
	bool res=m_sl->writeSequencesToAMOSFile(getRank(),getSize(),
	m_outbox,
	m_outboxAllocator,
	&m_loadSequenceStep,
	m_bubbleData,
	m_lastTime,
	m_parameters,m_switchMan->getMasterModePointer(),m_switchMan->getSlaveModePointer()
);
	if(!res){
		(*m_aborted)=true;
		m_switchMan->setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
		m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}

	uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	uint32_t*messageInInts=(uint32_t*)message;
	messageInInts[0]=m_parameters->getNumberOfFiles();

	for(int i=0;i<(int)m_parameters->getNumberOfFiles();i++){
		messageInInts[1+i]=(uint64_t)m_parameters->getNumberOfSequences(i);
	}
	
	for(int i=0;i<getSize();i++){
		Message aMessage(message,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
		i,RAY_MPI_TAG_LOAD_SEQUENCES,getRank());
		m_outbox->push_back(aMessage);
	}

	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_SLAVE_MODE_LOAD_SEQUENCES(){
	// TODO: initialise this parameters in the constructor

	m_sl->call_RAY_SLAVE_MODE_LOAD_SEQUENCES(getRank(),getSize(),
	m_outbox,
	m_outboxAllocator,
	&m_loadSequenceStep,
	m_bubbleData,
	m_lastTime,
	m_parameters,m_switchMan->getMasterModePointer(),m_switchMan->getSlaveModePointer()
);
}

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION(){
	m_timePrinter->printElapsedTime("Sequence loading");
	cout<<endl;
	
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING(){
	(*m_numberOfMachinesDoneSendingVertices)=0;
	m_timePrinter->printElapsedTime("Coverage distribution analysis");
	cout<<endl;

	cout<<endl;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_BUILD_GRAPH,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_SLAVE_MODE_BUILD_KMER_ACADEMY(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	if(!(*m_initialisedAcademy)){
		m_kmerAcademyBuilder->constructor(m_parameters->getSize(),m_parameters,m_subgraph);
		(*m_mode_send_vertices_sequence_id)=0;
		(*m_initialisedAcademy)=true;

		m_si->constructor(m_parameters,m_outboxAllocator,m_inbox,m_outbox,m_virtualCommunicator);
	}
	
	// TODO: initialise these things in the constructor
	m_kmerAcademyBuilder->call_RAY_SLAVE_MODE_BUILD_KMER_ACADEMY(		m_mode_send_vertices_sequence_id,
			m_myReads,
			&m_reverseComplementVertex,
			getRank(),
			m_outbox,
			m_inbox,
			m_parameters->getWordSize(),
			getSize(),
			m_outboxAllocator,
			m_switchMan->getSlaveModePointer()
		);


	MACRO_COLLECT_PROFILING_INFORMATION();
}

void MachineHelper::call_RAY_SLAVE_MODE_EXTRACT_VERTICES(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	// TODO: initialise these in the constructor
	m_verticesExtractor->call_RAY_SLAVE_MODE_EXTRACT_VERTICES(		m_mode_send_vertices_sequence_id,
			m_myReads,
			&m_reverseComplementVertex,
			getRank(),
			m_outbox,
			m_parameters->getWordSize(),
			getSize(),
			m_outboxAllocator,
			m_switchMan->getSlaveModePointer()
		);

	MACRO_COLLECT_PROFILING_INFORMATION();
}

void MachineHelper::call_RAY_MASTER_MODE_PURGE_NULL_EDGES(){
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
	m_timePrinter->printElapsedTime("Graph construction");
	cout<<endl;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_PURGE_NULL_EDGES,getRank());
		m_outbox->push_back(aMessage);
	}
}

void MachineHelper::call_RAY_SLAVE_MODE_PURGE_NULL_EDGES(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	m_edgePurger->call_RAY_SLAVE_MODE_PURGE_NULL_EDGES();

	MACRO_COLLECT_PROFILING_INFORMATION();
}

void MachineHelper::call_RAY_MASTER_MODE_WRITE_KMERS(){
	if(!(*m_writeKmerInitialised)){
		(*m_writeKmerInitialised)=true;
		m_coverageRank=0;
		m_numberOfRanksDone=0;
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()==RAY_MPI_TAG_WRITE_KMERS_REPLY){
		uint64_t*buffer=(uint64_t*)m_inbox->at(0)->getBuffer();
		int bufferPosition=0;
		for(int i=0;i<=4;i++){
			for(int j=0;j<=4;j++){
				m_edgeDistribution[i][j]+=buffer[bufferPosition++];
			}
		}
		m_numberOfRanksDone++;
	}else if(m_numberOfRanksDone==m_parameters->getSize()){
		if(m_parameters->writeKmers()){
			cout<<endl;
			cout<<"Rank "<<getRank()<<" wrote "<<m_parameters->getPrefix()<<"kmers.txt"<<endl;
		}

		m_switchMan->closeMasterMode();

		if(m_parameters->hasCheckpoint("GenomeGraph"))
			return;

		ostringstream edgeFile;
		edgeFile<<m_parameters->getPrefix()<<"degreeDistribution.txt";
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
		m_outbox->push_back(aMessage);
		m_coverageRank++;
	}
}

void MachineHelper::call_RAY_SLAVE_MODE_WRITE_KMERS(){
	if(m_parameters->writeKmers())
		m_coverageGatherer->writeKmers();
	
	/* send edge distribution */
	GridTableIterator iterator;
	iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	map<int,map<int,uint64_t> > distribution;
	while(iterator.hasNext()){
		Vertex*node=iterator.next();
		Kmer key=*(iterator.getKey());
		int parents=node->getIngoingEdges(&key,m_parameters->getWordSize()).size();
		int children=node->getOutgoingEdges(&key,m_parameters->getWordSize()).size();
		distribution[parents][children]++;
	}

	uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int outputPosition=0;
	for(int i=0;i<=4;i++){
		for(int j=0;j<=4;j++){
			buffer[outputPosition++]=distribution[i][j];
		}
	}

	Message aMessage(buffer,outputPosition,MASTER_RANK,RAY_MPI_TAG_WRITE_KMERS_REPLY,getRank());
	m_outbox->push_back(aMessage);
	m_switchMan->setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_INDEXING(){
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
	
	m_timePrinter->printElapsedTime("Null edge purging");
	cout<<endl;

	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_START_INDEXING_SEQUENCES,getRank());
		m_outbox->push_back(aMessage);
	}
}

void MachineHelper::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS(){
	cout<<endl;
	(*m_numberOfMachinesDoneSendingVertices)=-1;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0, i, RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS(){

	if(!m_coverageInitialised){
		m_timePrinter->printElapsedTime("K-mer counting");
		cout<<endl;
		m_coverageInitialised=true;
		m_coverageRank=0;
	}

	for(m_coverageRank=0;m_coverageRank<m_parameters->getSize();m_coverageRank++){
		Message aMessage(NULL,0,m_coverageRank,
			RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
		m_outbox->push_back(aMessage);
	}

	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_MASTER_MODE_PREPARE_SEEDING(){
	(*m_ranksDoneAttachingReads)=-1;
	(*m_readyToSeed)=getSize();
	
	m_switchMan->closeMasterMode();
}

void MachineHelper::call_RAY_SLAVE_MODE_ASSEMBLE_WAVES(){
	// take each seed, and extend it in both direction using previously obtained information.
	if(m_seedingData->m_SEEDING_i==(uint64_t)m_seedingData->m_SEEDING_seeds.size()){
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE,getRank());
		m_outbox->push_back(aMessage);
	}else{
	}
}

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_SEEDING(){
	m_timePrinter->printElapsedTime("Selection of optimal read markers");
	cout<<endl;
	(*m_readyToSeed)=-1;
	(*m_numberOfRanksDoneSeeding)=0;

	// tell everyone to seed now.
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_START_SEEDING,getRank());
		m_outbox->push_back(aMessage);
	}

	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_DETECTION(){
	m_timePrinter->printElapsedTime("Detection of assembly seeds");
	cout<<endl;
	(*m_numberOfRanksDoneSeeding)=-1;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION,getRank());
		m_outbox->push_back(aMessage);
	}
	(*m_numberOfRanksDoneDetectingDistances)=0;
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_MASTER_MODE_ASK_DISTANCES(){
	(*m_numberOfRanksDoneDetectingDistances)=-1;
	(*m_numberOfRanksDoneSendingDistances)=0;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_MASTER_MODE_START_UPDATING_DISTANCES(){
	(*m_numberOfRanksDoneSendingDistances)=-1;
	m_parameters->computeAverageDistances();
	m_switchMan->setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
	
	m_switchMan->closeMasterMode();
}

void MachineHelper::call_RAY_SLAVE_MODE_INDEX_SEQUENCES(){

	// TODO: initialise these things in the constructor
	m_si->call_RAY_SLAVE_MODE_INDEX_SEQUENCES(m_myReads,m_outboxAllocator,m_outbox,m_switchMan->getSlaveModePointer(),m_parameters->getWordSize(),
	m_parameters->getSize(),m_parameters->getRank());
}

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_EXTENSIONS(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_ASK_EXTENSION,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_SLAVE_MODE_SEND_EXTENSION_DATA(){
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

	cout<<"Rank "<<m_parameters->getRank()<< " is appending its fusions"<<endl;
	string output=m_parameters->getOutputFile();
	ofstream fp;
	if(m_parameters->getRank()== MASTER_RANK){
		fp.open(output.c_str());
	}else{
		fp.open(output.c_str(),ios_base::out|ios_base::app);
	}
	int total=0;

	m_scaffolder->setContigPaths(&(m_ed->m_EXTENSION_identifiers),&(m_ed->m_EXTENSION_contigs));
	m_searcher->setContigs(&(m_ed->m_EXTENSION_contigs),&(m_ed->m_EXTENSION_identifiers));

	for(int i=0;i<(int)m_ed->m_EXTENSION_contigs.size();i++){
		uint64_t uniqueId=m_ed->m_EXTENSION_identifiers[i];
		if(m_fusionData->m_FUSION_eliminated.count(uniqueId)>0){
			continue;
		}
		total++;
		string contig=convertToString(&(m_ed->m_EXTENSION_contigs[i]),m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
		
		string withLineBreaks=addLineBreaks(contig,m_parameters->getColumns());
		fp<<">contig-"<<uniqueId<<" "<<contig.length()<<" nucleotides"<<endl<<withLineBreaks;

	}
	cout<<"Rank "<<m_parameters->getRank()<<" appended "<<total<<" elements"<<endl;
	fp.close();

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(getRank());
	}

	/** possibly write the checkpoint */
	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("ContigPaths")){
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint ContigPaths"<<endl;
		ofstream f(m_parameters->getCheckpointFile("ContigPaths").c_str());
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

	m_switchMan->setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
	Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_EXTENSION_DATA_END,getRank());
	m_outbox->push_back(aMessage);
}

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_FUSIONS(){
	m_timePrinter->printElapsedTime("Bidirectional extension of seeds");
	cout<<endl;
	
	m_cycleNumber=0;

	m_switchMan->closeMasterMode();
}

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS(){

	(*m_reductionOccured)=true;
	m_cycleStarted=false;
	m_mustStop=false;
	
	m_switchMan->closeMasterMode();
}

void MachineHelper::call_RAY_MASTER_MODE_START_FUSION_CYCLE(){
	/** this master method may require the whole outbox... */
	if(m_outbox->size()!=0)
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

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		(*m_reductionOccured)=false;
		m_cycleStarted=true;
		(*m_isFinalFusion)=false;
		for(int i=0;i<getSize();i++){
			Message aMessage(buffer,count,i,RAY_MPI_TAG_CLEAR_DIRECTIONS,getRank());
			m_outbox->push_back(aMessage);
		}
		m_currentCycleStep=1;
		(*m_CLEAR_n)=0;

		cout<<"Rank 0: starting clear step. cycleNumber= "<<m_cycleNumber<<endl;

		/* change the regulators if this is the first cycle. */
		if(m_cycleNumber == 0){
			(*m_isFinalFusion) = true;
			m_currentCycleStep = 4;
		}

	}else if((*m_CLEAR_n)==getSize() && !(*m_isFinalFusion) && m_currentCycleStep==1){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_currentCycleStep++;
		(*m_CLEAR_n)=-1;

		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,getRank());
			m_outbox->push_back(aMessage);
		}
		(*m_DISTRIBUTE_n)=0;
	}else if((*m_DISTRIBUTE_n) ==getSize() && !(*m_isFinalFusion) && m_currentCycleStep==2){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_currentCycleStep++;
		(*m_DISTRIBUTE_n)=-1;
		(*m_isFinalFusion)=true;
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_FINISH_FUSIONS,getRank());
			m_outbox->push_back(aMessage);
		}
		(*m_FINISH_n)=0;
	}else if((*m_FINISH_n) ==getSize() && (*m_isFinalFusion) && m_currentCycleStep==3){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_currentCycleStep++;
		int count=0;

		//cout<<"DEBUG (*m_reductionOccured)= "<<(*m_reductionOccured)<<endl;

		/* if paths were merged in RAY_MPI_TAG_FINISH_FUSIONS,
		then we want to continue these mergeing events */
		if((*m_reductionOccured) && m_cycleNumber < lastAllowedCycleNumber)
			m_mustStop = false;

		if(m_mustStop){
			count=1;
		}
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		for(int i=0;i<getSize();i++){
			Message aMessage(buffer,count,i,RAY_MPI_TAG_CLEAR_DIRECTIONS,getRank());
			m_outbox->push_back(aMessage);
		}

		(*m_FINISH_n)=-1;
		(*m_CLEAR_n)=0;
	}else if((*m_CLEAR_n) ==getSize() && (*m_isFinalFusion) && m_currentCycleStep==4){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		(*m_CLEAR_n)=-1;
		m_currentCycleStep++;

		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,getRank());
			m_outbox->push_back(aMessage);
		}
		(*m_DISTRIBUTE_n)=0;
	
		cout<<"Rank 0: starting distribution step"<<endl;
	}else if((*m_DISTRIBUTE_n)==getSize() && (*m_isFinalFusion) && m_currentCycleStep==5){
		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_currentCycleStep++;

		/* if we have the checkpoint, we want to jump to the final step now */

		/* the other condition is that we have to stop */
		if(m_mustStop || m_parameters->hasCheckpoint("ContigPaths")){
			cout<<"Rank "<<m_parameters->getRank()<<" cycleNumber= "<<m_cycleNumber<<endl;
			m_timePrinter->printElapsedTime("Merging of redundant paths");
			cout<<endl;

			m_switchMan->setMasterMode(RAY_MASTER_MODE_ASK_EXTENSIONS);

			m_ed->m_EXTENSION_currentRankIsSet=false;
			m_ed->m_EXTENSION_rank=-1;
			return;
		}

		cout<<"Rank 0 tells others to compute fusions."<<endl;
		m_fusionData->m_FUSION_numberOfRanksDone=0;
		(*m_DISTRIBUTE_n)=-1;
		for(int i=0;i<(int)getSize();i++){// start fusion.
			Message aMessage(NULL,0,i,RAY_MPI_TAG_START_FUSION,getRank());
			m_outbox->push_back(aMessage);
		}
		
	}else if(m_fusionData->m_FUSION_numberOfRanksDone==getSize() && (*m_isFinalFusion) && m_currentCycleStep==6){

		/** always force cycle number 2 */
		if(m_cycleNumber == 0)
			(*m_reductionOccured) = true;

		//cout<<"cycleStep= "<<m_currentCycleStep<<endl;
		m_fusionData->m_FUSION_numberOfRanksDone=-1;

		//cout<<"DEBUG (*m_reductionOccured)= "<<(*m_reductionOccured)<<endl;

		if(!(*m_reductionOccured) || m_cycleNumber == lastAllowedCycleNumber){ 
			m_mustStop=true;
		}

		// we continue now!
		m_cycleStarted=false;
		m_cycleNumber++;
	}
}

void MachineHelper::call_RAY_MASTER_MODE_ASK_EXTENSIONS(){
	// ask ranks to send their extensions.
	if(!m_ed->m_EXTENSION_currentRankIsSet){
		m_ed->m_EXTENSION_currentRankIsSet=true;
		m_ed->m_EXTENSION_currentRankIsStarted=false;
		m_ed->m_EXTENSION_rank++;
	}
	if(m_ed->m_EXTENSION_rank==getSize()){
		m_timePrinter->printElapsedTime("Generation of contigs");
		if(m_parameters->useAmos()){
			m_switchMan->setMasterMode(RAY_MASTER_MODE_AMOS);

			m_ed->m_EXTENSION_currentRankIsStarted=false;
			m_ed->m_EXTENSION_currentPosition=0;
			m_ed->m_EXTENSION_rank=0;
			m_seedingData->m_SEEDING_i=0;
			m_ed->m_EXTENSION_reads_requested=false;
			cout<<endl;
		}else{

			m_switchMan->closeMasterMode();

			m_scaffolder->m_numberOfRanksFinished=0;
		}
		
	}else if(!m_ed->m_EXTENSION_currentRankIsStarted){
		m_ed->m_EXTENSION_currentRankIsStarted=true;
		Message aMessage(NULL,0,m_ed->m_EXTENSION_rank,RAY_MPI_TAG_ASK_EXTENSION_DATA,getRank());
		m_outbox->push_back(aMessage);
		m_ed->m_EXTENSION_currentRankIsDone=false;
	}else if(m_ed->m_EXTENSION_currentRankIsDone){
		m_ed->m_EXTENSION_currentRankIsSet=false;
	}
}

void MachineHelper::call_RAY_MASTER_MODE_SCAFFOLDER(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_START_SCAFFOLDER,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_MASTER_MODE_KILL_RANKS(){
	m_switchMan->closeMasterMode();
}

/** make the message-passing interface rank die */
void MachineHelper::call_RAY_SLAVE_MODE_DIE(){

	/* write the network test data if not already written */
	m_networkTest->writeData();

	/** write message-passing interface file */
	ostringstream file;
	file<<m_parameters->getPrefix()<<"MessagePassingInterface.txt";

	string fileInString=file.str();
	m_messagesHandler->appendStatistics(fileInString.c_str());

	/** actually die */
	(*m_alive)=false;

	/** tell master that the rank died 
 * 	obviously, this message won't be recorded in the MessagePassingInterface file...
 * 	Because of that, MessagesHandler will do it for us.
 * 	*/
	Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY,m_parameters->getRank());
	m_outbox->push_back(aMessage);

	/** do nothing while dying 
 * 	the aging process takes a while -- 1024 cycles.
 * 	after that, it is death itself.
 * 	*/
	m_switchMan->setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
}

/**
 * here we kill everyone because the computation is terminated.
 */
void MachineHelper::call_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS(){
	if(!m_initialisedKiller){
		m_initialisedKiller=true;
		m_machineRank=m_parameters->getSize()-1;

		/** empty the file if it exists */
		ostringstream file;
		file<<m_parameters->getPrefix()<<"MessagePassingInterface.txt";
		
		FILE*fp=fopen(file.str().c_str(),"w+");
		if(fp==NULL){
			cout<<"Error: cannot create file "<<file<<endl;
		}
		fprintf(fp,"# Source\tDestination\tTag\tCount\n");
		fclose(fp);

		// activate the relay checker
		m_numberOfRanksDone=0;
		for(Rank i=0;i<m_parameters->getSize();i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER,getRank());
			m_outbox->push_back(aMessage);
		}

	// another rank activated its relay checker
	}else if(m_inbox->size()>0 && (*m_inbox)[0]->getTag()==RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY){
		m_numberOfRanksDone++;

	// do nothing and wait
	}else if(m_numberOfRanksDone!=m_parameters->getSize()){

	/** for the first to process (getSize()-1) -- the last -- we directly send it
 * a message.
 * For the other ones, we wait for the response of the previous.
 */
	}else if(m_machineRank==m_parameters->getSize()-1 || 
	(m_inbox->size()>0 && (*m_inbox)[0]->getTag()==RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY)){

		/**
 * 			Rank 0 is the last to kill
 */
		if(m_machineRank==0){
			m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
		}

		/** send a killer message */
		Message aMessage(NULL,0,m_machineRank,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON,getRank());
		m_outbox->push_back(aMessage);

		/** change the next to kill */
		m_machineRank--;
	}
}

void MachineHelper::call_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS(){

	// TODO: initialise these things in the constructor
	m_fusionData->call_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS(m_seedingData,m_ed,m_parameters->getRank(),
		m_outboxAllocator,m_outbox,getSize(),m_switchMan->getSlaveModePointer());
}

int MachineHelper::getSize(){
	return m_parameters->getSize();
}

void MachineHelper::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();

	core->beginPluginRegistration(plugin);

	core->setPluginName(plugin,"MachineHelper");

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_LOAD_CONFIG);
	m_adapter_RAY_MASTER_MODE_LOAD_CONFIG.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_LOAD_CONFIG, &m_adapter_RAY_MASTER_MODE_LOAD_CONFIG);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_SEND_COVERAGE_VALUES);
	m_adapter_RAY_MASTER_MODE_SEND_COVERAGE_VALUES.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_SEND_COVERAGE_VALUES, &m_adapter_RAY_MASTER_MODE_SEND_COVERAGE_VALUES);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_WRITE_KMERS);
	m_adapter_RAY_MASTER_MODE_WRITE_KMERS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_WRITE_KMERS, &m_adapter_RAY_MASTER_MODE_WRITE_KMERS);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_LOAD_SEQUENCES);
	m_adapter_RAY_MASTER_MODE_LOAD_SEQUENCES.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_LOAD_SEQUENCES, &m_adapter_RAY_MASTER_MODE_LOAD_SEQUENCES);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION);
	m_adapter_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION, &m_adapter_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING);
	m_adapter_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING, &m_adapter_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_PURGE_NULL_EDGES);
	m_adapter_RAY_MASTER_MODE_PURGE_NULL_EDGES.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_PURGE_NULL_EDGES, &m_adapter_RAY_MASTER_MODE_PURGE_NULL_EDGES);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TRIGGER_INDEXING);
	m_adapter_RAY_MASTER_MODE_TRIGGER_INDEXING.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_INDEXING, &m_adapter_RAY_MASTER_MODE_TRIGGER_INDEXING);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS);
	m_adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS, &m_adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS);
	m_adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS, &m_adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_PREPARE_SEEDING);
	m_adapter_RAY_MASTER_MODE_PREPARE_SEEDING.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_PREPARE_SEEDING, &m_adapter_RAY_MASTER_MODE_PREPARE_SEEDING);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TRIGGER_SEEDING);
	m_adapter_RAY_MASTER_MODE_TRIGGER_SEEDING.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_SEEDING, &m_adapter_RAY_MASTER_MODE_TRIGGER_SEEDING);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TRIGGER_DETECTION);
	m_adapter_RAY_MASTER_MODE_TRIGGER_DETECTION.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_DETECTION, &m_adapter_RAY_MASTER_MODE_TRIGGER_DETECTION);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_ASK_DISTANCES);
	m_adapter_RAY_MASTER_MODE_ASK_DISTANCES.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_ASK_DISTANCES, &m_adapter_RAY_MASTER_MODE_ASK_DISTANCES);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_START_UPDATING_DISTANCES);
	m_adapter_RAY_MASTER_MODE_START_UPDATING_DISTANCES.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_START_UPDATING_DISTANCES, &m_adapter_RAY_MASTER_MODE_START_UPDATING_DISTANCES);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TRIGGER_EXTENSIONS);
	m_adapter_RAY_MASTER_MODE_TRIGGER_EXTENSIONS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_EXTENSIONS, &m_adapter_RAY_MASTER_MODE_TRIGGER_EXTENSIONS);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TRIGGER_FUSIONS);
	m_adapter_RAY_MASTER_MODE_TRIGGER_FUSIONS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_FUSIONS, &m_adapter_RAY_MASTER_MODE_TRIGGER_FUSIONS);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS);
	m_adapter_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS, &m_adapter_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_START_FUSION_CYCLE);
	m_adapter_RAY_MASTER_MODE_START_FUSION_CYCLE.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_START_FUSION_CYCLE, &m_adapter_RAY_MASTER_MODE_START_FUSION_CYCLE);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_ASK_EXTENSIONS);
	m_adapter_RAY_MASTER_MODE_ASK_EXTENSIONS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_ASK_EXTENSIONS, &m_adapter_RAY_MASTER_MODE_ASK_EXTENSIONS);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_SCAFFOLDER);
	m_adapter_RAY_MASTER_MODE_SCAFFOLDER.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_SCAFFOLDER, &m_adapter_RAY_MASTER_MODE_SCAFFOLDER);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_KILL_RANKS);
	m_adapter_RAY_MASTER_MODE_KILL_RANKS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_KILL_RANKS, &m_adapter_RAY_MASTER_MODE_KILL_RANKS);

	core->allocateMasterModeHandle(plugin,RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
	m_adapter_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_KILL_ALL_MPI_RANKS, &m_adapter_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_LOAD_SEQUENCES);
	m_adapter_RAY_SLAVE_MODE_LOAD_SEQUENCES.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_LOAD_SEQUENCES, &m_adapter_RAY_SLAVE_MODE_LOAD_SEQUENCES);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_BUILD_KMER_ACADEMY);
	m_adapter_RAY_SLAVE_MODE_BUILD_KMER_ACADEMY.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_BUILD_KMER_ACADEMY, &m_adapter_RAY_SLAVE_MODE_BUILD_KMER_ACADEMY);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_EXTRACT_VERTICES);
	m_adapter_RAY_SLAVE_MODE_EXTRACT_VERTICES.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_EXTRACT_VERTICES, &m_adapter_RAY_SLAVE_MODE_EXTRACT_VERTICES);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_PURGE_NULL_EDGES);
	m_adapter_RAY_SLAVE_MODE_PURGE_NULL_EDGES.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_PURGE_NULL_EDGES, &m_adapter_RAY_SLAVE_MODE_PURGE_NULL_EDGES);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_WRITE_KMERS);
	m_adapter_RAY_SLAVE_MODE_WRITE_KMERS.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_WRITE_KMERS, &m_adapter_RAY_SLAVE_MODE_WRITE_KMERS);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_COUNT_FILE_ENTRIES);
	m_adapter_RAY_SLAVE_MODE_COUNT_FILE_ENTRIES.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_COUNT_FILE_ENTRIES, &m_adapter_RAY_SLAVE_MODE_COUNT_FILE_ENTRIES);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_ASSEMBLE_WAVES);
	m_adapter_RAY_SLAVE_MODE_ASSEMBLE_WAVES.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_ASSEMBLE_WAVES, &m_adapter_RAY_SLAVE_MODE_ASSEMBLE_WAVES);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_INDEX_SEQUENCES);
	m_adapter_RAY_SLAVE_MODE_INDEX_SEQUENCES.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_INDEX_SEQUENCES, &m_adapter_RAY_SLAVE_MODE_INDEX_SEQUENCES);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_SEND_EXTENSION_DATA);
	m_adapter_RAY_SLAVE_MODE_SEND_EXTENSION_DATA.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_SEND_EXTENSION_DATA, &m_adapter_RAY_SLAVE_MODE_SEND_EXTENSION_DATA);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_DIE);
	m_adapter_RAY_SLAVE_MODE_DIE.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_DIE, &m_adapter_RAY_SLAVE_MODE_DIE);

	core->allocateSlaveModeHandle(plugin,RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS);
	m_adapter_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS, &m_adapter_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS);

	core->endPluginRegistration(plugin);
}
