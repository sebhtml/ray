/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#define PROCESSED_PERIOD 100000 /* 2^18 */

#define CONFIG_DOUBLE_PRECISION 100000

#include <plugin_Searcher/Searcher.h>
#include <plugin_VerticesExtractor/Vertex.h>
#include <core/OperatingSystem.h>
#include <core/ComputeCore.h>
#include <plugin_Searcher/ColoredPeakFinder.h>
#include <plugin_VerticesExtractor/GridTableIterator.h>

#include <stdio.h> /* for fopen, fprintf and fclose */
#include <fstream>
#include <sstream>
#include <math.h> /* sqrt */
using namespace std;

//#define CONFIG_CONTIG_ABUNDANCE_VERBOSE
//#define CONFIG_COUNT_ELEMENTS_VERBOSE
//#define CONFIG_DEBUG_IOPS
//#define CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
//#define CONFIG_CONTIG_IDENTITY_VERBOSE
//#define CONFIG_DEBUG_COLORS

//#define DEBUG_PHYLOGENY

#define CONFIG_SEARCH_THRESHOLD 0.001
#define CONFIG_FORCE_VALUE_FOR_MAXIMUM_SPEED false

void Searcher::constructor(Parameters*parameters,StaticVector*outbox,TimePrinter*timePrinter,SwitchMan*switchMan,
	VirtualCommunicator*vc,StaticVector*inbox,RingAllocator*outboxAllocator,
GridTable*graph){
	m_subgraph=graph;

	m_closedFiles=false;

	m_inbox=inbox;

	m_startedColors=false;

	m_activeFiles=0;

	m_virtualCommunicator=vc;

	m_countElementsSlaveStarted=false;
	m_countElementsMasterStarted=false;
	
	m_countContigKmersMasterStarted=false;
	m_countContigKmersSlaveStarted=false;

	m_countSequenceKmersMasterStarted=false;
	m_countSequenceKmersSlaveStarted=false;


	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;
	m_timePrinter=timePrinter;
	m_switchMan=switchMan;

	m_searchDirectories_size=0;
	m_searchDirectories=NULL;

	m_pendingMessages=0;
}

void Searcher::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS(Message*message){

	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*sizeof(uint64_t));

	int period=KMER_U64_ARRAY_SIZE+2;

	int outputPosition=0;

	for(int i=0;i<count;i+= period){

		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(incoming,&bufferPosition);

		Vertex*node=m_subgraph->find(&vertex);

		outputPosition+=KMER_U64_ARRAY_SIZE;

		// if it is not there, then it has a coverage of 0
		int coverage=0;
		int numberOfPhysicalColors=0;

		if(node!=NULL){
			coverage=node->getCoverage(&vertex);

			VirtualKmerColorHandle color=node->getVirtualColor();
			set<PhysicalKmerColor>*physicalColors=m_colorSet.getPhysicalColors(color);

			numberOfPhysicalColors=physicalColors->size();
		}

		message2[outputPosition++]=coverage;
		message2[outputPosition++]=numberOfPhysicalColors;
	}

	Rank rank=m_parameters->getRank();

	Message aMessage(message2,count,source,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS_REPLY,rank);
	m_outbox->push_back(aMessage);
}



void Searcher::call_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS(){

	if(!m_countElementsMasterStarted){

		createRootDirectories();

		m_countElementsMasterStarted=true;
		m_ranksDoneCounting=0;
		m_sendCounts=false;
		m_ranksDoneSharing=0;
		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_MASTER_COUNT_REPLY)){
		m_ranksSynced++;
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_SHARING_COMPLETED)){
		m_ranksDoneSharing++;
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_COUNTING_DONE)){

		uint64_t*buffer=m_inbox->at(0)->getBuffer();

		#ifdef ASSERT
		assert(m_inbox->at(0)->getCount()==1);
		#endif

		uint64_t count=buffer[0];

		/* agglomerate the count */
		m_totalNumberOfKmerObservations+=count;

		m_ranksDoneCounting++;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_ELEMENTS)){
		Message*message=m_inbox->at(0);
		uint64_t*buffer=message->getBuffer();
		int directory=buffer[0];
		int file=buffer[1];
		int count=buffer[2];

		m_searchDirectories[directory].setCount(file,count);

		// send a response
		m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),message->getSource(),RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY);

	}else if(m_ranksDoneCounting==m_parameters->getSize()){
		m_ranksDoneCounting=-1;
		
		#ifdef ASSERT
		assert(m_parameters->getRank() == 0);
		#endif

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		// send the total
		buffer[0]=m_totalNumberOfKmerObservations;

		m_switchMan->sendMessageToAll(buffer,1,m_outbox,m_parameters->getRank(),RAY_MPI_TAG_SEARCH_SHARE_COUNTS);

	}else if(m_ranksDoneSharing==m_parameters->getSize()){
		m_sendCounts=true;

		m_masterDirectoryIterator=0;
		m_masterFileIterator=0;

		m_ranksSynced=m_parameters->getSize();

		m_ranksDoneSharing=-1;
	}else if(m_sendCounts && m_ranksSynced == m_parameters->getSize()){
		if(m_masterDirectoryIterator==m_searchDirectories_size){
			m_sendCounts=false;
			
			m_switchMan->sendToAll(m_outbox,m_parameters->getRank(),RAY_MPI_TAG_SEARCH_MASTER_SHARING_DONE);
		}else if(m_masterFileIterator==(int)m_searchDirectories[m_masterDirectoryIterator].getSize()){
			m_masterDirectoryIterator++;
			m_masterFileIterator=0;
		}else if(m_ranksSynced == m_parameters->getSize()){

			int count=m_searchDirectories[m_masterDirectoryIterator].getCount(m_masterFileIterator);

			uint64_t*buffer2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			int bufferSize=0;
			buffer2[bufferSize++]=m_masterDirectoryIterator;
			buffer2[bufferSize++]=m_masterFileIterator;
			buffer2[bufferSize++]=count;

			m_switchMan->sendMessageToAll(buffer2,bufferSize,m_outbox,m_parameters->getRank(),RAY_MPI_TAG_SEARCH_MASTER_COUNT);

			m_ranksSynced=0;

			m_masterFileIterator++;
		}

	}else if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();

		m_timePrinter->printElapsedTime("Counting sequences to search");

		cout<<endl;
	}
}

void Searcher::call_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS(){

	if(!m_countElementsSlaveStarted){

		m_countElementsSlaveStarted=true;
		m_listedDirectories=false;

		m_bufferedData.constructor(m_parameters->getSize(),MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
			"RAY_MALLOC_TYPE_:Searcher",m_parameters->showMemoryAllocations(),KMER_U64_ARRAY_SIZE);

		#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
		cout<<"init"<<endl;
		#endif


	

	// the counting is completed.
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_MASTER_SHARING_DONE)){
		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_MASTER_COUNT)){
		Message*message=m_inbox->at(0);
		uint64_t*buffer=message->getBuffer();
		int directory=buffer[0];
		int file=buffer[1];
		int count=buffer[2];

		m_searchDirectories[directory].setCount(file,count);

		//cout<<"Rank "<<m_parameters->getRank()<<" : RAY_MPI_TAG_SEARCH_MASTER_COUNT "<<directory<<" "<<file<<" "<<count<<" from "<<message->getSource()<<endl;

		// send a response
		m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),message->getSource(),RAY_MPI_TAG_SEARCH_MASTER_COUNT_REPLY);

	}else if(!m_listedDirectories){
		#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
		cout<<"listing"<<endl;
		#endif

		vector<string>*directories=m_parameters->getSearchDirectories();

		// We don't use a a vector for m_searchDirectories
		// because default copy constructors
		// don't manage ifstream attributes correctly
		// which results in a compilation error
	
		if(directories->size() > 0){
			m_searchDirectories_size=directories->size();

			//cout<<"malloc with "<<m_searchDirectories_size<<" sizeof(SearchDirectory) is "<<sizeof(SearchDirectory)<<endl;

			// can not use malloc here because
			// the constructors of vector, ifstream and folks 
			// must be called.
			m_searchDirectories=new SearchDirectory[m_searchDirectories_size];

			#ifdef ASSERT
			assert(m_searchDirectories!=NULL);
			#endif
		}

		for(int i=0;i<(int)directories->size();i++){
			//cout<<"before constructor"<<endl;

			#ifdef ASSERT
			assert(m_searchDirectories!=NULL);
			#endif

			// this is an important line
			m_searchDirectories[i].constructor(directories->at(i));
			//cout<<"after constructor"<<endl;
		}

		m_listedDirectories=true;
		
		m_countedDirectories=false;
	}else if(!m_countedDirectories){
		#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
		cout<<"counting"<<endl;
		#endif

		// no communication here
		int fileNumber=0;

		for(int i=0;i<(int)m_searchDirectories_size;i++){
			int count=m_searchDirectories[i].getSize();
			for(int j=0;j<count;j++){
				if(fileNumber%m_parameters->getSize() == m_parameters->getRank()){
					#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
					cout<<"Hop counting"<<endl;
					#endif

					m_searchDirectories[i].countEntriesInFile(j);
					cout<<"Rank "<<m_parameters->getRank()<<" "<<*(m_searchDirectories[i].getDirectoryName())<<"/"<<*(m_searchDirectories[i].getFileName(j));
					cout<<" -> "<<m_searchDirectories[i].getCount(j)<<" sequences"<<endl;
				}
				fileNumber++;
			}
		}
		m_countedDirectories=true;

		#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
		cout<<"done counting"<<endl;
		#endif
		m_sharedCounts=false;
		m_shareCounts=false;

		// count the k-mer observations for the part of the graph

		uint64_t localKmerObservations=countKmerObservations();

		uint64_t*buffer2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int bufferSize=0;
		buffer2[bufferSize++]=localKmerObservations;

		// tell root that we are done
		m_switchMan->sendMessage(buffer2,bufferSize,m_outbox,m_parameters->getRank(),MASTER_RANK,RAY_MPI_TAG_SEARCH_COUNTING_DONE);

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_SHARE_COUNTS)){

		Message*message=m_inbox->at(0);
		uint64_t*buffer=message->getBuffer();
		
		#ifdef ASSERT
		assert(message->getCount()==1);
		#endif

		m_totalNumberOfKmerObservations=buffer[0];

		m_shareCounts=true;
		m_directoryIterator=0;
		m_fileIterator=0;
		m_waiting=false;
	
		#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
		cout<<"Received RAY_MPI_TAG_SEARCH_SHARE_COUNTS"<<endl;
		#endif

		cout<<"Rank "<<m_parameters->getRank()<<" syncing with master"<<endl;
	}else if(!m_sharedCounts && m_shareCounts){
		if(m_directoryIterator==m_searchDirectories_size){

			m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),MASTER_RANK,RAY_MPI_TAG_SEARCH_SHARING_COMPLETED);

			m_sharedCounts=true;

			m_synchronizationIsDone=false;
		}else if(m_fileIterator==(int)m_searchDirectories[m_directoryIterator].getSize()){
			cout<<"Rank "<<m_parameters->getRank()<<" synced "<<*(m_searchDirectories[m_directoryIterator].getDirectoryName())<<endl;

			m_fileIterator=0;
			m_directoryIterator++;
		}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY)){
			#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
			cout<<"received RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY, resuming work"<<endl;
			#endif

			m_waiting=false;
		}else if(!m_waiting){
			
			int count=m_searchDirectories[m_directoryIterator].getCount(m_fileIterator);

			// we don't need to sync stuff with 0 sequences
			// because 0 is the default value
			if(count==0){
				m_fileIterator++;
				return;
			}

			// sent a response
			uint64_t*buffer2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			int bufferSize=0;
			buffer2[bufferSize++]=m_directoryIterator;
			buffer2[bufferSize++]=m_fileIterator;
			buffer2[bufferSize++]=count;

			Message aMessage(buffer2,bufferSize,MASTER_RANK,
				RAY_MPI_TAG_SEARCH_ELEMENTS,m_parameters->getRank());

			m_outbox->push_back(aMessage);

			m_waiting=true;

		
			#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
			cout<<"Rank "<<m_parameters->getRank()<<" Sending RAY_MPI_TAG_SEARCH_ELEMENTS "<<m_directoryIterator<<" "<<m_fileIterator<<endl;
			#endif

			m_fileIterator++;
		}
	}
}

uint64_t Searcher::countKmerObservations(){

	uint64_t total=0;

	GridTableIterator iterator;
	iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	//* only fetch half of the iterated things because we just need one k-mer
	// for any pair of reverse-complement k-mers 
	
	int parity=0;

	map<int,uint64_t> frequencies;

	while(iterator.hasNext()){

		#ifdef ASSERT
		assert(parity==0 || parity==1);
		#endif

		Vertex*node=iterator.next();
		Kmer key=*(iterator.getKey());
		
		if(parity==0){
			parity=1;
		}else if(parity==1){
			parity=0;

			continue; // we only need data with parity=0
		}

		// check for assembly paths

		/* here, we just want to find a path with
		* a good progression */

		Direction*a=node->m_directions;
		bool nicelyAssembled=false;

		while(a!=NULL){
			int progression=a->getProgression();

			if(progression>= CONFIG_NICELY_ASSEMBLED_KMER_POSITION){
				nicelyAssembled=true;
			}

			a=a->getNext();
		}

		if(!nicelyAssembled){
			continue; // the k-mer is not nicely assembled...
		}

		#ifdef ASSERT
		assert(nicelyAssembled);
		#endif

		// at this point, we have a nicely assembled k-mer
		
		int kmerCoverage=node->getCoverage(&key);

		total+=kmerCoverage;
	}
	
	return total;
}

