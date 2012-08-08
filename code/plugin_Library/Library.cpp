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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

//#define GUILLIMIN_BUG

#include <plugin_SequencesIndexer/ReadAnnotation.h>
#include <plugin_Library/Library.h>
#include <communication/mpi_tags.h>
#include <sstream>
#include <core/OperatingSystem.h>
#include <application_core/common_functions.h>
#include <assert.h>
#include <application_core/Parameters.h>


__CreatePlugin(Library);

 /**/
__CreateMasterModeAdapter(Library,RAY_MASTER_MODE_UPDATE_DISTANCES); /**/
 /**/
__CreateSlaveModeAdapter(Library,RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION); /**/
__CreateSlaveModeAdapter(Library,RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES); /**/
 /**/
 /**/

using namespace std;

/* send the information to all ranks */
void Library::call_RAY_MASTER_MODE_UPDATE_DISTANCES(){
	if(m_currentLibrary<m_parameters->getNumberOfLibraries()){
		/** don't send information for manually-provided information */
		if(!m_parameters->isAutomatic(m_currentLibrary)){
			m_currentLibrary++;
		/** send the message if not already done */
		}else if(!m_informationSent){
			MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			int outputPosition=0;
			for(int i=0;i<m_parameters->getLibraryPeaks(m_currentLibrary);i++){
				message[outputPosition++]=m_currentLibrary;
				message[outputPosition++]=m_parameters->getLibraryAverageLength(m_currentLibrary,i);
				message[outputPosition++]=m_parameters->getLibraryStandardDeviation(m_currentLibrary,i);
			}

			for(int i=0;i<m_size;i++){
				Message aMessage(message,outputPosition,i,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION,m_rank);
				m_outbox->push_back(aMessage);
			}

			m_informationSent=true;
			m_ranksThatReplied=0;
		/** wait for a reply */
		}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()==RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY){
			m_ranksThatReplied++;
	
			/** when everyone replied, we can proceed with the next library */
			if(m_ranksThatReplied==m_parameters->getSize()){
				m_currentLibrary++;
				m_informationSent=false;
				m_ranksThatReplied=0;
			}
		}
	/** basically, we updated all libraries */
	}else{
		m_timePrinter->printElapsedTime("Estimation of outer distances for paired reads");
		cout<<endl;

		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_EXTENSIONS;
		m_ed->m_EXTENSION_rank=-1;
		m_ed->m_EXTENSION_currentRankIsSet=false;
	}
}

