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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>
*/

#include <search-engine/Searcher.h>
#include <fstream>
#include <memory/malloc_types.h>
#include <sstream>
#include <core/OperatingSystem.h>
#include <stdio.h> /* for fopen, fprintf and fclose */
using namespace std;

//#define CONFIG_CONTIG_ABUNDANCE_VERBOSE
//#define CONFIG_COUNT_ELEMENTS_VERBOSE
//#define CONFIG_DEBUG_IOPS
//#define CONFIG_SEQUENCE_ABUNDANCES_VERBOSE


#define CONFIG_FORCE_VALUE_FOR_MAXIMUM_SPEED false

void Searcher::constructor(Parameters*parameters,StaticVector*outbox,TimePrinter*timePrinter,SwitchMan*switchMan,
	VirtualCommunicator*vc,StaticVector*inbox,RingAllocator*outboxAllocator){

	m_inbox=inbox;

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

void Searcher::call_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS(){

	if(!m_countElementsMasterStarted){
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
		
		m_switchMan->sendToAll(m_outbox,m_parameters->getRank(),RAY_MPI_TAG_SEARCH_SHARE_COUNTS);

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

		// tell root that we are done
		m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),MASTER_RANK,RAY_MPI_TAG_SEARCH_COUNTING_DONE);

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SEARCH_SHARE_COUNTS)){
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

void Searcher::call_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES(){

	if(!m_countContigKmersMasterStarted){
		m_countContigKmersMasterStarted=true;

		// create directories 
		ostringstream directory1;
		directory1<<m_parameters->getPrefix()<<"/BiologicalAbundances";
		string directory1Str=directory1.str();
		createDirectory(directory1Str.c_str());

		ostringstream directory2;
		directory2<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly";
		string directory2Str=directory2.str();
		createDirectory(directory2Str.c_str());

		if(m_writeDetailedFiles){
			ostringstream directory3;
			directory3<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs";
			string directory3Str=directory3.str();
			createDirectory(directory3Str.c_str());

			ostringstream directory4;
			directory4<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/Coverage";
			string directory4Str=directory4.str();
			createDirectory(directory4Str.c_str());

			ostringstream directory5;
			directory5<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/CoverageDistribution";
			string directory5Str=directory5.str();
			createDirectory(directory5Str.c_str());
		}


		// create an empty file for identifications
		ostringstream identifications;
		identifications<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Identifications.tsv";
		m_identificationFile.open(identifications.str().c_str());
		m_identificationFile<<"#Contig	LengthInKmers	Category	SequenceNumber	SequenceName	LengthInKmers	Matches	Ratio"<<endl;

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
		int mode=buffer[bufferPosition++];

		//int totalCoverage=mode*length;

		double*bufferDouble=(double*)(buffer+bufferPosition++);
		double mean=bufferDouble[0];

		ContigSearchEntry entry(name,length,mode,mean);

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
		summary<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs.tsv";

		contigSummaryFile.open(summary.str().c_str());

		contigSummaryFile<<"#Category	Sequence	K-mer length	Length (in k-mers)	Mode k-mer coverage depth";
		contigSummaryFile<<"	Mean k-mer coverage depth";
		contigSummaryFile<<"	Total k-mer coverage depth	Total sample k-mer coverage depth";
		contigSummaryFile<<"	K-mer coverage depth proportion"<<endl;

		// count the total
		uint64_t total=0;
		for(int i=0;i<(int)m_listOfContigEntries.size();i++){
			total+=m_listOfContigEntries[i].getTotal();
		}

		// write entries
		for(int i=0;i<(int)m_listOfContigEntries.size();i++){
			m_listOfContigEntries[i].write(&contigSummaryFile,total,m_parameters->getWordSize());
		}

		m_listOfContigEntries.clear();

		contigSummaryFile.close();

		m_identificationFile.close();

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
		m_requestedCoverage=false;

		m_waitingForAbundanceReply=false;

		m_writeDetailedFiles=m_parameters->hasOption("-search-detailed");

		// this is not implemented
		m_writeDetailedFiles=false;

		m_bufferedData.constructor(m_parameters->getSize(),MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
			RAY_MALLOC_TYPE_KMER_ACADEMY_BUFFER,m_parameters->showMemoryAllocations(),KMER_U64_ARRAY_SIZE);

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
		file1<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/CoverageDistribution/contig-"<<contigName<<".tsv";

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

		for(map<int,int>::iterator i=m_coverageDistribution.begin();i!=m_coverageDistribution.end();i++){
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

		// empty the coverage distribution
		m_coverageDistribution.clear();

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int bufferSize=0;
		buffer[bufferSize++]=contigName;
		buffer[bufferSize++]=lengthInKmers;
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
					file2<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/Coverage/contig-"<<contigName<<".tsv";

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

			// pack the k-mer
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_bufferedData.addAt(rankToFlush,kmer->getU64(i));
			}

			// force flush the message
			if(m_bufferedData.flush(rankToFlush,KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_outboxAllocator,m_outbox,
				m_parameters->getRank(),force)){

				m_pendingMessages++;
				gatheringKmers=false;
			}
		}

		// at this point, we flushed something
		// or we processed all k-mers
		// if nothing was flushed, we force something now
		if(m_pendingMessages==0){

			m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_outboxAllocator,
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
			 && m_inbox->hasMessage(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY)){

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

		// iterate over the coverage values
		for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){

			// get the coverage.
			int coverage=buffer[i];

			#ifdef ASSERT
			assert(coverage>0);
			#endif

			m_coverageDistribution[coverage]++;

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
		m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_outboxAllocator,
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

	}else if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();

		m_timePrinter->printElapsedTime("Counting sequence biological abundances");

		cout<<endl;
	}

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

		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"Starting call_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES"<<endl;
		#endif

		createTrees();

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

		//int toProcess=m_lastSequence-m_firstSequence+1;

		//cout<<"Rank "<<rank<<" partition: "<<m_firstSequence<<" to "<<m_lastSequence<<" (total: "<<toProcess<<")"<<endl;
	
		printDirectoryStart();


		m_kmersProcessed=0;

		m_finished=false;

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
	
	// this sequence is not owned by me
	}else if(!isSequenceOwner() && !m_finished){

		showSequenceAbundanceProgress();

		// skip the file
		// ownership is on a per-file basis
		m_fileIterator++;
		m_globalFileIterator++;
		m_sequenceIterator=0;

		
		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"Skipping"<<endl;
		#endif

	}else if(!m_createdSequenceReader && !m_finished){
		// initiate the reader I guess
	
		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"Create sequence reader"<<endl;
		#endif

		m_searchDirectories[m_directoryIterator].createSequenceReader(m_fileIterator,m_sequenceIterator);

		m_coverageDistribution.clear();
		m_numberOfKmers=0;
		m_matches=0;
		m_createdSequenceReader=true;

		m_requestedCoverage=false;

		m_derivative.clear();
		m_derivative.addX(m_numberOfKmers);

	// compute abundances
	}else if(m_createdSequenceReader && !m_finished){
		if(m_pendingMessages==0  && m_bufferedData.isEmpty() &&  /* nothing to flush... */
			 !m_searchDirectories[m_directoryIterator].hasNextKmer(m_kmerLength)){
			
			m_derivative.addX(m_numberOfKmers);

			// process the thing, possibly send it to be written

			int mode=0;
			int modeCount=0;
			uint64_t sum=0;
			uint64_t totalCount=0;

			for(map<int,int>::iterator i=m_coverageDistribution.begin();i!=m_coverageDistribution.end();i++){
				int count=i->second;
				int coverage=i->first;

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

			showSequenceAbundanceProgress();

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

			#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
			cout<<"dir "<<m_directoryIterator<<" file "<<m_fileIterator<<" sequence "<<m_sequenceIterator<<" global "<<m_globalSequenceIterator<<" mode "<<mode<<endl;
			#endif

			double ratio=(0.0+m_matches)/m_numberOfKmers;
	
			// open the file
			// don't open it if there are 0 matches
			if((m_arrayOfFiles.count(m_directoryIterator)==0 || 
				m_arrayOfFiles[m_directoryIterator].count(m_fileIterator)==0) && m_matches>0){
				
				string*theDirectoryPath=m_searchDirectories[m_directoryIterator].getDirectoryName();
				string baseName=getBaseName(*theDirectoryPath);
	
	
				ostringstream fileName;
				fileName<<m_parameters->getPrefix()<<"/BiologicalAbundances/";
				fileName<<baseName<<"/";
			
				// add the file name without the .fasta
				string*theFileName=m_searchDirectories[m_directoryIterator].getFileName(m_fileIterator);
				fileName<<theFileName->substr(0,theFileName->length()-6)<<".tsv";
	
				m_arrayOfFiles[m_directoryIterator][m_fileIterator]=fopen(fileName.str().c_str(),"w");
				
				#ifdef ASSERT
				assert(m_activeFiles==0); // it is 0 or 1... 
				#endif
	
				m_activeFiles++;
		
				#ifdef ASSERT
				assert(m_activeFiles==1); // it is 0 or 1... 
				#endif
	
				fprintf(m_arrayOfFiles[m_directoryIterator][m_fileIterator],
					"#Category	SequenceNumber	SequenceName	LengthInKmers	Matches	Ratio	ModeKmerCoverage	MeanKmerCoverage\n");
		
				cout<<"Opened "<<fileName.str()<<", active file descriptors: "<<m_activeFiles<<endl;
			}
	
			// write the file if there are not 0 matches
			if(m_matches>0){
				ostringstream content;
				
				string sequenceName=m_searchDirectories[m_directoryIterator].getCurrentSequenceName();

				content<<m_fileNames[m_directoryIterator][m_fileIterator];
				content<<"	"<<m_sequenceIterator<<"	"<<sequenceName<<"	"<<m_numberOfKmers;
				content<<"	"<<m_matches<<"	"<<ratio<<"	"<<mode<<"	"<<mean<<endl;
	
				fprintf(m_arrayOfFiles[m_directoryIterator][m_fileIterator],
				"%s",content.str().c_str());
			}
	
			// close the file
			// even if there is 0 matches,
			// we need to close the file...
			// if it exists of course
			if(isLast && m_arrayOfFiles.count(m_directoryIterator)>0 
				&& m_arrayOfFiles[m_directoryIterator].count(m_fileIterator)>0){
	
				fclose(m_arrayOfFiles[m_directoryIterator][m_fileIterator]);
	
				#ifdef ASSERT
				assert(m_activeFiles==1);
				#endif
	
				m_activeFiles--;
	
				#ifdef ASSERT
				assert(m_activeFiles==0);
				#endif
	
				cout<<"Closed file "<<m_directoryIterator<<" "<<m_fileIterator<<", active file descriptors: "<<m_activeFiles<<endl;
	
				m_arrayOfFiles[m_directoryIterator].erase(m_fileIterator);
			}

			// go to the next sequence
			m_sequenceIterator++;
			m_globalSequenceIterator++;
			m_createdSequenceReader=false;
			m_requestedCoverage=false;

		}else if(m_pendingMessages==0 && !m_searchDirectories[m_directoryIterator].hasNextKmer(m_kmerLength) 
				 && m_bufferedData.isEmpty()){

			// we have to wait because we sent the summary 
			// and the response is not there yet

		// k-mers are available
		}else if(m_pendingMessages==0 && !m_requestedCoverage && m_bufferedData.isEmpty() ){
	
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

				int rankToFlush=m_parameters->_vertexRank(&kmer);

				// pack the k-mer
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedData.addAt(rankToFlush,kmer.getU64(i));
				}

				// force flush the message
				if(m_bufferedData.flush(rankToFlush,KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_outboxAllocator,m_outbox,
					m_parameters->getRank(),force)){
					m_pendingMessages++;
					gatheringKmers=false;
				}
			}

			// at this point, we flushed something
			// or we processed all k-mers
			// if nothing was flushed, we force something now
			if(m_pendingMessages==0){

				m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_outboxAllocator,
					m_outbox,m_parameters->getRank());

				#ifdef ASSERT
				assert(m_pendingMessages>0);
				#endif
			}

			#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
			if(m_numberOfKmers%1000==0)
				cout<<"Sending RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE, position="<<m_numberOfKmers<<endl;
			#endif
	
			m_requestedCoverage=true;

		}else if(m_pendingMessages > 0 &&
				m_requestedCoverage && m_inbox->hasMessage(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY)){

			Message*message=m_inbox->at(0);
			uint64_t*buffer=message->getBuffer();

			#ifdef ASSERT
			assert(message!=NULL);
			#endif

			int count=message->getCount();

			m_pendingMessages--;

			// iterate over the coverage values
			for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){

				// get the coverage.
				int coverage=buffer[i];

				if(coverage>0){
					m_coverageDistribution[coverage]++;
					m_matches++;
				}

				#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
				if(m_numberOfKmers%1000==0)
					cout<<"Received coverage position = "<<m_numberOfKmers<<" val= "<<coverage<<endl;
				#endif
			
				if(false && m_numberOfKmers%10000==0 && m_numberOfKmers > 0){
					cout<<"Rank "<<m_parameters->getRank()<<" processing sequence "<<m_globalSequenceIterator;
					cout<<" ProcessedKmers= "<<m_numberOfKmers<<endl;
				}

				m_numberOfKmers++;
				m_kmersProcessed++;

				if(m_kmersProcessed%1000000==0){
					showProcessedKmers();
				}
			}

			m_requestedCoverage=false;

		// we processed all the k-mers
		// now we need to flush the remaining half-full buffers
		// this section is now used if 
		// force=true 
		// in the above code
		}else if(m_pendingMessages==0 && !m_bufferedData.isEmpty()){

			m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_outboxAllocator,
				m_outbox,m_parameters->getRank());
	
			m_requestedCoverage=true;
		}
	}
}