void Searcher::createRootDirectories(){

	// create directories 
	ostringstream directory1;
	directory1<<m_parameters->getPrefix()<<"/BiologicalAbundances";
	string directory1Str=directory1.str();
	createDirectory(directory1Str.c_str());

	ostringstream directory2;
	directory2<<m_parameters->getPrefix()<<"/BiologicalAbundances/_DeNovoAssembly";
	string directory2Str=directory2.str();
	createDirectory(directory2Str.c_str());

	ostringstream colors;
	colors<<m_parameters->getPrefix()<<"/BiologicalAbundances/_Coloring";
	string directory87=colors.str();
	createDirectory(directory87.c_str());

	ostringstream frequencies;
	frequencies<<m_parameters->getPrefix()<<"/BiologicalAbundances/_Frequencies";
	string frequencyDirectory=frequencies.str();
	createDirectory(frequencyDirectory.c_str());

	if(m_writeDetailedFiles){
		ostringstream directory3;
		directory3<<m_parameters->getPrefix()<<"/BiologicalAbundances/_DeNovoAssembly/Contigs";
		string directory3Str=directory3.str();
		createDirectory(directory3Str.c_str());

		ostringstream directory4;
		directory4<<m_parameters->getPrefix()<<"/BiologicalAbundances/_DeNovoAssembly/Contigs/Coverage";
		string directory4Str=directory4.str();
		createDirectory(directory4Str.c_str());

		ostringstream directory5;
		directory5<<m_parameters->getPrefix()<<"/BiologicalAbundances/_DeNovoAssembly/Contigs/CoverageDistribution";
		string directory5Str=directory5.str();
		createDirectory(directory5Str.c_str());
	}

}

void Searcher::call_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES(){

	if(!m_countContigKmersMasterStarted){
		m_countContigKmersMasterStarted=true;


		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_CONTIG_ABUNDANCE)){
		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Received RAY_MPI_TAG_CONTIG_ABUNDANCE"<<endl;
		#endif

		Message*message=m_inbox->at(0);
		uint64_t*buffer=message->getBuffer();

		int bufferPosition=0;

		uint64_t name=buffer[bufferPosition++];
		int length=buffer[bufferPosition++];
		int coloredKmers=(int)buffer[bufferPosition++];
		int mode=buffer[bufferPosition++];

		//int totalCoverage=mode*length;

		double*bufferDouble=(double*)(buffer+bufferPosition++);
		double mean=bufferDouble[0];

		ContigSearchEntry entry(name,length,mode,mean,coloredKmers);

		m_listOfContigEntries.push_back(entry);

		// sent a response
		uint64_t*buffer2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_CONTIG_ABUNDANCE);

		Message aMessage(buffer2,elementsPerQuery,
			message->getSource(),
			RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY,m_parameters->getRank());

		m_outbox->push_back(aMessage);
	}else if(m_switchMan->allRanksAreReady()){

		// write the contig file

		ofstream contigSummaryFile;
		ostringstream summary;
		summary<<m_parameters->getPrefix()<<"/BiologicalAbundances/_DeNovoAssembly/Contigs.tsv";

		contigSummaryFile.open(summary.str().c_str());

		contigSummaryFile<<"#Contig name	K-mer length	Length in k-mers";
		contigSummaryFile<<"	Colored k-mers";
		contigSummaryFile<<"	Proportion";
		contigSummaryFile<<"	Mode k-mer coverage depth";
		contigSummaryFile<<"	K-mer observations	Total";
		contigSummaryFile<<"	Proportion"<<endl;

		// count the total
		uint64_t total=m_totalNumberOfKmerObservations;
		for(int i=0;i<(int)m_listOfContigEntries.size();i++){

			m_contigLengths[m_listOfContigEntries[i].getName()]=m_listOfContigEntries[i].getLength();
		}

		// write entries
		for(int i=0;i<(int)m_listOfContigEntries.size();i++){
			m_listOfContigEntries[i].write(&contigSummaryFile,total,m_parameters->getWordSize());
		}

		m_listOfContigEntries.clear();

		contigSummaryFile.close();

		m_timePrinter->printElapsedTime("Counting contig biological abundances");
		cout<<endl;

		m_switchMan->closeMasterMode();
	}
}

void Searcher::call_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES(){
	// Process virtual messages
	m_virtualCommunicator->forceFlush();
	m_virtualCommunicator->processInbox(&m_activeWorkers);
	m_activeWorkers.clear();

	if(!m_countContigKmersSlaveStarted){
		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Starting"<<endl;
		#endif

		m_contig=0;
		m_contigPosition=0;
		m_countContigKmersSlaveStarted=true;
		m_workerId=0;

		m_coloredKmers=0;
		m_requestedCoverage=false;

		m_waitingForAbundanceReply=false;

		m_writeDetailedFiles=m_parameters->hasOption("-search-detailed");

		// this is not implemented
		m_writeDetailedFiles=false;

	// we have finished our part
	}else if(m_contig == (int) m_contigs->size()){
		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());

		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Processed all contigs"<<endl;
		#endif

		#ifdef ASSERT
		assert(m_bufferedData.isEmpty());
		#endif

	// we finished a contig
	}else if(!m_requestedCoverage && m_contigPosition==(int)(*m_contigs)[m_contig].size()
		&& m_bufferedData.isEmpty()){

		uint64_t contigName=(*m_contigNames)[m_contig];

		int lengthInKmers=(*m_contigs)[m_contig].size();

		#ifdef CONFIG_DEBUG_IOPS
		cout<<"Closing file"<<endl;
		#endif

		if(m_writeDetailedFiles){
			m_currentCoverageFile.close();
		}

		// write the coverage distribution
		ostringstream file1;
		file1<<m_parameters->getPrefix()<<"/BiologicalAbundances/_DeNovoAssembly/Contigs/CoverageDistribution/contig-"<<contigName<<".tsv";

		#ifdef CONFIG_DEBUG_IOPS
		cout<<"Opening file"<<endl;
		#endif

		ofstream f1;
		if(m_writeDetailedFiles){
			f1.open(file1.str().c_str());
			f1<<"#KmerCoverage	Count"<<endl;
		}

		int mode=0;
		uint64_t sum=0;
		uint64_t totalCount=0;
		int modeCount=0;

		for(map<int,uint64_t>::iterator i=m_coverageDistribution.begin();i!=m_coverageDistribution.end();i++){
			int count=i->second;
			int coverage=i->first;

			if(m_writeDetailedFiles){
				f1<<coverage<<"	"<<count<<endl;
			}

			if(count>modeCount){
				mode=coverage;
				modeCount=count;
			}

			sum+=coverage*count;
			totalCount+=count;
		}

		double mean=sum;

		if(totalCount>0)
			mean=(0.0+sum)/totalCount;

		if(m_writeDetailedFiles){
			f1.close();
		}

		#ifdef CONFIG_DEBUG_IOPS
		cout<<"Closing file"<<endl;
		#endif

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int bufferSize=0;
		buffer[bufferSize++]=contigName;
		buffer[bufferSize++]=lengthInKmers;
		buffer[bufferSize++]=m_coloredKmers;
		buffer[bufferSize++]=mode;
		
		double*bufferDouble=(double*)(buffer+bufferSize++);
		bufferDouble[0]=mean;

		int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_CONTIG_ABUNDANCE);

		Message aMessage(buffer,elementsPerQuery,
			MASTER_RANK,
			RAY_MPI_TAG_CONTIG_ABUNDANCE,m_parameters->getRank());

		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);

		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Finished contig positions="<<m_contigPosition<<" size="<<(*m_contigs)[m_contig].size()<<endl;
		cout<<"Closing file"<<endl;
		cout<<"Sending abundance"<<endl;
		#endif

		m_contigPosition=0;
		m_contig++;

		m_waitingForAbundanceReply=true;

		#ifdef ASSERT
		assert(m_bufferedData.isEmpty());
		#endif

		// empty the coverage distribution
		m_coverageDistribution.clear();
		m_coloredKmers=0;

	// pull the reply
	}else if(m_waitingForAbundanceReply && m_virtualCommunicator->isMessageProcessed(m_workerId)){
		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Received reply for abundance"<<endl;
		#endif

		m_waitingForAbundanceReply=false;
		vector<uint64_t> data;
		m_virtualCommunicator->getMessageResponseElements(m_workerId,&data);

	// process contig kmers
	}else if(!m_waitingForAbundanceReply && !m_requestedCoverage && m_pendingMessages==0 
		 && m_contigPosition < (int)(*m_contigs)[m_contig].size()){ // we still have things to process

		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Requesting coverage at "<<m_contigPosition<<endl;
		#endif


		bool force=CONFIG_FORCE_VALUE_FOR_MAXIMUM_SPEED;

		// pull k-mers from the sequence and fill buffers
		// if one of the buffer if full,
		// flush it and return and wait for a response

		bool gatheringKmers=true;

		while(gatheringKmers && m_contigPosition < (int)(*m_contigs)[m_contig].size()){

			// show some information
			if(m_contigPosition==0){
				#ifdef CONFIG_DEBUG_IOPS
				cout<<"Opening file"<<endl;
				#endif
	
				if(m_writeDetailedFiles){

					uint64_t contigName=m_contigNames->at(m_contig);
					ostringstream file2;
					file2<<m_parameters->getPrefix()<<"/BiologicalAbundances/m_DeNovoAssembly/Contigs/Coverage/contig-"<<contigName<<".tsv";

					m_currentCoverageFile.open(file2.str().c_str());
					m_currentCoverageFile<<"#KmerPosition	KmerCoverage"<<endl;
				}
			}
	
			// show progression
			if(m_contigPosition % 1000 == 0 || m_contigPosition==(int)m_contigs->at(m_contig).size()-1){
				showContigAbundanceProgress();
			}

			#ifdef ASSERT
			assert(m_contig<(int)(*m_contigs).size());
			assert(m_contigPosition<(int)(*m_contigs)[m_contig].size());
			#endif

			// get the kmer
			Kmer*kmer=&((*m_contigs)[m_contig][m_contigPosition]);
			m_contigPosition++;

			int rankToFlush=m_parameters->_vertexRank(kmer);

			int period=KMER_U64_ARRAY_SIZE+2;

			// pack the k-mer
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_bufferedData.addAt(rankToFlush,kmer->getU64(i));
			}

			// do some padding
			m_bufferedData.addAt(rankToFlush,0);
			m_bufferedData.addAt(rankToFlush,0);

			// force flush the message
			if(m_bufferedData.flush(rankToFlush,period,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS,m_outboxAllocator,m_outbox,
				m_parameters->getRank(),force)){

				m_pendingMessages++;
				gatheringKmers=false;
			}
		}

		// at this point, we flushed something
		// or we processed all k-mers
		// if nothing was flushed, we force something now
		if(m_pendingMessages==0){

			m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS,m_outboxAllocator,
				m_outbox,m_parameters->getRank());

			#ifdef ASSERT
			assert(m_pendingMessages>0);
			#endif
		}

		m_requestedCoverage=true;

		#ifdef ASSERT
		assert(m_pendingMessages>0);
		#endif

	// receive the coverage values
	}else if(!m_waitingForAbundanceReply &&m_requestedCoverage && m_pendingMessages > 0
			 && m_inbox->hasMessage(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS_REPLY)){

		Message*message=m_inbox->at(0);
		uint64_t*buffer=message->getBuffer();
		int count=message->getCount();

		m_pendingMessages--;

		#ifdef ASSERT
		if(m_pendingMessages!=0){
			cout<<"m_pendingMessages is "<<m_pendingMessages<<" but should be "<<m_pendingMessages<<endl;
		}

		assert(m_pendingMessages==0);
		#endif

		int period=KMER_U64_ARRAY_SIZE+2;

		// iterate over the coverage values
		for(int i=0;i<count;i+=period){

			int bufferPosition=i;
			bufferPosition+=KMER_U64_ARRAY_SIZE;

			// get the coverage.
			int coverage=buffer[bufferPosition++];
			int colors=(int)buffer[bufferPosition++];

			#ifdef ASSERT
			assert(coverage>0);
			#endif

			m_coverageDistribution[coverage]++;

			if(colors>0){
				m_coloredKmers++;
			}

			if(m_writeDetailedFiles){
				m_currentCoverageFile<<"Unknown"<<"	"<<coverage<<endl;
			}
		}

		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Receiving coverage at "<<m_contigPosition<<endl;
		#endif

		m_requestedCoverage=false;

		#ifdef ASSERT
		assert(m_pendingMessages==0);
		assert(m_requestedCoverage==false);
		#endif

	// we finished all the work, but now we must flush stuff
	}else if(!m_bufferedData.isEmpty() 
		&& m_pendingMessages==0){ // only flush one thing at any time

		// we flush some of what is left
		m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS,m_outboxAllocator,
			m_outbox,m_parameters->getRank());

		// and it is as if we had requested it for real.
		m_requestedCoverage=true;

		#ifdef ASSERT
		assert(m_pendingMessages>0);
		#endif
	}
}

void Searcher::setContigs(vector<vector<Kmer> >*paths,vector<uint64_t>*names){
	m_contigs=paths;
	m_contigNames=names;
}

void Searcher::showContigAbundanceProgress(){
	cout<<"Rank "<<m_parameters->getRank()<<" computing contig abundances ["<<m_contig+1<<"/"<<m_contigs->size()<<"] ["<<m_contigPosition+1<<"/";
	cout<<(*m_contigs)[m_contig].size()<<"]"<<endl;

}

