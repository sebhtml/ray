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

#include <plugin_MachineHelper/MachineHelper.h>
#include <communication/mpi_tags.h>
#include <communication/Message.h>
#include <plugin_CoverageGatherer/CoverageDistribution.h>
#include <profiling/Profiler.h>
#include <plugin_SeedExtender/Chooser.h>

#include <map>
#include <sstream>

__CreatePlugin(MachineHelper);

 /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_LOAD_CONFIG); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_SEND_COVERAGE_VALUES); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_WRITE_KMERS); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_LOAD_SEQUENCES); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_PURGE_NULL_EDGES); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_INDEXING); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_PREPARE_SEEDING); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_SEEDING); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_DETECTION); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_ASK_DISTANCES); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_START_UPDATING_DISTANCES); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_EXTENSIONS); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_FUSIONS); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_START_FUSION_CYCLE); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_ASK_EXTENSIONS); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_SCAFFOLDER); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_KILL_RANKS); /**/
__CreateMasterModeAdapter(MachineHelper,RAY_MASTER_MODE_KILL_ALL_MPI_RANKS); /**/
 /**/
__CreateSlaveModeAdapter(MachineHelper,RAY_SLAVE_MODE_WRITE_KMERS); /**/
__CreateSlaveModeAdapter(MachineHelper,RAY_SLAVE_MODE_ASSEMBLE_WAVES); /**/
__CreateSlaveModeAdapter(MachineHelper,RAY_SLAVE_MODE_SEND_EXTENSION_DATA); /**/
__CreateSlaveModeAdapter(MachineHelper,RAY_SLAVE_MODE_DIE); /**/
 /**/
 /**/

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

	MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(2*sizeof(MessageUnit));
	message[0]=m_parameters->getWordSize();
	message[1]=m_parameters->getColorSpaceMode();

	for(Rank i=0;i<m_parameters->getSize();i++){
		Message aMessage(message,2,i,RAY_MPI_TAG_SET_WORD_SIZE,m_parameters->getRank());
		m_outbox->push_back(aMessage);
	}

	m_switchMan->setMasterMode(RAY_MASTER_MODE_TEST_NETWORK);
}