void Library::call_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION(){
	if(!m_initiatedIterator){
		m_SEEDING_i=0;

		m_activeWorkerIterator=m_activeWorkers.begin();
		m_initiatedIterator=true;
		m_maximumAliveWorkers=32768;

		if(!m_parameters->hasPairedReads()){
			completeSlaveMode();
			return;
		}


		m_virtualCommunicator->resetCounters();
	}

	#ifdef GUILLIMIN_BUG
	if(m_inbox->hasMessage(RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY)){
		Message*message=m_inbox->at(0);
		MessageUnit*buffer=message->getBuffer();
		if(m_parameters->getRank()==message->getSource()){
			cout<<endl;
			cout<<"Globally receiving RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY from "<<message->getSource()<<endl;
			cout<<"Inbox: "<<m_inbox->size()<<" Outbox: "<<m_outbox->size()<<endl;
			for(int p=0;p<message->getCount();p++){
				cout<<"; "<<p<<" -> "<<buffer[p];
			}
			cout<<endl;
			cout<<"This is before m_virtualCommunicator->processInbox()"<<endl;
		}
	}
	#endif


	m_virtualCommunicator->processInbox(&m_activeWorkersToRestore);

	#ifdef GUILLIMIN_BUG
	if(m_inbox->hasMessage(RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY)){
		Message*message=m_inbox->at(0);
		MessageUnit*buffer=message->getBuffer();
		if(m_parameters->getRank()==message->getSource()){
			cout<<endl;
			cout<<"Globally receiving RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY from "<<message->getSource()<<endl;
			cout<<"Inbox: "<<m_inbox->size()<<" Outbox: "<<m_outbox->size()<<endl;
			for(int p=0;p<message->getCount();p++){
				cout<<"; "<<p<<" -> "<<buffer[p];
			}
			cout<<endl;
			cout<<"This is after m_virtualCommunicator->processInbox()"<<endl;
		}
	}

	if(m_outbox->hasMessage(RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY)){
		Message*message=m_outbox->at(0);
		MessageUnit*buffer=message->getBuffer();
		if(m_parameters->getRank()==message->getDestination()){
			cout<<endl;
			cout<<"Globally sending RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY to "<<message->getDestination()<<endl;
			cout<<"Inbox: "<<m_inbox->size()<<" Outbox: "<<m_outbox->size()<<endl;
			for(int p=0;p<message->getCount();p++){
				cout<<"; "<<p<<" -> "<<buffer[p];
			}
			cout<<endl;
			cout<<"This is before the code in call_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION()"<<endl;
		}
	}

	#endif
	
	/* there is a strange bug that is avoided by waiting an
		extra tick before doing actual stuff.
		if we don't wait for m_outbox to be flushed, we get
		2 messages in the outbox and something strange happens
		
		the bug does not happen on 

			- Mammouth Parallel II;
			- colosse
			- ls30

		the bug occurs only on

			- guillimin
	*/
	if(!m_virtualCommunicator->isReady() || m_outbox->size() > 0){
		#ifdef GUILLIMIN_BUG
		if(m_inbox->size()>0 || m_outbox->size()>0){
			cout<<"m_virtualCommunicator is not ready yet."<<endl;
			cout<<"Inbox: "<<m_inbox->size()<<" Outbox: "<<m_outbox->size()<<endl;
			cout<<"returning."<<endl;
		}
		#endif

		return;
	}

	if(m_activeWorkerIterator!=m_activeWorkers.end()){
		WorkerHandle workerId=*m_activeWorkerIterator;

		#ifdef ASSERT
		if(m_aliveWorkers.count(workerId)==0){
			cout<<"Error: "<<workerId<<" is not in alive workers "<<m_activeWorkers.size()<<endl;
		}
		assert(m_aliveWorkers.count(workerId)>0);
		assert(!m_aliveWorkers[workerId].isDone());
		#endif

		m_virtualCommunicator->resetLocalPushedMessageStatus();

		//force the worker to work until he finishes or pushes something on the stack
		while(!m_aliveWorkers[workerId].isDone()&&!m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_aliveWorkers[workerId].work();
		}

		if(m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_waitingWorkers.push_back(workerId);
		}
		if(m_aliveWorkers[workerId].isDone()){
			m_workersDone.push_back(workerId);
		}
		m_activeWorkerIterator++;
	}else{
		updateStates();

		//  add one worker to active workers
		//  reason is that those already in the pool don't communicate anymore -- 
		//  as for they need responses.
		if(!m_virtualCommunicator->getGlobalPushedMessageStatus()&&m_activeWorkers.empty()){
			// there is at least one worker to start
			// AND
			// the number of alive workers is below the maximum
			if(m_SEEDING_i<m_seedingData->m_SEEDING_seeds.size()
				&&(int)m_aliveWorkers.size()<m_maximumAliveWorkers){

				#ifdef ASSERT
				if(m_SEEDING_i==0){
					assert(m_completedJobs==0&&m_activeWorkers.size()==0&&m_aliveWorkers.size()==0);
				}
				#endif

				// create a worker
				m_aliveWorkers[m_SEEDING_i].constructor(m_SEEDING_i,m_seedingData,m_virtualCommunicator,m_outboxAllocator,m_parameters,m_inbox,m_outbox,&m_libraryDistances,&m_detectedDistances,&m_allocator,
					RAY_MPI_TAG_GET_READ_MATE, RAY_MPI_TAG_REQUEST_VERTEX_READS
					);

				m_activeWorkers.insert(m_SEEDING_i);
				int population=m_aliveWorkers.size();
				if(population>m_maximumWorkers){
					m_maximumWorkers=population;
				}
				m_SEEDING_i++;
			}else{
	
				/* if there are no active workers and we failed to add
				new workers above, then we need to flush something right
				now
				otherwise, it is safe to wait a little longer */
				if(m_activeWorkers.empty()){
					m_virtualCommunicator->forceFlush();
				}

				#ifdef GUILLIMIN_BUG
				if(m_outbox->size()>0){
					cout<<endl;
					cout<<"Produced messages with forceFlush()"<<endl;
				
					cout<<"Inbox: "<<m_inbox->size()<<" Outbox: "<<m_outbox->size()<<endl;

					for(int p=0;p<m_outbox->size();p++){
						cout<<endl;
						cout<<"Message "<<p<<" destination "<<m_outbox->at(p)->getDestination();
						cout<<" tag "<<MESSAGE_TAGS[m_outbox->at(p)->getTag()]<<endl;
						cout<<"Content"<<endl;
						MessageUnit*buffer=m_outbox->at(p)->getBuffer();
						for(int q=0;q<m_outbox->at(p)->getCount();q++){
							cout<<"; "<<q<<" -> "<<buffer[q];
						}
						cout<<endl;
					}
				}
				#endif
			}
		}

		m_activeWorkerIterator=m_activeWorkers.begin();
	}

	#ifdef ASSERT
	assert((int)m_aliveWorkers.size()<=m_maximumAliveWorkers);
	#endif

	if(m_completedJobs==(int)m_seedingData->m_SEEDING_seeds.size()){
		printf("Rank %i detected %i library lengths\n",getRank(),m_detectedDistances);
		fflush(stdout);
		printf("Rank %i is calculating library lengths [%i/%i] (completed)\n",getRank(),(int)m_seedingData->m_SEEDING_seeds.size(),(int)m_seedingData->m_SEEDING_seeds.size());
		fflush(stdout);
		
		completeSlaveMode();
	}

	#ifdef GUILLIMIN_BUG
	for(int i=0;i<(int)m_outbox->size();i++){

		if(m_outbox->at(i)->getTag()!=RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY){
			continue;
		}

		Message*message=m_outbox->at(i);
		MessageUnit*buffer=message->getBuffer();
		if(m_parameters->getRank()==message->getDestination()){
			cout<<endl;
			cout<<"Globally sending RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY to "<<message->getDestination()<<endl;
			cout<<"Inbox: "<<m_inbox->size()<<" Outbox: "<<m_outbox->size()<<endl;
			for(int p=0;p<message->getCount();p++){
				cout<<"; "<<p<<" -> "<<buffer[p];
			}
			cout<<endl;
			cout<<"This is after the code in call_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION()"<<endl;
		}
	}
	#endif
}