void Searcher::call_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES(){
	if(!m_countSequenceKmersMasterStarted){

		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());

		m_countSequenceKmersMasterStarted=true;

		m_numberOfRanksThatFinishedSequenceAbundances=0;
		m_rankToCall=0;

		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"MASTER START call_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES"<<endl;
		#endif

	// a rank finished computing abundances in its files
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEQUENCE_ABUNDANCE_FINISHED)){
		m_numberOfRanksThatFinishedSequenceAbundances++;

		// ask the first rank to write its files
		if(m_numberOfRanksThatFinishedSequenceAbundances==m_parameters->getSize()){
			m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),m_rankToCall,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES);
			m_rankToCall++;
		}
		
	// a rank has written all its files
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_REPLY)){

		/// all ranks have written their files
		if(m_rankToCall == m_parameters->getSize()){

			m_switchMan->sendToAll(m_outbox,m_parameters->getRank(),RAY_MPI_TAG_SEQUENCE_ABUNDANCE_YOU_CAN_GO_HOME);

		// otherwise, ask the next rank to write its files
		}else{
			m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),m_rankToCall,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES);
			m_rankToCall++;
		}

	// this step is over
	}else if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();

		m_timePrinter->printElapsedTime("Counting sequence biological abundances");

		cout<<endl;


		m_contigLengths.clear();
	}

}

void Searcher::call_RAY_MASTER_MODE_SEARCHER_CLOSE(){

	if(!m_closedFiles){
		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());
		m_closedFiles=true;
	}else if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();
	}

}

void Searcher::call_RAY_SLAVE_MODE_SEARCHER_CLOSE(){

	// close identification files
	for(map<int,FILE*>::iterator i=m_identificationFiles.begin();
		i!=m_identificationFiles.end();i++){
		fclose(i->second);
	}

	m_identificationFiles.clear();

	// close abundance files

	// close the file
	// even if there is 0 matches,
	// we need to close the file...
	// if it exists of course

	for(map<int,FILE*>::iterator i=m_arrayOfFiles.begin();
		i!=m_arrayOfFiles.end();i++){

		int directoryIterator=i->first;

		fprintf(m_arrayOfFiles[directoryIterator],"</root>");
		fclose(m_arrayOfFiles[directoryIterator]);

		fprintf(m_arrayOfFilesShort[directoryIterator],"</root>");
		fclose(m_arrayOfFilesShort[directoryIterator]);

		cout<<"Closed file "<<m_directoryIterator<<" "<<m_fileIterator<<", active file descriptors: "<<m_activeFiles<<endl;

		m_activeFiles--;
	}

	m_arrayOfFiles.clear();
	m_arrayOfFilesShort.clear();

	#ifdef ASSERT
	assert(m_activeFiles==0);
	#endif

	// close the distribution writer
	m_writer.close();

	m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());
}

/** massively parallel implementation of
 * the biological abundance problem solver.
 * each MPI rankcount things.
 *
 * any MPI rank will have 0 or 1 file descriptor for reading.
 * the master MPI rank will have between 0 and NumberOfRanks
 * inclusively file descriptors for writing.
 *
 * VirtualCommunicator service is provided by VirtualCommunicator and by
 * BufferedData (legacy, but better in some cases)
 *
 * TODO: contig identification should be done elsewhere do reduce the overall communication burden.
 *
 * \author Sébastien Boisvert
 */
