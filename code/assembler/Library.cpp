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

#include <structures/ReadAnnotation.h>
#include <assembler/Library.h>
#include <communication/mpi_tags.h>
#include <sstream>
#include <core/common_functions.h>
#include <assert.h>
#include <core/Parameters.h>
#include <memory/malloc_types.h>
using namespace std;

void Library::updateDistances(){
	uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int libraries=m_parameters->getNumberOfLibraries();
	int*intMessage=(int*)message;
	intMessage[0]=libraries;

	for(int i=0;i<libraries;i++){
		int average=m_parameters->getLibraryAverageLength(i);
		int deviation=m_parameters->getLibraryStandardDeviation(i);
		intMessage[2*i+0+1]=average;
		intMessage[2*i+1+1]=deviation;
	}

	for(int i=0;i<m_size;i++){
		Message aMessage((uint64_t*)intMessage,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),i,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION,m_rank);
		m_outbox->push_back(aMessage);
	}

	m_timePrinter->printElapsedTime("Estimation of outer distances for paired reads");
	cout<<endl;

	(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_EXTENSIONS;
	m_ed->m_EXTENSION_rank=-1;
	m_ed->m_EXTENSION_currentRankIsSet=false;
}

void Library::detectDistances(){
	if(!m_initiatedIterator){
		m_SEEDING_i=0;

		m_activeWorkerIterator=m_activeWorkers.begin();
		m_initiatedIterator=true;
		m_maximumAliveWorkers=30000;
	}

	m_virtualCommunicator->processInbox(&m_activeWorkersToRestore);


	if(!m_virtualCommunicator->isReady()){
		return;
	}

	if(m_activeWorkerIterator!=m_activeWorkers.end()){
		uint64_t workerId=*m_activeWorkerIterator;
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
			if(m_SEEDING_i<m_seedingData->m_SEEDING_seeds.size()&&(int)m_aliveWorkers.size()<m_maximumAliveWorkers){

				#ifdef ASSERT
				if(m_SEEDING_i==0){
					assert(m_completedJobs==0&&m_activeWorkers.size()==0&&m_aliveWorkers.size()==0);
				}
				#endif

				m_aliveWorkers[m_SEEDING_i].constructor(m_SEEDING_i,m_seedingData,m_virtualCommunicator,m_outboxAllocator,m_parameters,m_inbox,m_outbox,&m_libraryDistances,&m_detectedDistances,&m_allocator);
				m_activeWorkers.insert(m_SEEDING_i);
				int population=m_aliveWorkers.size();
				if(population>m_maximumWorkers){
					m_maximumWorkers=population;
				}
				m_SEEDING_i++;
			}else{
				m_virtualCommunicator->forceFlush();
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
}

void Library::constructor(int m_rank,StaticVector*m_outbox,RingAllocator*m_outboxAllocator,int*m_sequence_id,int*m_sequence_idInFile,ExtensionData*m_ed,
int m_size,
TimePrinter*m_timePrinter,int*m_mode,int*m_master_mode,
Parameters*m_parameters,int*m_fileId,SeedingData*m_seedingData,StaticVector*inbox,VirtualCommunicator*vc
){
	this->m_rank=m_rank;
	this->m_outbox=m_outbox;
	this->m_outboxAllocator=m_outboxAllocator;
	#ifdef ASSERT
	assert(this->m_outboxAllocator!=NULL);
	#endif
	this->m_sequence_id=m_sequence_id;
	this->m_sequence_idInFile=m_sequence_idInFile;
	this->m_ed=m_ed;
	this->m_size=m_size;
	this->m_timePrinter=m_timePrinter;
	this->m_mode=m_mode;
	this->m_master_mode=m_master_mode;
	this->m_parameters=m_parameters;
	this->m_fileId=m_fileId;
	this->m_seedingData=m_seedingData;
	m_ready=0;
	m_virtualCommunicator=vc;
	m_inbox=inbox;

	m_initiatedIterator=false;
	ostringstream prefixFull;
	prefixFull<<m_parameters->getMemoryPrefix()<<"_Library";
	int chunkSize=4194304;
	m_allocator.constructor(chunkSize,RAY_MALLOC_TYPE_LIBRARY_ALLOCATOR,m_parameters->showMemoryAllocations());
	m_completedJobs=0;
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
	m_bufferedData.constructor(m_size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
		RAY_MALLOC_TYPE_LIBRARY_BUFFERS,m_parameters->showMemoryAllocations(),3);
	(m_libraryIndexInitiated)=false;
	(m_libraryIterator)=0;
	for(map<int,map<int,int> >::iterator i=m_libraryDistances.begin();
		i!=m_libraryDistances.end();i++){
		int libraryName=i->first;
		m_libraryIndexes.push_back(libraryName);
	}
}

void Library::sendLibraryDistances(){
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
		uint64_t workerId=m_workersDone[i];
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
		uint64_t workerId=m_waitingWorkers[i];
		#ifdef ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		#endif
		m_activeWorkers.erase(workerId);
	}
	m_waitingWorkers.clear();

	for(int i=0;i<(int)m_activeWorkersToRestore.size();i++){
		uint64_t workerId=m_activeWorkersToRestore[i];
		m_activeWorkers.insert(workerId);
	}
	m_activeWorkersToRestore.clear();

	m_virtualCommunicator->resetGlobalPushedMessageStatus();
}