void Library::constructor(int m_rank,StaticVector*m_outbox,RingAllocator*m_outboxAllocator,ExtensionData*m_ed,
int m_size,
TimePrinter*m_timePrinter,int*m_mode,int*m_master_mode,
Parameters*m_parameters,SeedingData*m_seedingData,StaticVector*inbox,VirtualCommunicator*vc
){
	this->m_rank=m_rank;
	m_currentLibrary=0;
	m_ranksThatReplied=0;
	this->m_outbox=m_outbox;
	this->m_outboxAllocator=m_outboxAllocator;
	#ifdef ASSERT
	assert(this->m_outboxAllocator!=NULL);
	#endif
	this->m_ed=m_ed;
	this->m_size=m_size;
	this->m_timePrinter=m_timePrinter;
	this->m_mode=m_mode;
	this->m_master_mode=m_master_mode;
	this->m_parameters=m_parameters;
	this->m_seedingData=m_seedingData;
	m_ready=0;
	m_virtualCommunicator=vc;
	m_inbox=inbox;

	m_initiatedIterator=false;
	ostringstream prefixFull;
	prefixFull<<m_parameters->getMemoryPrefix()<<"_Library";
	int chunkSize=4194304;
	m_allocator.constructor(chunkSize,"RAY_MALLOC_TYPE_LIBRARY_ALLOCATOR",m_parameters->showMemoryAllocations());
	m_completedJobs=0;
	m_informationSent=false;
}

void Library::setReadiness(){
	m_ready--;
}

int Library::getRank(){
	return m_rank;
}

int Library::getSize(){
	return m_size;
}

Library::Library(){
	m_detectedDistances=0;
}

void Library::allocateBuffers(){
	m_bufferedData.constructor(m_size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),
		"RAY_MALLOC_TYPE_LIBRARY_BUFFERS",m_parameters->showMemoryAllocations(),3);
	(m_libraryIndexInitiated)=false;
	(m_libraryIterator)=0;
	for(map<int,map<int,int> >::iterator i=m_libraryDistances.begin();
		i!=m_libraryDistances.end();i++){
		int libraryName=i->first;
		m_libraryIndexes.push_back(libraryName);
	}
}