void MachineHelper::constructor(int argc,char**argv,Parameters*parameters,
SwitchMan*switchMan,RingAllocator*outboxAllocator,
		StaticVector*outbox,bool*aborted,
	map<CoverageDepth,LargeCount>*coverageDistribution,
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

		LargeCount count=0;
		for(int i=0;i<n;i++){
			f.read((char*)&coverage,sizeof(int));
			f.read((char*)&count,sizeof(LargeCount));
			(*m_coverageDistribution)[coverage]=count;
		}
		f.close();
	}

	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("CoverageDistribution")){
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint CoverageDistribution"<<endl;
		ofstream f(m_parameters->getCheckpointFile("CoverageDistribution").c_str());
		int theSize=m_coverageDistribution->size();
		f.write((char*)&theSize,sizeof(int));

		for(map<CoverageDepth,LargeCount>::iterator i=m_coverageDistribution->begin();i!=m_coverageDistribution->end();i++){
			CoverageDepth coverage=i->first;
			LargeCount count=i->second;
			f.write((char*)&coverage,sizeof(CoverageDepth));
			f.write((char*)&count,sizeof(LargeCount));
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

	LargeCount numberOfVertices=0;
	LargeCount verticesWith1Coverage=0;
	CoverageDepth lowestCoverage=9999;
	
	LargeCount genomeKmers=0;

	for(map<CoverageDepth,LargeCount>::iterator i=m_coverageDistribution->begin();
		i!=m_coverageDistribution->end();i++){

		CoverageDepth coverageValue=i->first;
		LargeCount vertices=i->second;

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
	outputFile<<"Number of k-mers in the distributed de Bruijn graph: ";
	outputFile<<numberOfVertices<<endl;
	outputFile<<"Lowest coverage observed:\t"<<lowestCoverage<<endl;
	outputFile<<"MinimumCoverage:\t"<<m_parameters->getMinimumCoverage()<<endl;
	outputFile<<"PeakCoverage:\t"<<m_parameters->getPeakCoverage()<<endl;
	outputFile<<"RepeatCoverage:\t"<<m_parameters->getRepeatCoverage()<<endl;
	outputFile<<"Number of k-mers with at least MinimumCoverage:\t"<<genomeKmers<<" k-mers"<<endl;

	// don't report this as it is not accurate with the new algorithms
	//outputFile<<"Estimated genome length:\t"<<genomeKmers/2<<" nucleotides"<<endl;
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
	MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(3*sizeof(MessageUnit));
	buffer[0]=m_parameters->getMinimumCoverage();
	buffer[1]=m_parameters->getPeakCoverage();
	buffer[2]=m_parameters->getRepeatCoverage();

	(*m_numberOfRanksWithCoverageData)=0;

	for(Rank i=0;i<m_parameters->getSize();i++){
		Message aMessage(buffer,3,i,RAY_MPI_TAG_SEND_COVERAGE_VALUES,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

int MachineHelper::getRank(){
	return m_parameters->getRank();
}

/** actually, call_RAY_MASTER_MODE_LOAD_SEQUENCES 
 * writes the AMOS file */
void MachineHelper::call_RAY_MASTER_MODE_LOAD_SEQUENCES(){

	m_timePrinter->printElapsedTime("Counting sequences to assemble");
	cout<<endl;

	bool result=true;

	if(m_parameters->useAmos()){
/* This won't write anything if -amos was not provided */

		result=m_sl->writeSequencesToAMOSFile(getRank(),getSize(),
			m_outbox,
			m_outboxAllocator,
			&m_loadSequenceStep,
			m_bubbleData,
			m_lastTime,
			m_parameters,m_switchMan->getMasterModePointer(),m_switchMan->getSlaveModePointer());
	}

	if(!result){
		(*m_aborted)=true;
		m_switchMan->setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
		m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}

	MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	uint32_t*messageInInts=(uint32_t*)message;
	messageInInts[0]=m_parameters->getNumberOfFiles();

	for(int i=0;i<(int)m_parameters->getNumberOfFiles();i++){
		messageInInts[1+i]=(LargeCount)m_parameters->getNumberOfSequences(i);
	}
	
	for(Rank i=0;i<getSize();i++){
		Message aMessage(message,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),
			i,RAY_MPI_TAG_LOAD_SEQUENCES,getRank());
		m_outbox->push_back(aMessage);
	}

	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
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

void MachineHelper::call_RAY_MASTER_MODE_PURGE_NULL_EDGES(){
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
	m_timePrinter->printElapsedTime("Graph construction");
	cout<<endl;
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_PURGE_NULL_EDGES,getRank());
		m_outbox->push_back(aMessage);
	}
}

void MachineHelper::call_RAY_MASTER_MODE_WRITE_KMERS(){
	if(!(*m_writeKmerInitialised)){
		(*m_writeKmerInitialised)=true;
		m_coverageRank=0;
		m_numberOfRanksDone=0;
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()==RAY_MPI_TAG_WRITE_KMERS_REPLY){
		MessageUnit*buffer=(MessageUnit*)m_inbox->at(0)->getBuffer();
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

		if(m_parameters->writeKmers()){
			// do it in serial mode because we need to write a parallel file
			m_switchMan->sendEmptyMessage(m_outbox,getRank(),m_coverageRank,RAY_MPI_TAG_WRITE_KMERS);
			m_coverageRank ++ ;
		}else{
			// do it in parallel
			m_switchMan->sendToAll(m_outbox,getRank(),RAY_MPI_TAG_WRITE_KMERS);
			m_coverageRank+=m_parameters->getSize();
		}
	}
}

void MachineHelper::call_RAY_SLAVE_MODE_WRITE_KMERS(){
	if(m_parameters->writeKmers()){
		m_coverageGatherer->writeKmers();
	}
	
	/* send edge distribution */
	GridTableIterator iterator;
	iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	map<int,map<int,LargeCount> > distribution;
	while(iterator.hasNext()){
		Vertex*node=iterator.next();
		Kmer key=*(iterator.getKey());
		int parents=node->getIngoingEdges(&key,m_parameters->getWordSize()).size();
		int children=node->getOutgoingEdges(&key,m_parameters->getWordSize()).size();
		distribution[parents][children]++;
	}

	MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
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
	if(m_seedingData->m_SEEDING_i==(LargeCount)m_seedingData->m_SEEDING_seeds.size()){
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

void MachineHelper::call_RAY_MASTER_MODE_TRIGGER_EXTENSIONS(){
	for(int i=0;i<getSize();i++){
		Message aMessage(NULL,0,i,RAY_MPI_TAG_ASK_EXTENSION,getRank());
		m_outbox->push_back(aMessage);
	}
	m_switchMan->setMasterMode(RAY_MASTER_MODE_DO_NOTHING);
}

void MachineHelper::call_RAY_SLAVE_MODE_SEND_EXTENSION_DATA(){
	/* clear eliminated paths */
	vector<PathHandle> newNames;
	vector<vector<Kmer> > newPaths;

	for(int i=0;i<(int)m_ed->m_EXTENSION_contigs.size();i++){
		PathHandle uniqueId=m_ed->m_EXTENSION_identifiers[i];

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

	fp.open(output.c_str(),ios_base::out|ios_base::app);

	int total=0;

	m_scaffolder->setContigPaths(&(m_ed->m_EXTENSION_identifiers),&(m_ed->m_EXTENSION_contigs));
	m_searcher->setContigs(&(m_ed->m_EXTENSION_contigs),&(m_ed->m_EXTENSION_identifiers));

	ostringstream operationBuffer;

	for(int i=0;i<(int)m_ed->m_EXTENSION_contigs.size();i++){
		PathHandle uniqueId=m_ed->m_EXTENSION_identifiers[i];

		if(m_fusionData->m_FUSION_eliminated.count(uniqueId)>0){
			continue;
		}

		total++;
		string contig=convertToString(&(m_ed->m_EXTENSION_contigs[i]),m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
		
		string withLineBreaks=addLineBreaks(contig,m_parameters->getColumns());

		operationBuffer<<">contig-"<<uniqueId<<" "<<contig.length()<<" nucleotides"<<endl<<withLineBreaks;

		flushFileOperationBuffer(false,&operationBuffer,&fp,CONFIG_FILE_IO_BUFFER_SIZE);
	}

	cout<<"Rank "<<m_parameters->getRank()<<" appended "<<total<<" elements"<<endl;

	flushFileOperationBuffer(true,&operationBuffer,&fp,CONFIG_FILE_IO_BUFFER_SIZE);

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
			PathHandle name=m_ed->m_EXTENSION_identifiers[i];
			int vertices=m_ed->m_EXTENSION_contigs[i].size();
			f.write((char*)&name,sizeof(PathHandle));
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

	int lastAllowedCycleNumber=5 ; //=5;

	if(!m_cycleStarted){
		int count=0;

		if(m_mustStop){
			count=1;
		}

		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

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
		if((*m_reductionOccured) && m_cycleNumber < lastAllowedCycleNumber){
			m_mustStop = false;
		}

		if(m_mustStop){
			count=1;
		}
		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

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

		m_seedExtender->closePathFile();

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

int MachineHelper::getSize(){
	return m_parameters->getSize();
}

void MachineHelper::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"MachineHelper");
	core->setPluginDescription(plugin,"Legacy plugin for some master modes");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_LOAD_CONFIG=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_LOAD_CONFIG, __GetAdapter(MachineHelper,RAY_MASTER_MODE_LOAD_CONFIG));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_LOAD_CONFIG,"RAY_MASTER_MODE_LOAD_CONFIG");

	RAY_MASTER_MODE_SEND_COVERAGE_VALUES=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_SEND_COVERAGE_VALUES, __GetAdapter(MachineHelper,RAY_MASTER_MODE_SEND_COVERAGE_VALUES));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_SEND_COVERAGE_VALUES,"RAY_MASTER_MODE_SEND_COVERAGE_VALUES");

	RAY_MASTER_MODE_WRITE_KMERS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_WRITE_KMERS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_WRITE_KMERS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_WRITE_KMERS,"RAY_MASTER_MODE_WRITE_KMERS");

	RAY_MASTER_MODE_LOAD_SEQUENCES=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_LOAD_SEQUENCES, __GetAdapter(MachineHelper,RAY_MASTER_MODE_LOAD_SEQUENCES));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_LOAD_SEQUENCES,"RAY_MASTER_MODE_LOAD_SEQUENCES");

	RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION, __GetAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION,"RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION");

	RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING, __GetAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING,"RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING");

	RAY_MASTER_MODE_PURGE_NULL_EDGES=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_PURGE_NULL_EDGES, __GetAdapter(MachineHelper,RAY_MASTER_MODE_PURGE_NULL_EDGES));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_PURGE_NULL_EDGES,"RAY_MASTER_MODE_PURGE_NULL_EDGES");

	RAY_MASTER_MODE_TRIGGER_INDEXING=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_INDEXING, __GetAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_INDEXING));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TRIGGER_INDEXING,"RAY_MASTER_MODE_TRIGGER_INDEXING");

	RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS,"RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS");

	RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS,"RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS");

	RAY_MASTER_MODE_PREPARE_SEEDING=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_PREPARE_SEEDING, __GetAdapter(MachineHelper,RAY_MASTER_MODE_PREPARE_SEEDING));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_PREPARE_SEEDING,"RAY_MASTER_MODE_PREPARE_SEEDING");

	RAY_MASTER_MODE_TRIGGER_SEEDING=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_SEEDING, __GetAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_SEEDING));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TRIGGER_SEEDING,"RAY_MASTER_MODE_TRIGGER_SEEDING");

	RAY_MASTER_MODE_TRIGGER_DETECTION=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_DETECTION, __GetAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_DETECTION));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TRIGGER_DETECTION,"RAY_MASTER_MODE_TRIGGER_DETECTION");

	RAY_MASTER_MODE_ASK_DISTANCES=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_ASK_DISTANCES, __GetAdapter(MachineHelper,RAY_MASTER_MODE_ASK_DISTANCES));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_ASK_DISTANCES,"RAY_MASTER_MODE_ASK_DISTANCES");

	RAY_MASTER_MODE_START_UPDATING_DISTANCES=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_START_UPDATING_DISTANCES, __GetAdapter(MachineHelper,RAY_MASTER_MODE_START_UPDATING_DISTANCES));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_START_UPDATING_DISTANCES,"RAY_MASTER_MODE_START_UPDATING_DISTANCES");

	RAY_MASTER_MODE_TRIGGER_EXTENSIONS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_EXTENSIONS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_EXTENSIONS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TRIGGER_EXTENSIONS,"RAY_MASTER_MODE_TRIGGER_EXTENSIONS");

	RAY_MASTER_MODE_TRIGGER_FUSIONS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_FUSIONS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_FUSIONS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TRIGGER_FUSIONS,"RAY_MASTER_MODE_TRIGGER_FUSIONS");

	RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS,"RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS");

	RAY_MASTER_MODE_START_FUSION_CYCLE=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_START_FUSION_CYCLE, __GetAdapter(MachineHelper,RAY_MASTER_MODE_START_FUSION_CYCLE));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_START_FUSION_CYCLE,"RAY_MASTER_MODE_START_FUSION_CYCLE");

	RAY_MASTER_MODE_ASK_EXTENSIONS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_ASK_EXTENSIONS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_ASK_EXTENSIONS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_ASK_EXTENSIONS,"RAY_MASTER_MODE_ASK_EXTENSIONS");

	RAY_MASTER_MODE_SCAFFOLDER=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_SCAFFOLDER, __GetAdapter(MachineHelper,RAY_MASTER_MODE_SCAFFOLDER));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_SCAFFOLDER,"RAY_MASTER_MODE_SCAFFOLDER");

	RAY_MASTER_MODE_KILL_RANKS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_KILL_RANKS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_KILL_RANKS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_KILL_RANKS,"RAY_MASTER_MODE_KILL_RANKS");

	RAY_MASTER_MODE_KILL_ALL_MPI_RANKS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_KILL_ALL_MPI_RANKS, __GetAdapter(MachineHelper,RAY_MASTER_MODE_KILL_ALL_MPI_RANKS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_KILL_ALL_MPI_RANKS,"RAY_MASTER_MODE_KILL_ALL_MPI_RANKS");


	RAY_SLAVE_MODE_WRITE_KMERS=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_WRITE_KMERS, __GetAdapter(MachineHelper,RAY_SLAVE_MODE_WRITE_KMERS));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_WRITE_KMERS,"RAY_SLAVE_MODE_WRITE_KMERS");

	RAY_SLAVE_MODE_ASSEMBLE_WAVES=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_ASSEMBLE_WAVES, __GetAdapter(MachineHelper,RAY_SLAVE_MODE_ASSEMBLE_WAVES));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_ASSEMBLE_WAVES,"RAY_SLAVE_MODE_ASSEMBLE_WAVES");

	RAY_SLAVE_MODE_SEND_EXTENSION_DATA=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_SEND_EXTENSION_DATA, __GetAdapter(MachineHelper,RAY_SLAVE_MODE_SEND_EXTENSION_DATA));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_SEND_EXTENSION_DATA,"RAY_SLAVE_MODE_SEND_EXTENSION_DATA");

	RAY_SLAVE_MODE_DIE=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_DIE, __GetAdapter(MachineHelper,RAY_SLAVE_MODE_DIE));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_DIE,"RAY_SLAVE_MODE_DIE");

	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY");

	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON");

	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY");

}

