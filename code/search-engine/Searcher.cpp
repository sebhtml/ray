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
#include <sstream>
#include <core/OperatingSystem.h>
using namespace std;

//#define CONFIG_CONTIG_ABUNDANCE_VERBOSE
//#define CONFIG_COUNT_ELEMENTS_VERBOSE
//#define CONFIG_DEBUG_IOPS

void Searcher::constructor(Parameters*parameters,StaticVector*outbox,TimePrinter*timePrinter,SwitchMan*switchMan,
	VirtualCommunicator*vc,StaticVector*inbox,RingAllocator*outboxAllocator){

	m_inbox=inbox;

	m_virtualCommunicator=vc;

	m_countElementsSlaveStarted=false;
	m_countElementsMasterStarted=false;
	
	m_countContigKmersMasterStarted=false;
	m_countContigKmersSlaveStarted=false;

	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;
	m_timePrinter=timePrinter;
	m_switchMan=switchMan;
}

void Searcher::countElements_masterMethod(){

	if(!m_countElementsMasterStarted){
		m_countElementsMasterStarted=true;
		m_ranksDoneCounting=0;

		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());
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
	}else if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();

		m_timePrinter->printElapsedTime("Counting search category sequences");
		cout<<endl;
	}
}

void Searcher::countElements_slaveMethod(){

	if(!m_countElementsSlaveStarted){

		m_countElementsSlaveStarted=true;
		m_listedDirectories=false;

		#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
		cout<<"init"<<endl;
		#endif

	}else if(!m_listedDirectories){
		#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
		cout<<"listing"<<endl;
		#endif

		vector<string>*directories=m_parameters->getSearchDirectories();
		for(int i=0;i<(int)directories->size();i++){
			SearchDirectory entry;
			entry.constructor(directories->at(i));
			m_searchDirectories.push_back(entry);
		}

		m_listedDirectories=true;
		
		m_countedDirectories=false;
	}else if(!m_countedDirectories){
		#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
		cout<<"counting"<<endl;
		#endif

		// no communication here
		int fileNumber=0;

		for(int i=0;i<(int)m_searchDirectories.size();i++){
			int count=m_searchDirectories[i].getSize();
			for(int j=0;j<count;j++){
				if(fileNumber%m_parameters->getSize() == m_parameters->getRank()){
					#ifdef CONFIG_COUNT_ELEMENTS_VERBOSE
					cout<<"Hop counting"<<endl;
					#endif

					m_searchDirectories[i].countEntriesInFile(j);
					cout<<"Rank "<<m_parameters->getRank()<<" "<<*(m_searchDirectories[i].getDirectoryName())<<""<<*(m_searchDirectories[i].getFileName(j))<<" -> "<<m_searchDirectories[i].getCount(j)<<endl;
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
		if(m_directoryIterator==(int)m_searchDirectories.size()){
			m_sharedCounts=true;
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
	}else if(m_sharedCounts){

		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());
	}
}

void Searcher::countContigKmers_masterHandler(){

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

		ostringstream directory3;
		directory3<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs";
		string directory3Str=directory3.str();
		createDirectory(directory3Str.c_str());

		if(m_writeDetailedFiles){
			ostringstream directory4;
			directory4<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/Coverage";
			string directory4Str=directory4.str();
			createDirectory(directory4Str.c_str());

			ostringstream directory5;
			directory5<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/CoverageDistribution";
			string directory5Str=directory5.str();
			createDirectory(directory5Str.c_str());
		}

		ostringstream summary;
		summary<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/Summary.tsv";

		m_contigSummaryFile.open(summary.str().c_str());

		m_contigSummaryFile<<"#Category	SequenceName LengthInKmers Matches Ratio ModeKmerCoverage"<<endl;

		// create an empty summary at thet top level
		ostringstream summary2;
		summary2<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Summary.tsv";
		ofstream f3(summary2.str().c_str());
		f3<<"#Category	SequenceName LengthInKmers Matches Ratio ModeKmerCoverage"<<endl;
		f3.close();

		// create an empty file for identifications
		ostringstream identifications;
		identifications<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/Identifications.tsv";
		m_identificationFile.open(identifications.str().c_str());
		m_identificationFile<<"#Contig LengthInKmers Category SequenceName LengthInKmers Matches Ratio"<<endl;

		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_CONTIG_ABUNDANCE)){
		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Received RAY_MPI_TAG_CONTIG_ABUNDANCE"<<endl;
		#endif

		Message*message=m_inbox->at(0);
		uint64_t*buffer=message->getBuffer();
		uint64_t name=buffer[0];
		int length=buffer[1];
		int mode=buffer[2];

		m_contigSummaryFile<<"Contigs	contig-"<<name<<"	"<<length<<"	"<<length<<"	1.00	"<<mode<<endl;
	
		m_identificationFile<<"contig-"<<name<<"	"<<length<<"	UnknownCategory	UnknownSequence	UnknownLength	UnknownMatches	UnknownRatio"<<endl;

		// sent a response
		uint64_t*buffer2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_CONTIG_ABUNDANCE);

		Message aMessage(buffer2,elementsPerQuery,
			message->getSource(),
			RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY,m_parameters->getRank());

		m_outbox->push_back(aMessage);
	}else if(m_switchMan->allRanksAreReady()){
		m_contigSummaryFile.close();
		m_identificationFile.close();

		m_timePrinter->printElapsedTime("Counting contig biological abundances");
		cout<<endl;

		m_switchMan->closeMasterMode();
	}
}