void Library::call_RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES(){
	if(m_ready!=0){
		return;
	}

	if(m_libraryIterator==(int)m_libraryIndexes.size()){
		if(!m_bufferedData.isEmpty()){
			m_ready+=m_bufferedData.flushAll(RAY_MPI_TAG_LIBRARY_DISTANCE,m_outboxAllocator,m_outbox,getRank());
			return;
		}
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED,getRank());
		m_outbox->push_back(aMessage);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		m_bufferedData.showStatistics(m_parameters->getRank());
	}else if(!m_libraryIndexInitiated){
		m_libraryIndexInitiated=true;
		m_libraryIndex=m_libraryDistances[m_libraryIndexes[m_libraryIterator]].begin();
	}else if(m_libraryIndex==m_libraryDistances[m_libraryIndexes[m_libraryIterator]].end()){
		m_libraryIterator++;
		m_libraryIndexInitiated=false;
	}else{
		int library=m_libraryIndexes[m_libraryIterator];
		int distance=m_libraryIndex->first;
		int count=m_libraryIndex->second;
		m_bufferedData.addAt(MASTER_RANK,library);
		m_bufferedData.addAt(MASTER_RANK,distance);
		m_bufferedData.addAt(MASTER_RANK,count);

		if(m_bufferedData.flush(MASTER_RANK,3,RAY_MPI_TAG_LIBRARY_DISTANCE,m_outboxAllocator,m_outbox,getRank(),false)){
			m_ready++;
		}

		m_libraryIndex++;
	}
}

void Library::updateStates(){
	// erase completed jobs
	for(int i=0;i<(int)m_workersDone.size();i++){
		WorkerHandle workerId=m_workersDone[i];

		#ifdef ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		assert(m_aliveWorkers.count(workerId)>0);
		#endif

		m_activeWorkers.erase(workerId);
		m_aliveWorkers.erase(workerId);
		if(m_completedJobs%10==0){
			printf("Rank %i is calculating library lengths [%i/%i]\n",m_parameters->getRank(),m_completedJobs+1,(int)m_seedingData->m_SEEDING_seeds.size());
			fflush(stdout);
		}

		m_completedJobs++;
	}
	m_workersDone.clear();

	for(int i=0;i<(int)m_waitingWorkers.size();i++){
		WorkerHandle workerId=m_waitingWorkers[i];

		#ifdef ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		#endif

		m_activeWorkers.erase(workerId);
	}
	m_waitingWorkers.clear();

	for(int i=0;i<(int)m_activeWorkersToRestore.size();i++){
		WorkerHandle workerId=m_activeWorkersToRestore[i];

		m_activeWorkers.insert(workerId);
	}
	m_activeWorkersToRestore.clear();

	m_virtualCommunicator->resetGlobalPushedMessageStatus();
}

void Library::completeSlaveMode(){

	Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE,getRank());
	m_outbox->push_back(aMessage);
	(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
	m_allocator.clear();

	printf("Rank %i: peak number of workers: %i, maximum: %i\n",m_rank,m_maximumWorkers,m_maximumAliveWorkers);
	fflush(stdout);
	m_virtualCommunicator->printStatistics();

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_rank);
	}

}

void Library::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"Library");
	core->setPluginDescription(plugin,"Obtains an estimation of library lengths");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION, __GetAdapter(Library,RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION,"RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION");

	RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES, __GetAdapter(Library,RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES,"RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES");

	RAY_MASTER_MODE_UPDATE_DISTANCES=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_UPDATE_DISTANCES, __GetAdapter(Library,RAY_MASTER_MODE_UPDATE_DISTANCES));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_UPDATE_DISTANCES,"RAY_MASTER_MODE_UPDATE_DISTANCES");

	RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY,"RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY");


}

void Library::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES");
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");
	RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION");

	RAY_MASTER_MODE_TRIGGER_EXTENSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_EXTENSIONS");
	RAY_MASTER_MODE_UPDATE_DISTANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_UPDATE_DISTANCES");
	RAY_MASTER_MODE_TRIGGER_FUSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_FUSIONS");

	RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED");
	RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE");
	RAY_MPI_TAG_LIBRARY_DISTANCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_LIBRARY_DISTANCE");
	RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION");
	RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY");

	RAY_MPI_TAG_GET_READ_MATE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MATE");
	RAY_MPI_TAG_REQUEST_VERTEX_READS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_READS");
	RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY");

	RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY");


	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_UPDATE_DISTANCES,RAY_MASTER_MODE_TRIGGER_FUSIONS);

	__BindPlugin(Library);
}