/**
 * create trees
 * actually, only master creates trees
 * but all gather directory names.
 */
void Searcher::createTrees(){

	bool lazy=true;

	for(int i=0;i<m_searchDirectories_size;i++){
		string*directory=m_searchDirectories[i].getDirectoryName();

		string baseName=getBaseName(*directory);

		ostringstream directory2;
		directory2<<m_parameters->getPrefix()<<"/BiologicalAbundances/"<<baseName;
		string directory2Str=directory2.str();

		#ifdef CONFIG_SEQUENCE_ABUNDANCES_VERBOSE
		cout<<"create dir "<<directory2Str<<endl;
		#endif

		if(m_parameters->getRank()==MASTER_RANK)
			createDirectory(directory2Str.c_str());

		m_directoryNames.push_back(baseName);

		vector<string> directories;

		for(int j=0;j<(int)m_searchDirectories[i].getSize();j++){
			string*file=m_searchDirectories[i].getFileName(j);

			int theLength=file->length();
			// .fasta is 6

			#ifdef ASSERT
			assert(theLength>0);
			#endif

			//cout<<"code 5"<<endl;
			string theFile=file->substr(0,theLength-6);

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
		}

		m_fileNames.push_back(directories);
	}
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

void Searcher::showSequenceAbundanceProgress(){
	// show progress
	if(m_globalSequenceIterator % 100==0 || m_sequenceIterator == m_searchDirectories[m_directoryIterator].getCount(m_fileIterator)){
		cout<<"Rank "<<m_parameters->getRank()<<" biological abundances ";
		cout<<m_globalSequenceIterator<<" ["<<m_directoryIterator+1;
		cout<<"/"<<m_searchDirectories_size<<"] ["<<m_fileIterator+1<<"/";
		cout<<m_searchDirectories[m_directoryIterator].getSize()<<"] ["<<m_sequenceIterator+1;
		cout<<"/"<<m_searchDirectories[m_directoryIterator].getCount(m_fileIterator)<<"]"<<endl;

		// display the speed obtained.
		m_derivative.printStatus(SLAVE_MODES[m_switchMan->getSlaveMode()],
				m_switchMan->getSlaveMode());

	}

}

bool Searcher::isSequenceOwner(){
	return m_globalFileIterator%m_parameters->getSize()==m_parameters->getRank();
}

void Searcher::printDirectoryStart(){
	if(! (m_directoryIterator< m_searchDirectories_size))
		return;

	cout<<endl;
	cout<<"Rank "<<m_parameters->getRank()<<" starting to process "<<m_directoryNames[m_directoryIterator]<<endl;
}

void Searcher::showProcessedKmers(){
	cout<<"Rank "<<m_parameters->getRank()<<": Searcher: processed k-mers so far: "<<m_kmersProcessed<<endl;
}