void Searcher::call_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES(){

	// Process virtual messages
	m_virtualCommunicator->forceFlush();
	m_virtualCommunicator->processInbox(&m_activeWorkers);

	// this contains a list of workers that received something
	// here, we only have one worker.
	m_activeWorkers.clear();

	if(!m_countSequenceKmersSlaveStarted){

		ostringstream frequencies;
		frequencies<<m_parameters->getPrefix()<<"/BiologicalAbundances/_Frequencies";
		string frequencyDirectory=frequencies.str();

		// build the writer
		m_writer.setBase(frequencyDirectory.c_str());
		m_writer.setRank(m_parameters->getRank());

		int virtualColors=m_colorSet.getTotalNumberOfVirtualColors();
		int physicalColors=m_colorSet.getTotalNumberOfPhysicalColors();
		cout<<"Rank "<<m_parameters->getRank()<<" colored the graph with "<<physicalColors<<" real colors using "<<virtualColors<<" virtual colors"<<endl;

		//cout<<"VIRTUAL COLOR SUMMARY"<<endl;

		ostringstream summaryFile;
		summaryFile<<m_parameters->getPrefix()<<"/BiologicalAbundances/_Coloring/"<<m_parameters->getRank()<<".Operations.txt";
		ofstream f1(summaryFile.str().c_str());

		m_colorSet.printSummary(&f1);

		f1.close();

		#ifdef CONFIG_DEBUG_COLORS

		ostringstream virtualStuff;
		virtualStuff<<m_parameters->getPrefix()<<"/BiologicalAbundances/_Coloring/"<<m_parameters->getRank()<<".VirtualColors.txt";
		ofstream f2(virtualStuff.str().c_str());
		m_colorSet.printColors(&f2);
		f2.close();

		#endif

		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"Starting call_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES"<<endl;
		#endif

		m_directoryIterator=0;
		m_fileIterator=0;
		m_globalFileIterator=0;
		m_sequenceIterator=0;
		m_globalSequenceIterator=0;

		m_countSequenceKmersSlaveStarted=true;
		m_createdSequenceReader=false;

		m_kmerLength=m_parameters->getWordSize();

		// compute partition
		int total=0;
		for(int i=0;i<(int)m_searchDirectories_size;i++){
			for(int j=0;j<(int)m_searchDirectories[i].getSize();j++){

				#ifdef ASSERT
				assert(i<m_searchDirectories_size);
				assert(j<m_searchDirectories[i].getSize());
				#endif

				total+=m_searchDirectories[i].getCount(j);
			}
		}

		int slice=total/m_parameters->getSize();
	
		int rank=m_parameters->getRank();
		m_firstSequence=rank*slice;
		m_lastSequence=(rank+1)*slice;
	
		if(rank==m_parameters->getSize()-1){
			m_lastSequence=total-1;
		}

		m_checkedHits=true;

		//int toProcess=m_lastSequence-m_firstSequence+1;

		//cout<<"Rank "<<rank<<" partition: "<<m_firstSequence<<" to "<<m_lastSequence<<" (total: "<<toProcess<<")"<<endl;
	
		printDirectoryStart();


		m_kmersProcessed=0;

		m_finished=false;

		m_derivative.clear();

		m_lastPrinted=0;
		m_derivative.addX(m_kmersProcessed);

		m_processedFiles=0;
		m_processedSequences=0;

	}else if(m_pendingMessages > 0 && m_inbox->hasMessage(RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY)){

		// We are ready to check the hits
		
		#ifdef CONFIG_DEBUG_IOPS
		cout<<"Received RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY"<<endl;
		cout<<"Is ready to check hits"<<endl;
		#endif

		m_checkedHits=false;

		m_pendingMessages--;

		#ifdef ASSERT
		assert(m_pendingMessages==0);
		#endif

	// we must check the hits
	// hits are checked for contig identification
	// TODO: this code uses a lot of memory I believe.
	}else if(!m_checkedHits){
	
		// all hits were processed
		if(m_sortedHitsIterator==m_sortedHits.end()){
			m_checkedHits=true;

			// go to the next sequence
			m_sequenceIterator++;
			m_globalSequenceIterator++;
			m_createdSequenceReader=false;
			m_requestedCoverage=false;

			#ifdef ASSERT
			assert(m_pendingMessages==0);
			#endif

			//cout<<"Done checking hits"<<endl;

	
		// receive the reply
		}else if(m_inbox->hasMessage(RAY_MPI_TAG_CONTIG_IDENTIFICATION_REPLY)){
			m_pendingMessages--;
			
			// next! ,please.
			m_sortedHitsIterator++;

		// wait for a reply
		}else if(m_pendingMessages>0){
			// wait
			
		// at this point, we have a valid iterator
		}else{
	
			#ifdef ASSERT
			assert(m_pendingMessages==0);
			assert(m_sortedHitsIterator!=m_sortedHits.end());
			#endif

			// here, m_contigCounts contains thing related to contig counts
			// seed a message to root with these information:
			//
			// directory
			// file
			// sequenceNumber
			// sequenceName (possibly long) 
			// <wordSize is known on the other end>
			// sequenceLength
			//
			// number of contigs that have a hit
			//
			//  for each contig hit:
			//
			// contigName
			// <contig length is known on the other end>
			// Matches on the contig in k-mers
		
			ContigHit hit=*m_sortedHitsIterator;
			
			uint64_t contig=hit.getContig();
			int count=hit.getMatches();
			char strand=hit.getStrand();

			uint64_t*messageBuffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			int bufferPosition=0;

			messageBuffer[bufferPosition++]=contig;
			messageBuffer[bufferPosition++]=strand;
			messageBuffer[bufferPosition++]=count;
			messageBuffer[bufferPosition++]=m_directoryIterator;
			messageBuffer[bufferPosition++]=m_fileIterator;
			messageBuffer[bufferPosition++]=m_sequenceIterator;
			messageBuffer[bufferPosition++]=m_numberOfKmers;

			#ifdef ASSERT
			if(count > m_numberOfKmers){
				cout<<"Error before sending, count= "<<count<<" m_numberOfKmers= "<<m_numberOfKmers<<endl;
				cout<<"m_sequenceIterator= "<<m_sequenceIterator<<endl;
				cout<<"contig= "<<contig<<" strand= "<<strand<<endl;
				cout<<"m_sortedHits.size()= "<<m_sortedHits.size()<<endl;
			}
			assert(count <= m_numberOfKmers);
			#endif

			char*sequence=(char*) (messageBuffer+bufferPosition);
			
			string sequenceName=m_searchDirectories[m_directoryIterator].getCurrentSequenceName();

			strcpy(sequence,sequenceName.c_str());

			// here, the message is ready to be send.
			// the filtering is done on the other end...

			Message aMessage(messageBuffer,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
				getWriter(m_directoryIterator),RAY_MPI_TAG_CONTIG_IDENTIFICATION,m_parameters->getRank());

			m_outbox->push_back(aMessage);
		
			m_pendingMessages++;

		}

	// all directories were processed
	}else if(m_directoryIterator==m_searchDirectories_size && !m_finished){
	
		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"Finished call_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES"<<endl;
		#endif

		showProcessedKmers();

		m_bufferedData.showStatistics(m_parameters->getRank());

		// tell master that we have finished our task
		m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),MASTER_RANK,RAY_MPI_TAG_SEQUENCE_ABUNDANCE_FINISHED);
	
		// don't come here again.
		m_finished=true;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES)){
		
		// write files
		// code to be added...

		// now that we have written files, we can signal master about it
		m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),MASTER_RANK,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_REPLY);

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEQUENCE_ABUNDANCE_YOU_CAN_GO_HOME)){

		// this kills the batman
		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());

	// all files in a directory were processed
	}else if(!m_finished && m_fileIterator==(int)m_searchDirectories[m_directoryIterator].getSize()){
	
		cout<<"Rank "<<m_parameters->getRank()<<" has processed its entries from "<<m_directoryNames[m_directoryIterator]<<endl;
		cout<<endl;

		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"next directory"<<endl;
		#endif

		m_directoryIterator++;
		m_fileIterator=0;
		m_sequenceIterator=0;

		printDirectoryStart();

	// all sequences in a file were processed
	}else if(!m_finished && m_sequenceIterator==m_searchDirectories[m_directoryIterator].getCount(m_fileIterator)){

		m_fileIterator++;
		m_globalFileIterator++;
		m_sequenceIterator=0;

		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"Next file"<<endl;
		#endif
	
		m_processedFiles++;

	// this sequence is not owned by me
	}else if(!isFileOwner(m_globalFileIterator) && !m_finished){

		m_globalSequenceIterator+=m_searchDirectories[m_directoryIterator].getCount(m_fileIterator);

		// skip the file
		// ownership is on a per-file basis
		m_fileIterator++;
		m_globalFileIterator++;
		m_sequenceIterator=0;
		
		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"Skipping"<<endl;
		#endif

	// start a sequence
	}else if(!m_createdSequenceReader && !m_finished){
		// initiate the reader I guess
	
		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"Create sequence reader"<<endl;
		#endif

		m_searchDirectories[m_directoryIterator].createSequenceReader(m_fileIterator,m_sequenceIterator,m_parameters->getWordSize());
		
		m_currentLength=m_searchDirectories[m_directoryIterator].getCurrentSequenceLengthInKmers();

		m_contigCounts.clear();

		m_sortedHits.clear();
		m_sortedHitsIterator=m_sortedHits.begin();

		m_numberOfKmers=0;

		m_createdSequenceReader=true;

		m_matches=0;
		m_coverageDistribution.clear();

		m_coloredMatches=0;
		m_coloredCoverageDistribution.clear();

		m_coloredAssembledMatches=0;
		m_coloredAssembledCoverageDistribution.clear();

		m_requestedCoverage=false;

		m_processedSequences++;

		m_sequencesToProcessInFile=m_searchDirectories[m_directoryIterator].getCount(m_fileIterator);

		m_processedAbundance=false;

	// compute abundances
	}else if(m_createdSequenceReader && !m_finished){

		// the current sequence has been processed
		if(m_pendingMessages==0  && m_bufferedData.isEmpty() &&  /* nothing to flush... */
			 !m_searchDirectories[m_directoryIterator].hasNextKmer(m_kmerLength)
			&& !m_processedAbundance){
			
			// process the thing, possibly send it to be written

			int mode=getDistributionMode(&m_coverageDistribution);
/*
			double mean=sum;

			if(totalCount>0)
				mean=(0.0+sum)/totalCount;
*/
			
			showProcessedKmers();

			#ifdef ASSERT
			assert(m_directoryIterator<m_searchDirectories_size);
			assert(m_fileIterator<m_searchDirectories[m_directoryIterator].getSize());
			#endif

			// is it the last sequence in this file ?
			bool isLast=(m_sequenceIterator== m_searchDirectories[m_directoryIterator].getCount(m_fileIterator)-1);

			// don't send things with 0 matches
			// is it is the last, send it anyway
			// because we need to close files on the other end
			if(!isLast && m_matches==0){
				
				#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
				cout<<"No matches, not sending"<<endl;
				#endif

				m_sequenceIterator++;
				m_globalSequenceIterator++;
				m_createdSequenceReader=false;

				return;

			}

			// compute the colored mode
			
			int coloredMode=getDistributionMode(&m_coloredCoverageDistribution);

			int coloredAssembledMode=getDistributionMode(&m_coloredAssembledCoverageDistribution);

			#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
			cout<<"dir "<<m_directoryIterator<<" file "<<m_fileIterator<<" sequence "<<m_sequenceIterator<<" global "<<m_globalSequenceIterator<<" mode "<<mode<<endl;
			#endif

			double ratio=(0.0+m_matches)/m_numberOfKmers;


			// to be worthy, an entry needs a minimum number of matches
			bool entryIsWorthy=false;

			if(ratio >= CONFIG_SEARCH_THRESHOLD){
				entryIsWorthy=true;
			}

			m_sortedHits.clear();

			if(entryIsWorthy){

				double qualityColoredVsAll=m_caller.computeQuality(&m_coloredCoverageDistribution,&m_coverageDistribution);
				double qualityAssembledVsAll=m_caller.computeQuality(&m_coloredAssembledCoverageDistribution,&m_coverageDistribution);
				double qualityAssembledVsColored=m_caller.computeQuality(&m_coloredAssembledCoverageDistribution,&m_coloredCoverageDistribution);

				vector<int> xValues;
				vector<int> yValues;
				vector<int> peaks;
				vector<int> deviations;

				bool hasHighFrequency=false;

				for(map<int,uint64_t>::iterator i=m_coloredAssembledCoverageDistribution.begin();
					i!=m_coloredAssembledCoverageDistribution.end();i++){
					
					xValues.push_back(i->first);
					yValues.push_back(i->second);
		
					if(i->second >= 1024){
						hasHighFrequency=true;
					}
				}
	
				ColoredPeakFinder peakCaller;
				peakCaller.findPeaks(&xValues,&yValues,&peaks,&deviations);

				bool hasPeak=peaks.size()>=1;


				//cout<<"Adding hits for sequence "<<m_sequenceIterator<<endl;

				// store the hits
				// sort hits
				for(map<uint64_t,set<int> >::iterator i=m_contigCounts['F'].begin();i!=m_contigCounts['F'].end();i++){
					int matches=i->second.size();
					uint64_t contig=i->first;

					ContigHit hit(m_sequenceIterator,contig,'F',matches);
					m_sortedHits.push_back(hit);
		
					// the number of matches can not exceed the length
					#ifdef ASSERT
					assert(matches <= m_numberOfKmers);
					#endif
		
					//cout<<"contig-"<<contig<<" has "<<matches<<" matches on strand 'F'"<<endl;
					//cout.flush();
				}
		
				for(map<uint64_t,set<int> >::iterator i=m_contigCounts['R'].begin();i!=m_contigCounts['R'].end();i++){
					int matches=i->second.size();
					uint64_t contig=i->first;
		
					ContigHit hit(m_sequenceIterator,contig,'R',matches);
					m_sortedHits.push_back(hit);
					
					// the number of matches can not exceed the length
					#ifdef ASSERT
					assert(matches <= m_numberOfKmers);
					#endif
		
					//cout<<"contig-"<<contig<<" has "<<matches<<" matches on strand 'R'"<<endl;
					//cout.flush();
				}
		
				// send a message to write the abundances

				uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
				int bufferPosition=0;

				// pack stuff.

				buffer[bufferPosition++]=m_directoryIterator;
				buffer[bufferPosition++]=m_fileIterator;
				buffer[bufferPosition++]=m_sequenceIterator;

				#ifdef ASSERT
				assert(m_directoryIterator==(int)buffer[0]);
				assert(m_fileIterator==(int)buffer[1]);
				assert(m_sequenceIterator==(int)buffer[2]);
				#endif

				buffer[bufferPosition++]=m_numberOfKmers;

				buffer[bufferPosition++]=m_matches;
				buffer[bufferPosition++]=mode;

				buffer[bufferPosition++]=m_coloredMatches;
				buffer[bufferPosition++]=coloredMode;

				buffer[bufferPosition++]=m_coloredAssembledMatches;
				buffer[bufferPosition++]=coloredAssembledMode;

				buffer[bufferPosition++]=entryIsWorthy;

				buffer[bufferPosition++]=qualityColoredVsAll*CONFIG_DOUBLE_PRECISION;
				buffer[bufferPosition++]=qualityAssembledVsAll*CONFIG_DOUBLE_PRECISION;
				buffer[bufferPosition++]=qualityAssembledVsColored*CONFIG_DOUBLE_PRECISION;

				buffer[bufferPosition++]=hasPeak;
				buffer[bufferPosition++]=hasHighFrequency;

				string sequenceName=m_searchDirectories[m_directoryIterator].getCurrentSequenceName();

				char*sequenceNameFromMessage=(char*)(buffer+bufferPosition);
				strcpy(sequenceNameFromMessage,sequenceName.c_str());

				// the rank that can write to this directory.
				int writer=getAbundanceWriter(m_directoryIterator);

				Message aMessage(buffer,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
					writer,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY,
					m_parameters->getRank());

				m_outbox->push_back(aMessage);

				#ifdef ASSERT
				assert(m_directoryIterator==(int)buffer[0]);
				assert(m_fileIterator==(int)buffer[1]);
				assert(m_sequenceIterator==(int)buffer[2]);
				assert(m_outbox->size()>0);
				#endif

				#ifdef ASSERT
				assert(m_pendingMessages==0);
				#endif

				m_pendingMessages++;
				
				#ifdef ASSERT
				assert(m_pendingMessages==1);
				#endif

				/* also dump the distribution */
				/* write it in BiologicalAbundances/Directory/FileName/SequenceNumber.tsv */
	
				// don't dump too many of these distribution
				// otherwise, it may result in too many files
	
				dumpDistributions();
			}


			// no hits to check...
			if(m_pendingMessages==0){
				m_checkedHits=false;
			}

			//cout<<"Will check hits later "<<endl;
			//cout<<"At least "<<m_sortedHits.size()<<endl;

			m_sortedHitsIterator=m_sortedHits.begin();

			m_processedAbundance=true;

		// we have to wait for a reply
		}else if(m_pendingMessages==0 && !m_searchDirectories[m_directoryIterator].hasNextKmer(m_kmerLength) 
				 && m_bufferedData.isEmpty()){

			// we have to wait because we sent the summary 
			// and the response is not there yet

		// k-mers are available
		// pull data from the sequence
		// and throw messages onto the network
		}else if(m_pendingMessages==0 && !m_requestedCoverage ){
			/*	&& m_bufferedData.isEmpty() ){*/ // TODO: remove the last condition, it is not necessary and it would accelerate the thing...
	
			// don't use message aggregation ?
			// true= no multiplexing
			// false= multiplexing
			//
			// if this is set to true
			// thie whole thing will be 10 to 100 
			// times slower
			//
			// Actual numbers:
			//
			// force=true
			// 	Speed= 25000
			//
			// force=false
			//	Speed= 800000
			//
			//	speedup: 32
			//
			// this would be a good switch it Ray would
			// be a commercial product.
			//
			// - Raytrek Ray 8 Free Edition
			// - Raytrek Ray 8i 00.54.222.1 Edition Opal (including 10x speed increase)
			//
			// In fact, switching off BufferedData and VirtualCommunicator
			// everywhere would mean that assembling
			// a bacteria genome (~ 5 minutes) would
			// take 5 hours.
			//
			// this impressive speed-up could be further improved
			// by trading the BufferedData for a VirtualCommunicator
			// (both provide the service of VirtualCommunicator,
			// but BufferedData is manual while VirtualCommunicator
			// is automated
			// Furthermore, VirtualCommunicator can be used with VirtualProcessor
			// so tasks can be splitted...
			//
			// but here this is not feasible because tasks are input-output-bounded
			// spawning workers would spawn too many file descriptors
			//and it would kill the virtual file system, if any.

			bool force=CONFIG_FORCE_VALUE_FOR_MAXIMUM_SPEED;

			// pull k-mers from the sequence and fill buffers
			// if one of the buffer if full,
			// flush it and return and wait for a response

			bool gatheringKmers=true;

			while(gatheringKmers && m_searchDirectories[m_directoryIterator].hasNextKmer(m_kmerLength)){
				Kmer kmer;
				m_searchDirectories[m_directoryIterator].getNextKmer(m_kmerLength,&kmer);
				m_searchDirectories[m_directoryIterator].iterateToNextKmer();

				// if the k-mer contains non-standard character,
				// skip it

				if(m_searchDirectories[m_directoryIterator].kmerContainsN()){
					m_numberOfKmers++; // the number of k-mers for the sequence

					m_kmersProcessed++; // the total number of k-mers processed

					showProcessedKmers();

					continue;
				}

				int rankToFlush=m_parameters->_vertexRank(&kmer);

				int added=0;
				// pack the k-mer
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedData.addAt(rankToFlush,kmer.getU64(i));
					added++;
				}

				#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
				cout<<"Will request stuff at sequence position "<<m_numberOfKmers<<endl;
				#endif

				// add the sequence position
				m_bufferedData.addAt(rankToFlush,m_numberOfKmers);
				added++;

				// this little guy have to be right after iterateToNextKmer()
				// otherwise, it is buggy.
				m_numberOfKmers++;
				m_kmersProcessed++;

				showProcessedKmers();

				int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_COVERAGE_AND_PATHS);

				// pad the thing to the ground
				while(added<period){
					m_bufferedData.addAt(rankToFlush,0);
					added++;
				}
	
				// force flush the message
				if(m_bufferedData.flush(rankToFlush,period,
					RAY_MPI_TAG_GET_COVERAGE_AND_PATHS,m_outboxAllocator,m_outbox,
					m_parameters->getRank(),force)){

					m_pendingMessages++;
					gatheringKmers=false;
				}
			}

			// at this point, we flushed something
			// or we processed all k-mers
			// if nothing was flushed, we force something now
			if(m_pendingMessages==0){

				m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_GET_COVERAGE_AND_PATHS,
					m_outboxAllocator,
					m_outbox,m_parameters->getRank());

				#ifdef ASSERT
				assert(m_pendingMessages>=0);
				#endif
			}

			#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
			if(m_numberOfKmers%1000==0)
				cout<<"Sending RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE, position="<<m_numberOfKmers<<endl;
			#endif
	
			m_requestedCoverage=true;

		// we have a response
		}else if(m_pendingMessages > 0 &&
				m_requestedCoverage && m_inbox->hasMessage(RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY)){

			Message*message=m_inbox->at(0);
			uint64_t*buffer=message->getBuffer();

			#ifdef ASSERT
			assert(message!=NULL);
			#endif

			int count=message->getCount();

/* TODO: implement responses 
			uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*sizeof(uint64_t));
			int outputBufferPosition=0;
*/

			m_pendingMessages--;

			#ifdef ASSERT
			assert(m_pendingMessages==0);
			#endif

			// the size (number of uint64_t) of things per vertex
			int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_COVERAGE_AND_PATHS);

			#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
			cout<<endl;
			cout<<"RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY"<<endl;
			cout<<"meta count= "<<count<<" period= "<<period<<endl;
			#endif

			#ifdef ASSERT
			assert(count%period==0);
			#endif

			// iterate over the coverage values
			for(int i=0;i<count;i+=period){
	// the message contains:
	//  a k-mer
	//  sequence position
	//  a coverage value
	//  a count
	//  an index
	//  a total
	//  a list of pairs

				int bufferPosition=i;

				Kmer kmer;

				// unpack the k-mer
				// we will need it if it is repeated.
				kmer.unpack(buffer,&bufferPosition);

				int sequencePosition=buffer[bufferPosition++];
	
				#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
				cout<<"sequence position "<<sequencePosition<<endl;
				#endif

				// get the coverage.
				int coverage=buffer[bufferPosition++];

				#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
				cout<<"Coverage is "<<coverage<<endl;
				#endif

				if(coverage>= 2){// the k-mer exists

					m_coverageDistribution[coverage]++;
					m_matches++;

					bool colorsAreUniqueInNamespaces=buffer[bufferPosition++];
	
					bool nicelyAssembled=buffer[bufferPosition++];

					if(colorsAreUniqueInNamespaces){
						m_coloredCoverageDistribution[coverage]++;
						m_coloredMatches++;
					}

					int numberOfPaths=buffer[bufferPosition++];

					#ifdef ASSERT

					int index=buffer[bufferPosition];
					int total=buffer[bufferPosition+1];

					assert(numberOfPaths<=total);
					assert(index<=total);
					#endif

					bufferPosition++; // skip the index
					bufferPosition++; // skip th total

					if(numberOfPaths>0 && nicelyAssembled && colorsAreUniqueInNamespaces){
						
						m_coloredAssembledCoverageDistribution[coverage]++;
						m_coloredAssembledMatches++;

						//cout<<"WARNING sequence position "<<sequencePosition<<" has coverage "<<coverage<<" but no contig path"<<endl;
					}

					#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
					cout<<"Paths: "<<numberOfPaths<<" total: "<<total<<endl;
					#endif
	
					//bool reply=false;

					// this flag will say if we need to send another message
					// TODO: implement it
					//if(index < total)
						//reply=true;

					for(int j=0;j<numberOfPaths;j++){
						uint64_t contigPath=buffer[bufferPosition++];
						int contigPosition=buffer[bufferPosition++];
						char strand=buffer[bufferPosition++];

						#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
						cout<<"Contig Data "<<contigPath<<" "<<contigPosition<<endl;
						#endif

						// don't process the same item twice for the current position
						if(m_observedPaths.count(sequencePosition)>0 
							&& m_observedPaths[sequencePosition].count(contigPath)>0){  // a contig path can only be processed once per position<

							#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
							cout<<"Skipping because we already used "<<contigPath<<" for position "<<sequencePosition<<endl;
							#endif

							continue; // 
						}

						// a contig position can only be utilised once
						// for all the sequence queried
						if((m_contigCounts['F'].count(contigPath)>0 && m_contigCounts['F'][contigPath].count(contigPosition)>0)
							||(m_contigCounts['R'].count(contigPath)>0 && m_contigCounts['R'][contigPath].count(contigPosition)>0)){

							#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
							cout<<"Skipping because we already used contig "<<contigPosition<<" (contig position: "<<contigPosition<<" for current sequence"<<endl;
							#endif

							continue;  //
						}

						#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
						cout<<"Adding contig "<<contigPath<<", position "<<contigPosition<<" for current sequence at position "<<sequencePosition<<endl;
						#endif

						int theContigPosition=contigPosition;

						// add the contig position
						m_contigCounts[strand][contigPath].insert(theContigPosition);

						// the number of matches can not exceed the length
						#ifdef ASSERT
						assert((int)m_contigCounts[strand][contigPath].size()<= m_numberOfKmers);
						#endif

						// mark it as utilised for the current sequence position
						m_observedPaths[sequencePosition].insert(contigPath);
	
						// we don't care if the vertex is repeated
						// for now.
					}
				}

				#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
				if(m_numberOfKmers%1000==0)
					cout<<"Received coverage position = "<<m_numberOfKmers<<" val= "<<coverage<<endl;
				#endif
			
				if(false && m_numberOfKmers%10000==0 && m_numberOfKmers > 0){
					cout<<"Rank "<<m_parameters->getRank()<<" processing sequence "<<m_globalSequenceIterator;
					cout<<" ProcessedKmers= "<<m_numberOfKmers<<endl;
				}
			}

			if(false){ // this is a repeated vertex, we need to fetch more stuff
				// not implemented yet
				/*
				Message aMessage(message2,count,source,RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY,
					m_parameters->getRank());
				m_outbox->push_back(aMessage);
				*/

			}else{
				m_requestedCoverage=false;
		
				// clear observed contigs for future sequence positions
				m_observedPaths.clear();
			}

		// we processed all the k-mers
		// now we need to flush the remaining half-full buffers
		// this section is now used if 
		// force=true 
		// in the above code
		}else if(m_pendingMessages==0 && !m_bufferedData.isEmpty()){

			m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_GET_COVERAGE_AND_PATHS,m_outboxAllocator,
				m_outbox,m_parameters->getRank());
	
			m_requestedCoverage=true;
		}
	}
}