void Searcher::countContigKmers_slaveHandler(){
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

	// we have finished our part
	}else if(m_contig == (int) m_contigs->size()){
		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());

		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Processed all contigs"<<endl;
		#endif

	// we finished a contig
	}else if(m_contigPosition==(int)(*m_contigs)[m_contig].size()){

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
		}

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
	}else if(m_waitingForAbundanceReply && m_virtualCommunicator->isMessageProcessed(m_workerId)){
		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Received reply for abundance"<<endl;
		#endif

		m_waitingForAbundanceReply=false;
		vector<uint64_t> data;
		m_virtualCommunicator->getMessageResponseElements(m_workerId,&data);
	}else if(!m_waitingForAbundanceReply && !m_requestedCoverage){
		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Requesting coverage at "<<m_contigPosition<<endl;
		#endif

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int bufferPosition=0;
		Kmer*kmer=&((*m_contigs)[m_contig][m_contigPosition]);
		kmer->pack(buffer,&bufferPosition);

		int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE);

		Message aMessage(buffer,elementsPerQuery,
			m_parameters->_vertexRank(kmer),
			RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_parameters->getRank());

		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
	
		m_requestedCoverage=true;
	}else if(!m_waitingForAbundanceReply &&m_requestedCoverage && m_virtualCommunicator->isMessageProcessed(m_workerId)){
		#ifdef CONFIG_CONTIG_ABUNDANCE_VERBOSE
		cout<<"Receiving coverage at "<<m_contigPosition<<endl;
		#endif
		vector<uint64_t> data;
		m_virtualCommunicator->getMessageResponseElements(m_workerId,&data);
		int coverage=data[0];

		if(m_contigPosition==0){
			#ifdef CONFIG_DEBUG_IOPS
			cout<<"Opening file"<<endl;
			#endif

			uint64_t contigName=m_contigNames->at(m_contig);
			ostringstream file2;
			file2<<m_parameters->getPrefix()<<"/BiologicalAbundances/DeNovoAssembly/Contigs/Coverage/contig-"<<contigName<<".tsv";

			if(m_writeDetailedFiles){
				m_currentCoverageFile.open(file2.str().c_str());
				m_currentCoverageFile<<"#KmerPosition	KmerCoverage"<<endl;
			}
		}

		if(m_contigPosition % 1000 == 0 || m_contigPosition==(int)m_contigs->at(m_contig).size()-1){
			showContigAbundanceProgress();
		}

		if(m_writeDetailedFiles){
			m_currentCoverageFile<<m_contigPosition+1<<"	"<<coverage<<endl;
		}

		m_coverageDistribution[coverage]++;

		m_contigPosition++;
		m_requestedCoverage=false;
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
