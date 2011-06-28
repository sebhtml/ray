/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#include <assembler/EdgePurger.h>
#include <stdlib.h>
#include <stdio.h>

/* code based on assembler/Library.cpp */

void EdgePurger::constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
		int*slaveMode,int*masterMode,VirtualCommunicator*vc,GridTable*subgraph){
	m_subgraph=subgraph;
	m_masterMode=masterMode;
	m_slaveMode=slaveMode;
	m_virtualCommunicator=vc;
	m_outbox=outbox;
	m_inbox=inbox;
	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;
	m_masterCountFinished=0;
	m_done=false;
	m_initiatedIterator=false;
	m_completedJobs=0;
}

void EdgePurger::work(){
	/* master control */
	if(m_inbox->size()>0&&m_inbox->at(0)->getTag()==RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY){
		m_masterCountFinished++;
		if(m_masterCountFinished==m_parameters->getSize()){
			*m_masterMode=RAY_MASTER_MODE_WRITE_KMERS;
		}
	}
	if(m_done){
		return;
	}

	if(!m_initiatedIterator){
		m_SEEDING_i=0;
		m_graphIterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

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
			if(m_SEEDING_i<m_subgraph->size()&&(int)m_aliveWorkers.size()<m_maximumAliveWorkers){

				#ifdef ASSERT
				if(m_SEEDING_i==0){
					assert(m_completedJobs==0&&m_activeWorkers.size()==0&&m_aliveWorkers.size()==0);
				}
				#endif
				
				Vertex*vertex=m_graphIterator.next();
				Kmer*currentKmer=m_graphIterator.getKey();

				m_aliveWorkers[m_SEEDING_i].constructor(m_SEEDING_i,vertex,currentKmer,m_subgraph,m_virtualCommunicator,m_outboxAllocator,m_parameters,m_inbox,m_outbox);
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

	if(m_completedJobs==(int)m_subgraph->size()){
		printf("Rank %i is purging edges [%i/%i] (completed)\n",m_parameters->getRank(),(int)m_subgraph->size(),(int)m_subgraph->size());
		fflush(stdout);
		
		m_done=true;
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY,
		m_parameters->getRank());
		m_outbox->push_back(aMessage);

		printf("Rank %i: peak number of workers: %i, maximum: %i\n",m_parameters->getRank(),m_maximumWorkers,m_maximumAliveWorkers);
		fflush(stdout);
		m_virtualCommunicator->printStatistics();

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_parameters->getRank());
		}
	}
}

void EdgePurger::updateStates(){
	// erase completed jobs
	for(int i=0;i<(int)m_workersDone.size();i++){
		uint64_t workerId=m_workersDone[i];
		#ifdef ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		assert(m_aliveWorkers.count(workerId)>0);
		#endif
		m_activeWorkers.erase(workerId);
		m_aliveWorkers.erase(workerId);
		if(m_completedJobs%50000==0){
			printf("Rank %i is purging edges [%i/%i]\n",m_parameters->getRank(),m_completedJobs+1,(int)m_subgraph->size());
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