int Searcher::getAbundanceWriter(int directory){

	/* things are distributed here too */
	return directory % m_parameters->getSize();
}

int Searcher::getWriter(int directory){

	return MASTER_RANK;

	/* things are distributed here too */
	// TODO: distribute the writing too...
	//return directory % m_parameters->getSize();
}

/**
 * create trees
 * actually, only master creates trees
 * but all gather directory names.
 */
void Searcher::createTrees(){

	bool lazy=true;

	ofstream directoriesFile;

	if(m_parameters->getRank()==MASTER_RANK){
		ostringstream directory6;
		directory6<<m_parameters->getPrefix()<<"/BiologicalAbundances/_Directories.tsv";
	
		directoriesFile.open(directory6.str().c_str());
		directoriesFile<<"#Directory	DirectoryName	Files"<<endl;
	}

	for(int i=0;i<m_searchDirectories_size;i++){

		string baseName=getDirectoryBaseName(i);

		ostringstream directory2;
		directory2<<m_parameters->getPrefix()<<"/BiologicalAbundances/"<<baseName;
		string directory2Str=directory2.str();

		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"create dir "<<directory2Str<<endl;
		#endif

		ostringstream fileForFiles;
		fileForFiles<<directory2Str<<"/_Files.tsv";

		ofstream files;

		if(m_parameters->getRank()==MASTER_RANK){
			createDirectory(directory2Str.c_str());

			files.open(fileForFiles.str().c_str());

			files<<"#FileNumber	FileName	Sequences"<<endl;

			directoriesFile<<i<<"	"<<baseName<<"	"<<m_searchDirectories[i].getSize()<<endl;

		}

		m_directoryNames.push_back(baseName);

		vector<string> directories;

		for(int j=0;j<(int)m_searchDirectories[i].getSize();j++){

			string theFile=getFileBaseName(i,j);

			ostringstream directory3;
			directory3<<m_parameters->getPrefix()<<"/BiologicalAbundances/"<<baseName<<"/"<<theFile;
			string directory3Str=directory3.str();

			#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
			cout<<"file is "<<*file<<endl;
			cout<<"Create dir "<<directory3Str<<endl;
			#endif

			if(!lazy && m_parameters->getRank()==MASTER_RANK)
				createDirectory(directory3Str.c_str());

			directories.push_back(theFile);

			int numberOfSequences=m_searchDirectories[i].getCount(j);

			if(m_parameters->getRank()==MASTER_RANK){
				files<<j<<"	"<<theFile<<"	"<<numberOfSequences<<endl;

			}
		}

		m_fileNames.push_back(directories);

		if(m_parameters->getRank()==MASTER_RANK){
			files.close();
		}
	}

	if(m_parameters->getRank()==MASTER_RANK){
		directoriesFile.close();
	}
}

string Searcher::getFileBaseName(int i,int j){
			
	string*file=m_searchDirectories[i].getFileName(j);

	int theLength=file->length();
	// .fasta is 6

	#ifdef ASSERT
	assert(theLength>0);
	#endif

	//cout<<"code 5"<<endl;
	string theFile=file->substr(0,theLength-6);

	return theFile;
}

string Searcher::getBaseName(string a){
	// absolute path:
	// /pubseq/sra/RayKmerSearchStuff/2011-12-23/Bacteria-Genomes
	//   returns Bacteria-Genomes
	//
	// absolute path
	//   /Data
	//     returns Data
	//
	// relative path:
	// dragonborn/dna-sequences
	//   returns dna-sequences
	//
	// relative path:
	//
	// LocalDatabase
	//   returns LocalDatabase
	//
	// LocalDatabase/
	//   returns LocalDatabase
	//
	// LocalDatabase//
	//   returns LocalDatabase

	int theLength=a.length();
	int lastPosition=theLength-1;

	// remove trailing slashes
	while(lastPosition>0 && a[lastPosition]=='/'){
		lastPosition--;
	}

	#ifdef ASSERT
	assert(lastPosition>=0);
	assert(lastPosition!=0);
	assert(a[lastPosition]!='/');
	#endif

	// only keep the base name 
	// find a slash, if any, before lastPosition
	// if there are no slash, then the lastSlash will be
	// 0
	int lastSlash=lastPosition;

	// find the last slash
	while(lastSlash>0 && a[lastSlash]!='/'){
		lastSlash--;
	}

	// at this point, lastSlash may point
	// to a slash
	// if not, it is 0
	// note that it can be 0 and point to a
	// slash
	
	// skip the slash, if any
	if(a[lastSlash]=='/'){
		lastSlash++;
	}else{
		#ifdef ASSERT
		assert(lastSlash==0);
		#endif
	}
	
	// at this point, we have 2 positions
	// from lastSlash to lastPosition inclusively,
	// there is no slash.
	#ifdef ASSERT
	for(int i=lastSlash;i<=lastPosition;i++){
		if(a[i]=='/'){
			cout<<"Input= "<<a<<" lastSlash= "<<lastSlash<<" lastPosition="<<lastPosition<<endl;
		}

		assert(a[i]!='/');
	}
	#endif

	#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
	cout<<"Input= "<<a<<" lastSlash= "<<lastSlash<<" lastPosition="<<lastPosition<<endl;
	cout<<"code 3"<<endl;

	#endif

	int count=lastPosition-lastSlash+1;

	#ifdef ASSERT
	assert(count>0);
	#endif

	return a.substr(lastSlash,count);
}

bool Searcher::isFileOwner(int globalFileIterator){
	return globalFileIterator%m_parameters->getSize()==m_parameters->getRank();
}

void Searcher::printDirectoryStart(){
	if(! (m_directoryIterator< m_searchDirectories_size))
		return;

	cout<<endl;
	cout<<"Rank "<<m_parameters->getRank()<<" starting to process "<<m_directoryNames[m_directoryIterator]<<endl;
}

void Searcher::showProcessedKmers(){
	if(m_kmersProcessed < m_lastPrinted + PROCESSED_PERIOD){
		return;
	}

	if(m_directoryIterator < m_searchDirectories_size &&
  		m_fileIterator < m_searchDirectories[m_directoryIterator].getSize()
		&& m_sequenceIterator < m_searchDirectories[m_directoryIterator].getCount(m_fileIterator)){

		cout<<"Rank "<<m_parameters->getRank()<<" biological abundances ";
		cout<<m_globalSequenceIterator<<" ["<<m_directoryIterator+1;
		cout<<"/"<<m_searchDirectories_size<<"] ["<<m_fileIterator+1<<"/";
		cout<<m_searchDirectories[m_directoryIterator].getSize()<<"] ["<<m_sequenceIterator+1;
		cout<<"/"<<m_searchDirectories[m_directoryIterator].getCount(m_fileIterator)<<"]"<<endl;
	}

	cout<<"Rank "<<m_parameters->getRank()<<" "<<SLAVE_MODES[m_switchMan->getSlaveMode()]<<" processed files: "<<m_processedFiles<<"/"<<m_filesToProcess<<endl;
	cout<<"Rank "<<m_parameters->getRank()<<" "<<SLAVE_MODES[m_switchMan->getSlaveMode()]<<" processed sequences in file: "<<m_sequenceIterator<<"/"<<m_sequencesToProcessInFile<<endl;
	cout<<"Rank "<<m_parameters->getRank()<<" "<<SLAVE_MODES[m_switchMan->getSlaveMode()]<<" total processed sequences: "<<m_processedSequences-1<<"/"<<m_sequencesToProcess<<endl;
	cout<<"Rank "<<m_parameters->getRank()<<" "<<SLAVE_MODES[m_switchMan->getSlaveMode()]<<" processed k-mers for current sequence: "<<m_numberOfKmers<<"/"<<m_currentLength<<endl;
	cout<<"Rank "<<m_parameters->getRank()<<" "<<SLAVE_MODES[m_switchMan->getSlaveMode()]<<" total processed k-mers: "<<m_kmersProcessed<<endl;

	m_derivative.addX(m_kmersProcessed);

	// display the speed obtained.
	m_derivative.printStatus(SLAVE_MODES[m_switchMan->getSlaveMode()],
			m_switchMan->getSlaveMode());

	if(m_switchMan->getSlaveMode()==RAY_SLAVE_MODE_ADD_COLORS){
		

		m_colorSet.printSummary(&cout);
		//m_colorSet.printColors();
	}

	m_lastPrinted=m_kmersProcessed;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
	}

}

void Searcher::call_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS(Message*message){

	#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
	cout<<endl;
	cout<<"call_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS"<<endl;
	#endif

	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();

	// reply buffer
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*sizeof(uint64_t));

	int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_COVERAGE_AND_PATHS);

	// the message contains:
	//  a k-mer
	//  sequencePosition
	//  a coverage value
	//  a count
	//  an index
	//  a total
	//  a list of pairs
	int pathsThatCanBePacked=(period-KMER_U64_ARRAY_SIZE-1-1-1-3)/3;

	#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
	cout<<"Meta: count= "<<count<<" period= "<<period<<endl;
	cout<<"Capacity: "<<pathsThatCanBePacked<<" paths."<<endl;
	#endif

	#ifdef ASSERT
	assert(count%period==0);
	#endif

	for(int i=0;i<count;i+=period){

		// unpack the k-mer from the message buffer
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(incoming,&bufferPosition);

		Vertex*node=m_subgraph->find(&vertex);

		int sequencePosition=incoming[bufferPosition++];

		#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
		cout<<"Sequence position: "<<sequencePosition<<endl;
		#endif

		bufferPosition++;// skip the coverage

		bufferPosition++;// skip the uniqueness
		bufferPosition++;// skip the count
		int pathIndex=incoming[bufferPosition++]; // get the index

		#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
		cout<<"Will use index "<<pathIndex<<endl;
		#endif

		bufferPosition++; // skip the total

		// if it is not there, then it has a coverage of 0
		int coverage=0;

		if(node!=NULL){
			coverage=node->getCoverage(&vertex);
			
			#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
			cout<<"Not NULL, coverage= "<<coverage<<endl;
			#endif
		}

		int outputBufferPosition=i;
		vertex.pack(message2,&outputBufferPosition);// pack the k-mer
		message2[outputBufferPosition++]=sequencePosition; // pack the sequence position, we will need it
		message2[outputBufferPosition++]=coverage;

		bool uniqueness=false;

		if(node!=NULL){
			// check the colors
			VirtualKmerColorHandle color=node->getVirtualColor();
			set<PhysicalKmerColor>*physicalColors=m_colorSet.getPhysicalColors(color);

			// verify that there is only one color in each namespace
			
			uniqueness=true;

			map<int,int> counts;

			for(set<PhysicalKmerColor>::iterator j=physicalColors->begin();
				j!=physicalColors->end();j++){
				PhysicalKmerColor physicalColor=*j;
		
				int nameSpace=physicalColor/COLOR_NAMESPACE;
			
				counts[nameSpace]++;
				if(counts[nameSpace]>1){
					uniqueness=false;
					break;
				}
			}
		}

		// store the uniqueness of the color
		message2[outputBufferPosition++]=uniqueness;

		int nicelyAssembledPosition=outputBufferPosition++;

		message2[nicelyAssembledPosition]=false;

		// store the place where the count will be deposited
		int positionForCount=outputBufferPosition++;
		int positionForIndex=outputBufferPosition++;
		int positionForTotal=outputBufferPosition++;

		// set things to 0 for now
		message2[positionForCount]=0;
		message2[positionForIndex]=0;
		message2[positionForTotal]=0;


		if(node!=NULL){
			Kmer lowerKey=node->m_lowerKey;
			
			bool weHaveLowerKey=lowerKey.isEqual(&vertex);

			vector<Direction> paths;

			/* here, we just want to find a path with
 			* a good progression */

			Direction*a=node->m_directions;

			bool nicelyAssembled=false;

			while(a!=NULL){
				paths.push_back(*a);

				int progression=a->getProgression();

				//cout<<"PROGRESSION "<<progression<<endl;

				if(progression>= CONFIG_NICELY_ASSEMBLED_KMER_POSITION){
					nicelyAssembled=true;
				}

				a=a->getNext();
			}

			/* update the flag for nicely assembled */
			message2[nicelyAssembledPosition]=nicelyAssembled;

			#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
			cout<<"paths found: "<<paths.size()<<endl;
			#endif

			// remove duplicates
			set<uint64_t> contigPaths;

			int processed=0;

			while(pathIndex<(int)paths.size()){

				#ifdef ASSERT
				assert(pathIndex<(int)paths.size());
				#endif

				uint64_t path=paths[pathIndex].getWave();
				int contigPosition=paths[pathIndex].getProgression();
				char strand='F';

				if(weHaveLowerKey!=paths[pathIndex].isLower())
					strand='R';

				#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
				cout<<"Index= "<<pathIndex<<" Storing Contig Data "<<path<<" "<<contigPosition<<endl;
				#endif

				//store the contig path
				message2[outputBufferPosition++]=path; // path identifier
				message2[outputBufferPosition++]=contigPosition; // path position
				message2[outputBufferPosition++]=strand;

				processed++;
				
				pathIndex++;

				// we just don't send them all because
				// repeats are repeated
				if(processed==pathsThatCanBePacked)
					break;

			}
	
			message2[positionForCount]=processed;
			message2[positionForIndex]=pathIndex;
			message2[positionForTotal]=paths.size();

			#ifdef CONFIG_CONTIG_IDENTITY_VERBOSE
			cout<<"Processed: "<<processed<<" NewIndex: "<<pathIndex<<" Total: "<<paths.size()<<endl;
			#endif
		}
	}

	#ifdef ASSERT
	assert(count*sizeof(uint64_t)<=MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif

	Message aMessage(message2,count,source,RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY,
		m_parameters->getRank());
	m_outbox->push_back(aMessage);
}