void MachineHelper::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_ADD_COLORS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ADD_COLORS");
	RAY_SLAVE_MODE_AMOS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_AMOS");
	RAY_SLAVE_MODE_ASSEMBLE_WAVES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ASSEMBLE_WAVES");
	RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION");
	RAY_SLAVE_MODE_ADD_VERTICES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ADD_VERTICES");
	RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES");
	RAY_SLAVE_MODE_COUNT_FILE_ENTRIES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_COUNT_FILE_ENTRIES");
	RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS");
	RAY_SLAVE_MODE_DIE=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DIE");
	RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS");
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");
	RAY_SLAVE_MODE_ADD_EDGES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ADD_EDGES");
	RAY_SLAVE_MODE_FINISH_FUSIONS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_FINISH_FUSIONS");
	RAY_SLAVE_MODE_FUSION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_FUSION");
	RAY_SLAVE_MODE_INDEX_SEQUENCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_INDEX_SEQUENCES");
	RAY_SLAVE_MODE_LOAD_SEQUENCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_LOAD_SEQUENCES");
	RAY_SLAVE_MODE_SCAFFOLDER=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SCAFFOLDER");
	RAY_SLAVE_MODE_SEND_DISTRIBUTION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_DISTRIBUTION");
	RAY_SLAVE_MODE_SEND_EXTENSION_DATA=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_EXTENSION_DATA");
	RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES");
	RAY_SLAVE_MODE_SEND_SEED_LENGTHS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_SEED_LENGTHS");
	RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES");
	RAY_SLAVE_MODE_START_SEEDING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_START_SEEDING");
	RAY_SLAVE_MODE_TEST_NETWORK=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_TEST_NETWORK");
	RAY_SLAVE_MODE_WRITE_KMERS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_WRITE_KMERS");
	RAY_SLAVE_MODE_EXTENSION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_EXTENSION");

	RAY_MASTER_MODE_ADD_COLORS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ADD_COLORS");
	RAY_MASTER_MODE_AMOS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_AMOS");
	RAY_MASTER_MODE_ASK_DISTANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ASK_DISTANCES");
	RAY_MASTER_MODE_ASK_EXTENSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ASK_EXTENSIONS");
	RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES");
	RAY_MASTER_MODE_COUNT_FILE_ENTRIES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_COUNT_FILE_ENTRIES");
	RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS");
	RAY_MASTER_MODE_DO_NOTHING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_DO_NOTHING");
	RAY_MASTER_MODE_KILL_ALL_MPI_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_ALL_MPI_RANKS");
	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");
	RAY_MASTER_MODE_LOAD_CONFIG=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_LOAD_CONFIG");
	RAY_MASTER_MODE_LOAD_SEQUENCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_LOAD_SEQUENCES");
	RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS");
	RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS");
	RAY_MASTER_MODE_PREPARE_SEEDING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PREPARE_SEEDING");
	RAY_MASTER_MODE_PURGE_NULL_EDGES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PURGE_NULL_EDGES");
	RAY_MASTER_MODE_SCAFFOLDER=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_SCAFFOLDER");
	RAY_MASTER_MODE_SEND_COVERAGE_VALUES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_SEND_COVERAGE_VALUES");
	RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES");
	RAY_MASTER_MODE_START_FUSION_CYCLE=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_START_FUSION_CYCLE");
	RAY_MASTER_MODE_START_UPDATING_DISTANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_START_UPDATING_DISTANCES");
	RAY_MASTER_MODE_TEST_NETWORK=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TEST_NETWORK");
	RAY_MASTER_MODE_TRIGGER_DETECTION=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_DETECTION");
	RAY_MASTER_MODE_TRIGGER_EXTENSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_EXTENSIONS");
	RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS");
	RAY_MASTER_MODE_TRIGGER_FUSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_FUSIONS");
	RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING");
	RAY_MASTER_MODE_TRIGGER_INDEXING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_INDEXING");
	RAY_MASTER_MODE_TRIGGER_SEEDING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_SEEDING");
	RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION");
	RAY_MASTER_MODE_UPDATE_DISTANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_UPDATE_DISTANCES");
	RAY_MASTER_MODE_WRITE_KMERS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_WRITE_KMERS");
	RAY_MASTER_MODE_WRITE_SCAFFOLDS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_WRITE_SCAFFOLDS");

	RAY_MPI_TAG_FINISH_FUSIONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_FINISH_FUSIONS");
	RAY_MPI_TAG_GET_CONTIG_CHUNK=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_CONTIG_CHUNK");
	RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY");
	RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION");
	RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY");
	RAY_MPI_TAG_GET_COVERAGE_AND_PATHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_PATHS");
	RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY");
	RAY_MPI_TAG_GET_PATH_LENGTH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_LENGTH");
	RAY_MPI_TAG_GET_PATH_LENGTH_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_LENGTH_REPLY");
	RAY_MPI_TAG_GET_PATH_VERTEX=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_VERTEX");
	RAY_MPI_TAG_GET_PATH_VERTEX_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_VERTEX_REPLY");
	RAY_MPI_TAG_GET_READ_MARKERS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MARKERS");
	RAY_MPI_TAG_GET_READ_MARKERS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MARKERS_REPLY");
	RAY_MPI_TAG_GET_READ_MATE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MATE");
	RAY_MPI_TAG_GET_READ_MATE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MATE_REPLY");
	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT");
	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY");
	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON");
	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY");
	RAY_MPI_TAG_HAS_PAIRED_READ=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_HAS_PAIRED_READ");
	RAY_MPI_TAG_HAS_PAIRED_READ_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_HAS_PAIRED_READ_REPLY");
	RAY_MPI_TAG_LOAD_SEQUENCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_LOAD_SEQUENCES");
	RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION");
	RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION");
	RAY_MPI_TAG_PURGE_NULL_EDGES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PURGE_NULL_EDGES");
	RAY_MPI_TAG_REQUEST_READ_SEQUENCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_READ_SEQUENCE");
	RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY");
	RAY_MPI_TAG_REQUEST_SEED_LENGTHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_SEED_LENGTHS");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES");
	RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_READS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_READS");
	RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY");
	RAY_MPI_TAG_SCAFFOLDING_LINKS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SCAFFOLDING_LINKS");
	RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY");
	RAY_MPI_TAG_SEND_COVERAGE_VALUES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEND_COVERAGE_VALUES");
	RAY_MPI_TAG_SEQUENCE_BIOLOGICAL_ABUNDANCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEQUENCE_BIOLOGICAL_ABUNDANCES");
	RAY_MPI_TAG_SET_WORD_SIZE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SET_WORD_SIZE");
	RAY_MPI_TAG_START_FUSION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_FUSION");
	RAY_MPI_TAG_START_INDEXING_SEQUENCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_INDEXING_SEQUENCES");
	RAY_MPI_TAG_START_SCAFFOLDER=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_SCAFFOLDER");
	RAY_MPI_TAG_START_SEEDING=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_SEEDING");
	RAY_MPI_TAG_START_VERTICES_DISTRIBUTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_VERTICES_DISTRIBUTION");
	RAY_MPI_TAG_TEST_NETWORK=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TEST_NETWORK");
	RAY_MPI_TAG_TEST_NETWORK_MESSAGE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TEST_NETWORK_MESSAGE");
	RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY");
	RAY_MPI_TAG_VERTEX_INFO=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_INFO");
	RAY_MPI_TAG_VERTEX_INFO_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_INFO_REPLY");
	RAY_MPI_TAG_WRITE_AMOS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_AMOS");
	RAY_MPI_TAG_WRITE_KMERS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_KMERS");
	RAY_MPI_TAG_WRITE_KMERS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_KMERS_REPLY");

	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER");
	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY");
	RAY_MPI_TAG_ADD_COLORS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ADD_COLORS");
	RAY_MPI_TAG_ADD_KMER_COLOR=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ADD_KMER_COLOR");
	RAY_MPI_TAG_ASK_EXTENSION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_EXTENSION");
	RAY_MPI_TAG_ASK_EXTENSION_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_EXTENSION_DATA");
	RAY_MPI_TAG_ASK_LIBRARY_DISTANCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_LIBRARY_DISTANCES");
	RAY_MPI_TAG_ASK_READ_LENGTH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_READ_LENGTH");
	RAY_MPI_TAG_ASK_READ_LENGTH_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_READ_LENGTH_REPLY");
	RAY_MPI_TAG_ASK_VERTEX_PATH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATH");
	RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY");
	RAY_MPI_TAG_ASSEMBLE_WAVES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASSEMBLE_WAVES");
	RAY_MPI_TAG_ASSEMBLE_WAVES_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASSEMBLE_WAVES_DONE");
	RAY_MPI_TAG_ATTACH_SEQUENCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ATTACH_SEQUENCE");
	RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY");
	RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION");
	RAY_MPI_TAG_BUILD_GRAPH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_BUILD_GRAPH");
	RAY_MPI_TAG_CLEAR_DIRECTIONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CLEAR_DIRECTIONS");
	RAY_MPI_TAG_CONTIG_ABUNDANCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_ABUNDANCE");
	RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY");
	RAY_MPI_TAG_CONTIG_BIOLOGICAL_ABUNDANCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_BIOLOGICAL_ABUNDANCES");
	RAY_MPI_TAG_CONTIG_INFO=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_INFO");
	RAY_MPI_TAG_CONTIG_INFO_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_INFO_REPLY");
	RAY_MPI_TAG_COUNT_FILE_ENTRIES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COUNT_FILE_ENTRIES");
	RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS");
	RAY_MPI_TAG_DISTRIBUTE_FUSIONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS");
	RAY_MPI_TAG_EXTENSION_DATA_END=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_EXTENSION_DATA_END");

	core->setFirstMasterMode(m_plugin,RAY_MASTER_MODE_LOAD_CONFIG);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_LOAD_CONFIG, RAY_MASTER_MODE_TEST_NETWORK);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_PREPARE_SEEDING,RAY_MASTER_MODE_TRIGGER_SEEDING);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_TRIGGER_SEEDING,RAY_MASTER_MODE_START_UPDATING_DISTANCES);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_LOAD_SEQUENCES,RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS,RAY_MASTER_MODE_SEND_COVERAGE_VALUES);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_SEND_COVERAGE_VALUES,RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING,RAY_MASTER_MODE_PURGE_NULL_EDGES);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_PURGE_NULL_EDGES,RAY_MASTER_MODE_WRITE_KMERS);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_WRITE_KMERS,RAY_MASTER_MODE_TRIGGER_INDEXING);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_TRIGGER_INDEXING,RAY_MASTER_MODE_PREPARE_SEEDING);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_START_UPDATING_DISTANCES,RAY_MASTER_MODE_UPDATE_DISTANCES);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_TRIGGER_FUSIONS,RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS,RAY_MASTER_MODE_START_FUSION_CYCLE);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_START_FUSION_CYCLE,RAY_MASTER_MODE_ASK_EXTENSIONS);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_ASK_EXTENSIONS,RAY_MASTER_MODE_SCAFFOLDER);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_SCAFFOLDER,RAY_MASTER_MODE_WRITE_SCAFFOLDS);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_KILL_RANKS,RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);

	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON");
	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY");
	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY");

	RAY_SLAVE_MODE_DIE=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DIE");

	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON, RAY_SLAVE_MODE_DIE);

	core->setObjectSymbol(m_plugin,&(m_ed->m_EXTENSION_contigs),"/RayAssembler/ObjectStore/ContigPaths.ray");
	core->setObjectSymbol(m_plugin,&(m_ed->m_EXTENSION_identifiers),"/RayAssembler/ObjectStore/ContigNames.ray");

	__BindPlugin(MachineHelper);
}