void Searcher::call_RAY_MPI_TAG_ADD_KMER_COLOR(Message*message){
	// add the color
	
	#ifdef CONFIG_DEBUG_COLORS
	cout<<"Sending reply to "<<message->getSource()<<" slaveMode= "<<SLAVE_MODES[m_switchMan->getSlaveMode()]<<endl;
	#endif

	int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ADD_KMER_COLOR);
	uint64_t*buffer=message->getBuffer();
	int count=message->getCount();

	for(int i=0;i<count;i+=period){
		Kmer kmer;
		int bufferPosition=i;

		kmer.unpack(buffer,&bufferPosition);


		Vertex*node=m_subgraph->find(&kmer);

		// the k-mer does not exist
		if(node==NULL){
			//cout<<"kmer does not exist "<<endl;
			continue;
		}

		PhysicalKmerColor color=buffer[bufferPosition++];

		PhysicalKmerColor colorForPhylogeny=buffer[bufferPosition++];

		addColorToKmer(node,color);

		/* don't add it if it is the same */
		if(colorForPhylogeny != color){
			addColorToKmer(node,colorForPhylogeny);
		}

	}

	// send reply
	m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),message->getSource(),RAY_MPI_TAG_ADD_KMER_COLOR_REPLY);
}

void Searcher::addColorToKmer(Vertex*node,PhysicalKmerColor color){
	//cout<<"kmer exists"<<endl;

	VirtualKmerColorHandle virtualColorHandle=node->getVirtualColor();

	// the physical color is already there
	// we don't need to add it...
	if(m_colorSet.virtualColorHasPhysicalColor(virtualColorHandle,color)){
		return;
	}

	// get a virtual color with the requested physical colors
	// it will fetch an already existing virtual color, if any
	// otherwise, it will create a new one.
	VirtualKmerColorHandle newVirtualColor=m_colorSet.getVirtualColorFrom(virtualColorHandle,color);

	node->setVirtualColor(newVirtualColor);

	m_colorSet.incrementReferences(newVirtualColor);
	m_colorSet.decrementReferences(virtualColorHandle);

	#ifdef ASSERT
	assert(m_colorSet.virtualColorHasPhysicalColor(newVirtualColor,color));

	// maybe this color was purged..
	//assert(m_colorSet.getNumberOfPhysicalColors(virtualColorHandle)+1 == m_colorSet.getNumberOfPhysicalColors(newVirtualColor));
	
	assert(m_colorSet.getNumberOfReferences(newVirtualColor)>=1);
	assert(m_colorSet.getNumberOfReferences(virtualColorHandle)>=0);
	#endif

}

void Searcher::call_RAY_MASTER_MODE_ADD_COLORS(){
	if(!m_startedColors){
		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());
		m_startedColors=true;
	}else if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();

		m_timePrinter->printElapsedTime("Graph coloring");

		cout<<endl;
	}
}

void Searcher::call_RAY_SLAVE_MODE_ADD_COLORS(){

	// Process virtual messages
	m_virtualCommunicator->forceFlush();
	m_virtualCommunicator->processInbox(&m_activeWorkers);

	// this contains a list of workers that received something
	// here, we only have one worker.
	m_activeWorkers.clear();

	if(!m_colorSequenceKmersSlaveStarted){

		createTrees();

		m_directoryIterator=0;
		m_fileIterator=0;
		m_globalFileIterator=0;
		m_sequenceIterator=0;
		m_globalSequenceIterator=0;

		m_colorSequenceKmersSlaveStarted=true;
		m_createdSequenceReader=false;

		m_kmerLength=m_parameters->getWordSize();

		printDirectoryStart();

		m_pendingMessages=0;

		#ifdef CONFIG_DEBUG_COLORS
		cout<<"m_colorSequenceKmersSlaveStarted := true"<<endl;
		#endif

		#ifdef ASSERT
		assert(m_bufferedData.isEmpty());
		#endif

		m_derivative.clear();

		m_lastPrinted=0;
		m_kmersProcessed=0;

		m_derivative.addX(m_kmersProcessed);


		// count the files that will be processed.
		// and also the sequences

		m_processedFiles=0;
		m_processedSequences=0;

		m_sequencesToProcess=0;
		m_filesToProcess=0;

		int globalFile=0;
		for(int i=0;i<m_searchDirectories_size;i++){
			for(int file=0;file<(int)m_searchDirectories[i].getSize();file++){
				if(isFileOwner(globalFile)){
					m_filesToProcess++;
					m_sequencesToProcess+= m_searchDirectories[i].getCount(file);
				}
				globalFile++;
			}
		}

		cout<<"Rank "<<m_parameters->getRank()<<" will add colors, "<<m_sequencesToProcess<<" sequences in "<<m_filesToProcess<<" files to process"<<endl;

	// we have a response
	}else if(m_pendingMessages > 0 &&
			m_inbox->hasMessage(RAY_MPI_TAG_ADD_KMER_COLOR_REPLY)){

		#ifdef ASSERT
		assert(m_pendingMessages==1);
		#endif

		#ifdef CONFIG_DEBUG_COLORS
		cout<<"received RAY_MPI_TAG_ADD_KMER_COLOR_REPLY, pending= "<<m_pendingMessages<<endl;
		#endif

		m_pendingMessages--;

		#ifdef ASSERT
		assert(m_pendingMessages==0);
		#endif


		#ifdef CONFIG_DEBUG_COLORS
		cout<<"now => received RAY_MPI_TAG_ADD_KMER_COLOR_REPLY, pending= "<<m_pendingMessages<<endl;
		#endif

	// all directories were processed
	}else if(m_directoryIterator==m_searchDirectories_size){
	
		showProcessedKmers();

		m_bufferedData.showStatistics(m_parameters->getRank());

		// this kills the batman
		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());

		#ifdef CONFIG_DEBUG_COLORS
		cout<<"Finished."<<endl;
		#endif


	// all files in a directory were processed
	}else if(m_fileIterator==(int)m_searchDirectories[m_directoryIterator].getSize()){
	
		cout<<"Rank "<<m_parameters->getRank()<<" has colored its entries from "<<m_directoryNames[m_directoryIterator]<<endl;
		cout<<endl;

		m_directoryIterator++;
		m_fileIterator=0;
		m_sequenceIterator=0;

		printDirectoryStart();

	// this sequence is not owned by me
	}else if(!isFileOwner(m_globalFileIterator)){

		m_globalSequenceIterator+=m_searchDirectories[m_directoryIterator].getCount(m_fileIterator);
		
		showProcessedKmers();

		// skip the file
		// ownership is on a per-file basis
		m_fileIterator++;
		m_globalFileIterator++;
		m_sequenceIterator=0;

		#ifdef CONFIG_DEBUG_COLORS
		cout<<"is not owner"<<endl;
		#endif

	// all sequences in a file were processed
	}else if(m_sequenceIterator==m_searchDirectories[m_directoryIterator].getCount(m_fileIterator) ){

		if(m_pendingMessages>0){
			// wait for it
			//


		// we processed all the k-mers
		// now we need to flush the remaining half-full buffers
		// this section is now used if 
		// force=true 
		// in the above code
		}else if(!m_bufferedData.isEmpty()){

			#ifdef ASSERT
			assert(m_pendingMessages==0);
			#endif

			m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_ADD_KMER_COLOR,m_outboxAllocator,
				m_outbox,m_parameters->getRank());

			#ifdef ASSERT
			assert(m_pendingMessages>0);
			#endif

		// finished the file
		}else if(m_pendingMessages==0){

			#ifdef ASSERT
			assert(m_bufferedData.isEmpty());
			#endif

			m_fileIterator++;
			m_globalFileIterator++;
			m_sequenceIterator=0;

			#ifdef CONFIG_DEBUG_COLORS
			cout<<" file processed.."<<endl;
			#endif

			m_processedFiles++;
		}

	// start a sequence
	}else if(!m_createdSequenceReader){
		// initiate the reader I guess
	
		#ifdef CONFIG_DEBUG_COLORS
		cout<<"Creating sequence reader."<<endl;
		#endif

		m_searchDirectories[m_directoryIterator].createSequenceReader(m_fileIterator,m_sequenceIterator,m_parameters->getWordSize());

		m_currentLength=m_searchDirectories[m_directoryIterator].getCurrentSequenceLengthInKmers();

		m_numberOfKmers=0;
		m_createdSequenceReader=true;

		// check if we need to color the graph 
		// for this sequence...

		m_mustAddColors=true;

		// maximum value for a uint64_t:
		// 18446744073709551615
		// xxxx000yyyyyyyyyyyyy
		//    10000000000000000

		m_color= m_globalSequenceIterator + COLOR_NAMESPACE * (m_directoryIterator);

		/* unique identifiers have their own namespace */

		m_identifier=m_color;

		if(m_searchDirectories[m_directoryIterator].hasCurrentSequenceIdentifier()){
			uint64_t theIdentifier=m_searchDirectories[m_directoryIterator].getCurrentSequenceIdentifier();

			uint64_t nameSpace=PHYLOGENY_NAMESPACE;
			nameSpace*= COLOR_NAMESPACE;
			m_identifier=theIdentifier + nameSpace;
		}


		#ifdef DEBUG_PHYLOGENY
		cout<<"[phylogeny] identifier= "<<m_identifier<<endl;
		#endif

		#ifdef ASSERT
		//assert(m_pendingMessages==0); not used anymore
		#endif

		m_sequencesToProcessInFile=m_searchDirectories[m_directoryIterator].getCount(m_fileIterator);

		m_processedSequences++;

	// compute abundances
	}else if(m_createdSequenceReader){

		// the current sequence has been processed
		if( !m_searchDirectories[m_directoryIterator].hasNextKmer(m_kmerLength)){
			

			#ifdef ASSERT
			assert(m_directoryIterator<m_searchDirectories_size);
			assert(m_fileIterator<m_searchDirectories[m_directoryIterator].getSize());
			#endif

			m_sequenceIterator++;
			m_globalSequenceIterator++;
			m_createdSequenceReader=false;

			#ifdef CONFIG_DEBUG_COLORS
			cout<<"Finished sequence"<<endl;
			#endif

		// k-mers are available
		// pull data from the sequence
		// and throw messages onto the network
		}else if(m_pendingMessages==0 ){
	
			bool force=CONFIG_FORCE_VALUE_FOR_MAXIMUM_SPEED;

			// pull k-mers from the sequence and fill buffers
			// if one of the buffer if full,
			// flush it and return and wait for a response

			bool gatheringKmers=true;

			#ifdef ASSERT
			assert(m_pendingMessages==0);
			#endif

			#ifdef CONFIG_DEBUG_COLORS
			cout<<"Sending colors"<<endl;
			#endif

			while(gatheringKmers && m_searchDirectories[m_directoryIterator].hasNextKmer(m_kmerLength)){

				Kmer kmer;
				m_searchDirectories[m_directoryIterator].getNextKmer(m_kmerLength,&kmer);
				m_searchDirectories[m_directoryIterator].iterateToNextKmer();

				//cout<<"coloring getNextKmer"<<endl;

				// if the k-mer contains non-standard character,
				// skip it

				if( m_searchDirectories[m_directoryIterator].kmerContainsN()){

					m_numberOfKmers++; // the number of k-mers for the sequence

					//cout<<"has N, skipping."<<endl;
					m_kmersProcessed++; // the total number of k-mers processed

					showProcessedKmers();

					continue;
				}

				int rankToFlush=m_parameters->_vertexRank(&kmer);

				int added=0;
				// pack the k-mer
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedData.addAt(rankToFlush,kmer.getU64(i));
					added++;
				}

				uint64_t color=m_color;

				// add the color
				m_bufferedData.addAt(rankToFlush,color);
				added++;

				/* also add the GenBank identifier for phylogeny analyses */

				uint64_t identifier=m_identifier;
				m_bufferedData.addAt(rankToFlush,identifier);
				added++;

				// this little guy have to be right after iterateToNextKmer()
				// otherwise, it is buggy.
				m_numberOfKmers++;
				m_kmersProcessed++;

				showProcessedKmers();

				int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ADD_KMER_COLOR);

				#ifdef ASSERT
				assert(period == added);
				#endif

				// force flush the message
				if(m_bufferedData.flush(rankToFlush,period,
					RAY_MPI_TAG_ADD_KMER_COLOR,m_outboxAllocator,m_outbox,
					m_parameters->getRank(),force)){

					m_pendingMessages++;
					gatheringKmers=false;

					#ifdef CONFIG_DEBUG_COLORS
					cout<<"flushed a message."<<endl;
					#endif

					#ifdef ASSERT
					assert(m_pendingMessages>0);
					assert(m_pendingMessages==1);
					#endif
				}
			}

		}
	
	}

}

void Searcher::dumpDistributions(){
	string directoryName=getDirectoryBaseName(m_directoryIterator);
	string fileName=getFileBaseName(m_directoryIterator,m_fileIterator);

	m_writer.write(m_directoryIterator,m_fileIterator,m_sequenceIterator,
		&m_coverageDistribution,&m_coloredCoverageDistribution,
		&m_coloredAssembledCoverageDistribution,directoryName.c_str(),
		fileName.c_str());
}

int Searcher::getDistributionMode(map<int,uint64_t>*distribution){

	int mode=0;
	int modeCount=0;

	for(map<int,uint64_t>::iterator i=distribution->begin();
		i!=distribution->end();i++){

		int count=i->second;
		int coverage=i->first;

		if(count>modeCount){
			mode=coverage;
			modeCount=count;
		}
	}

	return mode;
}

void Searcher::call_RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY(Message*message){

	// unpack stuff
	
	int bufferPosition=0;
	uint64_t*buffer=message->getBuffer();



	int directoryIterator=buffer[bufferPosition++];
	int fileIterator=buffer[bufferPosition++];
	int sequenceIterator=buffer[bufferPosition++];

	//cout<<"Received sequenceIterator= "<<sequenceIterator<<endl;

	int numberOfKmers=buffer[bufferPosition++];

	int matches=buffer[bufferPosition++];
	int mode=buffer[bufferPosition++];

	int coloredMatches=buffer[bufferPosition++];
	int coloredMode=buffer[bufferPosition++];

	int coloredAssembledMatches=buffer[bufferPosition++];
	int coloredAssembledMode=buffer[bufferPosition++];

	bool entryIsWorthy=buffer[bufferPosition++];

	double qualityColoredVsAll=(buffer[bufferPosition++]+0.0)/CONFIG_DOUBLE_PRECISION;
	double qualityAssembledVsAll=(buffer[bufferPosition++]+0.0)/CONFIG_DOUBLE_PRECISION;
	double qualityAssembledVsColored=(buffer[bufferPosition++]+0.0)/CONFIG_DOUBLE_PRECISION;

	bool hasPeak=(bool)buffer[bufferPosition++];
	bool hasHighFrequency=(bool)buffer[bufferPosition++];

	char*sequenceNameFromMessage=(char*)(buffer+bufferPosition);


	// open the file
	// don't open it if there are 0 matches
	if(entryIsWorthy && m_arrayOfFiles.count(directoryIterator)==0) {
		
		string baseName=getDirectoryBaseName(directoryIterator);

		ostringstream fileName;
		fileName<<m_parameters->getPrefix()<<"/BiologicalAbundances/";
		fileName<<baseName<<"/";
		// add the file name without the .fasta
		fileName<<"SequenceAbundancesRaw.xml";

		m_arrayOfFiles[directoryIterator]=fopen(fileName.str().c_str(),"w");
		

		// also create a shorter version of the file
		ostringstream fileName2;
		fileName2<<m_parameters->getPrefix()<<"/BiologicalAbundances/";
		fileName2<<baseName<<"/";
		// add the file name without the .fasta
		fileName2<<"SequenceAbundances.xml";


		m_arrayOfFilesShort[directoryIterator]=fopen(fileName2.str().c_str(),"w");


		#ifdef ASSERT
		assert(m_activeFiles>=0); // it is 0 or 1 or something else
		#endif

		m_activeFiles++;

		#ifdef ASSERT
		assert(m_activeFiles>=1); 
		#endif

		ostringstream content88;
		content88<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<endl;

		content88<<"<root><sample>"<<m_parameters->getSampleName()<<"</sample><searchDirectory>"<<baseName<<"</searchDirectory>"<<endl;
	
		content88<<"<totalAssembledKmerObservations>"<<m_totalNumberOfKmerObservations;
		content88<<"</totalAssembledKmerObservations>"<<endl;

		fprintf(m_arrayOfFilesShort[directoryIterator],
			"%s",content88.str().c_str());

		fprintf(m_arrayOfFiles[directoryIterator],
			"%s",content88.str().c_str());

		cout<<"Opened "<<fileName.str()<<", active file descriptors: "<<m_activeFiles<<endl;
	}


	// write the file if there are not 0 matches
	if(entryIsWorthy ){

		ostringstream content;
		
		string sequenceName=sequenceNameFromMessage;

		double ratio=(0.0+matches)/numberOfKmers;

		ostringstream header;
		header<<"#Category	Sequence number	Sequence name	K-mer length	Length in k-mers";
		
		header<<"	K-mer matches	Ratio	Mode k-mer coverage depth";

		header<<"	Uniquely colored k-mer matches";
		header<<"	Ratio	Mode uniquely colored k-mer coverage depth";

		header<<"	Uniquely colored assembled k-mer matches";
		header<<"	Ratio	Mode uniquely colored assembled k-mer coverage depth";

		header<<"	Quality1	Quality2	Quality3";
		header<<"	Has peak ?	Has high frequency ?";
		header<<"	Demultiplexed k-mer observations";
		header<<"	Total	Proportion";

		header<<endl;

		
		content<<"<entry><file>"<<m_fileNames[directoryIterator][fileIterator]<<"</file>"<<endl;
		content<<"<sequence>"<<sequenceIterator<<"</sequence><name>"<<sequenceName<<"</name>"<<endl;
		content<<"<kmerLength>"<<m_parameters->getWordSize();
		
		content<<"</kmerLength><lengthInKmers>"<<numberOfKmers<<"</lengthInKmers>"<<endl;

		content<<"<raw><kmerMatches>"<<matches<<"</kmerMatches><proportion>"<<ratio<<"</proportion><modeKmerCoverage>";
		content<<mode<<"</modeKmerCoverage></raw>"<<endl;

		double coloredRatio=coloredMatches;

		if(numberOfKmers!=0)
			coloredRatio/=numberOfKmers;

		content<<"<uniquelyColored><kmerMatches>"<<coloredMatches<<"</kmerMatches><proportion>"<<coloredRatio<<"</proportion>";
		content<<"<modeKmerCoverage>"<<coloredMode<<"</modeKmerCoverage></uniquelyColored>"<<endl;

		double coloredAssembledRatio=coloredAssembledMatches;

		if(numberOfKmers!=0)
			coloredAssembledRatio/=numberOfKmers;

		content<<"<uniquelyColoredAndAssembled><kmerMatches>"<<coloredAssembledMatches<<"</kmerMatches><proportion>"<<coloredAssembledRatio;
		content<<"</proportion><modeKmerCoverage>"<<coloredAssembledMode<<"</modeKmerCoverage></uniquelyColoredAndAssembled>"<<endl;

		content<<"<qualityControl><correlationColoredVsRaw>"<<qualityColoredVsAll<<"</correlationColoredVsRaw>";
		content<<"<correlationAssembledVsRaw>"<<qualityAssembledVsAll<<"</correlationAssembledVsRaw>";
		content<<"<correlationAssembledVsColored>"<<qualityAssembledVsColored<<"</correlationAssembledVsColored>";

		content<<"<hasPeak>"<<hasPeak<<"</hasPeak><hasHighFrequency>"<<hasHighFrequency<<"</hasHighFrequency></qualityControl>"<<endl;

		uint64_t demultiplexedObservations=0;

		if(hasPeak || hasHighFrequency){
			demultiplexedObservations=coloredAssembledMode*matches;
		}

		if(coloredAssembledMode >= 10* coloredMode){ // this means that the entry is invalid usually
			demultiplexedObservations=0;
		}

		content<<"<demultiplexedKmerObservations>"<<demultiplexedObservations<<"</demultiplexedKmerObservations>";

		double proportion=demultiplexedObservations;
		if(m_totalNumberOfKmerObservations!=0)
			proportion/=m_totalNumberOfKmerObservations;

		content<<"<proportion>"<<proportion<<"</proportion></entry>"<<endl;

		#ifdef ASSERT
		assert(m_arrayOfFiles.count(directoryIterator)>0);
		assert(m_arrayOfFilesShort.count(directoryIterator)>0);
		#endif

		fprintf(m_arrayOfFiles[directoryIterator],
			"%s",content.str().c_str());


		// also write a shorter version

		if(demultiplexedObservations>0){


			ostringstream content2;

			content2<<"<entry><file>"<<m_fileNames[directoryIterator][fileIterator]<<"</file>"<<endl;
			content2<<"<sequence>"<<sequenceIterator<<"</sequence><name>"<<sequenceName<<"</name>"<<endl;

			content2<<"<demultiplexedKmerObservations>"<<demultiplexedObservations<<"</demultiplexedKmerObservations>";
			content2<<"<proportion>"<<proportion<<"</proportion></entry>";
			content2<<endl;

			fprintf(m_arrayOfFilesShort[directoryIterator],
				"%s",content2.str().c_str());
		}
	}

	// send a reply
	//
	m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),
		message->getSource(),RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY);

}

void Searcher::call_RAY_MPI_TAG_CONTIG_IDENTIFICATION(Message*message){

       	uint64_t*messageBuffer=message->getBuffer();

       	// process the message
       	int bufferPosition=0;
       	uint64_t contig=messageBuffer[bufferPosition++];

       	char strand=messageBuffer[bufferPosition++];

       	#ifdef ASSERT
       	assert(m_contigLengths.count(contig)>0);
       	#endif

       	int kmerLength=m_parameters->getWordSize();
       	int contigLength=m_contigLengths[contig];
       	int count=messageBuffer[bufferPosition++];

       	#ifdef ASSERT
       	assert(kmerLength>0);
       	assert(contigLength>0);
       	assert(count>0);
       	#endif

       	int directoryIterator=messageBuffer[bufferPosition++];
       	int fileIterator=messageBuffer[bufferPosition++];

       	#ifdef ASSERT
       	assert(directoryIterator<m_searchDirectories_size);
       	assert(fileIterator<m_searchDirectories[directoryIterator].getSize());
       	#endif

       	string category=m_fileNames[directoryIterator][fileIterator];

       	int sequenceIterator=messageBuffer[bufferPosition++];

       	int numberOfKmers=messageBuffer[bufferPosition++];

       	// the number of matches can not be greater than
       	// the number of k-mers in the query sequence
       	// otherwise, it does not make sense
       	#ifdef ASSERT
       	if(count> numberOfKmers){
       		cout<<"Error: "<<count<<" k-mers observed, but the contig has only "<<numberOfKmers<<endl;
       		cout<<"Sequence= "<<sequenceIterator<<endl;
       	}
       	assert(count <= numberOfKmers);
       	#endif

       	char*sequenceName=(char*) (messageBuffer+bufferPosition);

       	// contigLength can not be 0 anyway
       	double ratio=(0.0+count)/contigLength;

       	double sequenceRatio=count;

       	if(numberOfKmers!=0)
       		sequenceRatio/=numberOfKmers;

       	bool thresholdIsGood=false;

       	if(ratio >= CONFIG_SEARCH_THRESHOLD)
       		thresholdIsGood=true;

       	if(sequenceRatio >= CONFIG_SEARCH_THRESHOLD)
       		thresholdIsGood=true;

       	// open the file for reading
       	if(thresholdIsGood &&
       		count>0 && ( m_identificationFiles.count(directoryIterator)==0)){

       		// create an empty file for identifications
       		ostringstream identifications;

       		string*theDirectoryPath=m_searchDirectories[directoryIterator].getDirectoryName();
       		string baseName=getBaseName(*theDirectoryPath);
       		identifications<<m_parameters->getPrefix()<<"/BiologicalAbundances/";
       		identifications<<baseName<<"/ContigIdentifications.tsv";

       		m_identificationFiles[directoryIterator]=fopen(identifications.str().c_str(),"w");

       		ostringstream line;
       	
       		// push header
       		line<<"#Contig name	K-mer length	Contig length in k-mers	Contig strand	Category	";
       		line<<"Sequence number	Sequence name";
       		line<<"	Sequence length in k-mers	Matches in contig	Contig length ratio";
       		line<<"	Sequence length ratio"<<endl;

       		fprintf(m_identificationFiles[directoryIterator],"%s",line.str().c_str());
       	}

       	// write an entry in the file
       
       	if(thresholdIsGood){
       		ostringstream line;
       		line<<"contig-"<<contig<<"	"<<kmerLength<<"	"<<contigLength;
       		line<<"	"<<strand;
       		line<<"	"<<category;
       		line<<"	"<<sequenceIterator<<"	"<<sequenceName<<"	";
       		line<<numberOfKmers<<"	"<<count<<"	"<<ratio<<"	"<<sequenceRatio<<endl;

       		fprintf(m_identificationFiles[directoryIterator],"%s",line.str().c_str());
       	}

	// send a reply
	m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),
		message->getSource(),RAY_MPI_TAG_CONTIG_IDENTIFICATION_REPLY);
}

string Searcher::getDirectoryBaseName(int directoryIterator){

	string*theDirectoryPath=m_searchDirectories[directoryIterator].getDirectoryName();
	string baseName=getBaseName(*theDirectoryPath);

	return baseName;
}

uint64_t Searcher::getTotalNumberOfKmerObservations(){
	return m_totalNumberOfKmerObservations;
}

void Searcher::registerPlugin(ComputeCore*core){

	PluginHandle plugin=core->allocatePluginHandle();

	m_plugin=plugin;

	core->setPluginName(plugin,"Searcher");
	core->setPluginDescription(plugin,"Add colors in the graph and search things");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES=core->allocateMasterModeHandle(plugin);
	m_adapter_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES,&m_adapter_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES);
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES,"RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES");

	RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES=core->allocateMasterModeHandle(plugin);
	m_adapter_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES,&m_adapter_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES);
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES,"RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES");

	RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS=core->allocateMasterModeHandle(plugin);
	m_adapter_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS,&m_adapter_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS);
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS,"RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS");

	RAY_MASTER_MODE_ADD_COLORS=core->allocateMasterModeHandle(plugin);
	m_adapter_RAY_MASTER_MODE_ADD_COLORS.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_ADD_COLORS,&m_adapter_RAY_MASTER_MODE_ADD_COLORS);
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_ADD_COLORS,"RAY_MASTER_MODE_ADD_COLORS");

	RAY_MASTER_MODE_SEARCHER_CLOSE=core->allocateMasterModeHandle(plugin);
	m_adapter_RAY_MASTER_MODE_SEARCHER_CLOSE.setObject(this);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_SEARCHER_CLOSE,&m_adapter_RAY_MASTER_MODE_SEARCHER_CLOSE);
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_SEARCHER_CLOSE,"RAY_MASTER_MODE_SEARCHER_CLOSE");

	RAY_SLAVE_MODE_ADD_COLORS=core->allocateSlaveModeHandle(plugin);
	m_adapter_RAY_SLAVE_MODE_ADD_COLORS.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_ADD_COLORS,&m_adapter_RAY_SLAVE_MODE_ADD_COLORS);
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_ADD_COLORS,"RAY_SLAVE_MODE_ADD_COLORS");

	RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES=core->allocateSlaveModeHandle(plugin);
	m_adapter_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES,&m_adapter_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES);
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES,"RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES");

	RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES=core->allocateSlaveModeHandle(plugin);
	m_adapter_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES,&m_adapter_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES);
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES,"RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES");

	RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS=core->allocateSlaveModeHandle(plugin);
	m_adapter_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS,&m_adapter_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS);
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS,"RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS");

	RAY_SLAVE_MODE_SEARCHER_CLOSE=core->allocateSlaveModeHandle(plugin);
	m_adapter_RAY_SLAVE_MODE_SEARCHER_CLOSE.setObject(this);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_SEARCHER_CLOSE,&m_adapter_RAY_SLAVE_MODE_SEARCHER_CLOSE);
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_SEARCHER_CLOSE,"RAY_SLAVE_MODE_SEARCHER_CLOSE");

	RAY_MPI_TAG_GET_COVERAGE_AND_PATHS=core->allocateMessageTagHandle(plugin);
	m_adapter_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS.setObject(this);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_COVERAGE_AND_PATHS,&m_adapter_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_COVERAGE_AND_PATHS,"RAY_MPI_TAG_GET_COVERAGE_AND_PATHS");

	RAY_MPI_TAG_ADD_KMER_COLOR=core->allocateMessageTagHandle(plugin);
	m_adapter_RAY_MPI_TAG_ADD_KMER_COLOR.setObject(this);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ADD_KMER_COLOR,&m_adapter_RAY_MPI_TAG_ADD_KMER_COLOR);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ADD_KMER_COLOR,"RAY_MPI_TAG_ADD_KMER_COLOR");

	RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS,"RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS");

	RAY_MPI_TAG_SEARCH_COUNTING_DONE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCH_COUNTING_DONE,"RAY_MPI_TAG_SEARCH_COUNTING_DONE");

	RAY_MPI_TAG_SEARCH_ELEMENTS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCH_ELEMENTS,"RAY_MPI_TAG_SEARCH_ELEMENTS");

	RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY,"RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY");

	RAY_MPI_TAG_SEARCH_MASTER_COUNT=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCH_MASTER_COUNT,"RAY_MPI_TAG_SEARCH_MASTER_COUNT");

	RAY_MPI_TAG_SEARCH_MASTER_COUNT_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCH_MASTER_COUNT_REPLY,"RAY_MPI_TAG_SEARCH_MASTER_COUNT_REPLY");

	RAY_MPI_TAG_SEARCH_MASTER_SHARING_DONE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCH_MASTER_SHARING_DONE,"RAY_MPI_TAG_SEARCH_MASTER_SHARING_DONE");

	RAY_MPI_TAG_SEARCH_SHARE_COUNTS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCH_SHARE_COUNTS,"RAY_MPI_TAG_SEARCH_SHARE_COUNTS");

	RAY_MPI_TAG_SEARCHER_CLOSE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCHER_CLOSE,"RAY_MPI_TAG_SEARCHER_CLOSE");

	RAY_MPI_TAG_SEARCH_SHARING_COMPLETED=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEARCH_SHARING_COMPLETED,"RAY_MPI_TAG_SEARCH_SHARING_COMPLETED");

	RAY_MPI_TAG_CONTIG_ABUNDANCE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_CONTIG_ABUNDANCE,"RAY_MPI_TAG_CONTIG_ABUNDANCE");

	RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY,"RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY");

	RAY_MPI_TAG_CONTIG_BIOLOGICAL_ABUNDANCES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_CONTIG_BIOLOGICAL_ABUNDANCES,"RAY_MPI_TAG_CONTIG_BIOLOGICAL_ABUNDANCES");

	RAY_MPI_TAG_SEQUENCE_ABUNDANCE_FINISHED=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEQUENCE_ABUNDANCE_FINISHED,"RAY_MPI_TAG_SEQUENCE_ABUNDANCE_FINISHED");

	RAY_MPI_TAG_SEQUENCE_ABUNDANCE_YOU_CAN_GO_HOME=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEQUENCE_ABUNDANCE_YOU_CAN_GO_HOME,"RAY_MPI_TAG_SEQUENCE_ABUNDANCE_YOU_CAN_GO_HOME");

	RAY_MPI_TAG_SEQUENCE_BIOLOGICAL_ABUNDANCES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEQUENCE_BIOLOGICAL_ABUNDANCES,"RAY_MPI_TAG_SEQUENCE_BIOLOGICAL_ABUNDANCES");

	RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_REPLY,"RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_REPLY");

	RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY=core->allocateMessageTagHandle(plugin);
	m_adapter_RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY.setObject(this);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY,&m_adapter_RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY,"RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY");

	RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY,"RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY");

	RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES,"RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES");

	RAY_MPI_TAG_ADD_COLORS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ADD_COLORS,"RAY_MPI_TAG_ADD_COLORS");

	RAY_MPI_TAG_ADD_KMER_COLOR_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ADD_KMER_COLOR_REPLY,"RAY_MPI_TAG_ADD_KMER_COLOR_REPLY");

	RAY_MPI_TAG_CONTIG_IDENTIFICATION=core->allocateMessageTagHandle(plugin);
	m_adapter_RAY_MPI_TAG_CONTIG_IDENTIFICATION.setObject(this);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_CONTIG_IDENTIFICATION,&m_adapter_RAY_MPI_TAG_CONTIG_IDENTIFICATION);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_CONTIG_IDENTIFICATION,"RAY_MPI_TAG_CONTIG_IDENTIFICATION");

	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS=core->allocateMessageTagHandle(m_plugin);
	m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS.setObject(this);
	core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS,&m_adapter_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS");
	
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS_REPLY=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS_REPLY,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS_REPLY");

	RAY_MPI_TAG_CONTIG_IDENTIFICATION_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_CONTIG_IDENTIFICATION_REPLY,"RAY_MPI_TAG_CONTIG_IDENTIFICATION_REPLY");

	core->setObjectSymbol(m_plugin,m_parameters,"/RayAssembler/ObjectStore/Parameters.ray");
	core->setObjectSymbol(m_plugin,m_subgraph,"/RayAssembler/ObjectStore/deBruijnGraph_part.ray");
	core->setObjectSymbol(m_plugin,&m_colorSet,"/RayAssembler/ObjectStore/VirtualColorManagementUnit.ray");
	core->setObjectSymbol(m_plugin,m_timePrinter,"/RayAssembler/ObjectStore/Timer.ray");
	core->setObjectSymbol(m_plugin,this,"/RayAssembler/ObjectStore/plugin_Searcher.ray");

	m_totalNumberOfKmerObservations=0;
}

void Searcher::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_ADD_COLORS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ADD_COLORS");
	RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES");
	RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES");
	RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS");
	RAY_SLAVE_MODE_SEARCHER_CLOSE=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEARCHER_CLOSE");

	RAY_MASTER_MODE_ADD_COLORS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ADD_COLORS");
	RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS");
	RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES");
	RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES");
	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");
	RAY_MASTER_MODE_SEARCHER_CLOSE=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_SEARCHER_CLOSE");
	RAY_MASTER_MODE_PHYLOGENY_MAIN=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PHYLOGENY_MAIN");

	RAY_MPI_TAG_ADD_KMER_COLOR=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ADD_KMER_COLOR");
	RAY_MPI_TAG_ADD_KMER_COLOR_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ADD_KMER_COLOR_REPLY");
	RAY_MPI_TAG_CONTIG_ABUNDANCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_ABUNDANCE");
	RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY");
	RAY_MPI_TAG_CONTIG_IDENTIFICATION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_IDENTIFICATION");
	RAY_MPI_TAG_CONTIG_IDENTIFICATION_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_IDENTIFICATION_REPLY");
	RAY_MPI_TAG_GET_COVERAGE_AND_PATHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_PATHS");
	RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY");
	RAY_MPI_TAG_SEARCH_COUNTING_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCH_COUNTING_DONE");
	RAY_MPI_TAG_SEARCH_ELEMENTS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCH_ELEMENTS");
	RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY");
	RAY_MPI_TAG_SEARCH_MASTER_COUNT=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCH_MASTER_COUNT");
	RAY_MPI_TAG_SEARCH_MASTER_COUNT_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCH_MASTER_COUNT_REPLY");
	RAY_MPI_TAG_SEARCH_MASTER_SHARING_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCH_MASTER_SHARING_DONE");
	RAY_MPI_TAG_SEARCH_SHARE_COUNTS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCH_SHARE_COUNTS");
	RAY_MPI_TAG_SEARCH_SHARING_COMPLETED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCH_SHARING_COMPLETED");
	RAY_MPI_TAG_SEQUENCE_ABUNDANCE_FINISHED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEQUENCE_ABUNDANCE_FINISHED");
	RAY_MPI_TAG_SEQUENCE_ABUNDANCE_YOU_CAN_GO_HOME=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEQUENCE_ABUNDANCE_YOU_CAN_GO_HOME");
	RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_REPLY");
	RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY");
	RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY");
	RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES");

	RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS");
	RAY_MPI_TAG_ADD_COLORS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ADD_COLORS");
	RAY_MPI_TAG_ADD_KMER_COLOR_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ADD_KMER_COLOR_REPLY");
	RAY_MPI_TAG_SEARCHER_CLOSE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEARCHER_CLOSE");
	
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_AND_COLORS_REPLY");

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS, RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS);
	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES, RAY_MPI_TAG_CONTIG_BIOLOGICAL_ABUNDANCES);
	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES,   RAY_MPI_TAG_SEQUENCE_BIOLOGICAL_ABUNDANCES);
	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_ADD_COLORS, RAY_MPI_TAG_ADD_COLORS );
	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_SEARCHER_CLOSE,RAY_MPI_TAG_SEARCHER_CLOSE);

	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS,        RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_SEQUENCE_BIOLOGICAL_ABUNDANCES, RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_ADD_COLORS, RAY_SLAVE_MODE_ADD_COLORS );
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_CONTIG_BIOLOGICAL_ABUNDANCES, RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_SEARCHER_CLOSE,RAY_SLAVE_MODE_SEARCHER_CLOSE);

	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_CONTIG_ABUNDANCE,             RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY );

	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_GET_COVERAGE_AND_PATHS,       RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY );

	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_CONTIG_ABUNDANCE, 5);

/** data fields:
 *   - k-mer (KMER_U64_ARRAY_SIZE)
 *     - sequence position (1)
 *       - coverage (1)
 *         - number of paths (1)
 *           - path index (1)
 *             - total number of paths (1)
 *               - list of paths (n*2)
 *               **/
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_GET_COVERAGE_AND_PATHS, KMER_U64_ARRAY_SIZE+1+1+1+1+3+4*3 );

	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_ADD_KMER_COLOR, KMER_U64_ARRAY_SIZE+2 );

	/* RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS is the entry point */

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS,RAY_MASTER_MODE_ADD_COLORS);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_ADD_COLORS,RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES,RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES, RAY_MASTER_MODE_SEARCHER_CLOSE);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_SEARCHER_CLOSE, RAY_MASTER_MODE_PHYLOGENY_MAIN);
}


