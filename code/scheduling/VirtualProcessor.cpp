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

#include <scheduling/VirtualProcessor.h>
#ifdef ASSERT
#include <assert.h>
#endif

#define DEBUG_VIRTUAL_PROCESSOR

void VirtualProcessor::updateStates(){

	// erase completed jobs
	for(int i=0;i<(int)m_workersDone.size();i++){
		uint64_t workerId=m_workersDone[i];

		#ifdef ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		assert(m_aliveWorkers.count(workerId)>0);
		#endif

		m_activeWorkers.erase(workerId);
		m_aliveWorkers.erase(workerId);

		m_completedJobs++;
	}
	m_workersDone.clear();

	/** make some worker sleep */
	for(int i=0;i<(int)m_waitingWorkers.size();i++){
		uint64_t workerId=m_waitingWorkers[i];

		#ifdef ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		#endif

		m_activeWorkers.erase(workerId);
	}
	m_waitingWorkers.clear();

	/** wake up some workers */
	for(int i=0;i<(int)m_activeWorkersToRestore.size();i++){
		uint64_t workerId=m_activeWorkersToRestore[i];
		m_activeWorkers.insert(workerId);
	}

	m_activeWorkersToRestore.clear();

	m_virtualCommunicator->resetGlobalPushedMessageStatus();
}

/** actually initialize the VirtualProcessor */
void VirtualProcessor::constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
		VirtualCommunicator*vc){

	int rank=parameters->getRank();
	cout<<"Rank "<<rank<<" Initializing VirtualProcessor"<<endl;
	m_virtualCommunicator=vc;
	m_inbox=inbox;
	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;

	/** the maximum number of workers on this VirtualProcessor */
	m_maximumAliveWorkers=30000;

	reset();
}

/** make the VirtualProcessor run a little bit */
bool VirtualProcessor::run(){

	if(!m_initiatedIterator){

		m_activeWorkerIterator=m_activeWorkers.begin();
		m_initiatedIterator=true;
	}

	/** fetch the message from the inbox and de-multiplex it if necessary */
	m_virtualCommunicator->processInbox(&m_activeWorkersToRestore);

	/** if the VirtualCommunicator is not ready, return immediately */
	if(!m_virtualCommunicator->isReady()){
		return false;
	}

	/** make the current worker work */
	if(m_activeWorkerIterator!=m_activeWorkers.end()){
		uint64_t workerId=*m_activeWorkerIterator;

		/** save the current worker identifier */
		m_currentWorker=workerId;

		#ifdef ASSERT
		if(m_aliveWorkers.count(workerId)==0){
			cout<<"Error: "<<workerId<<" is not in alive workers "<<m_activeWorkers.size()<<endl;
		}
		assert(m_aliveWorkers.count(workerId)>0);
		assert(!m_aliveWorkers[workerId]->isDone());
		#endif
		m_virtualCommunicator->resetLocalPushedMessageStatus();

		//force the worker to work until he finishes or pushes something on the stack
		while(!m_aliveWorkers[workerId]->isDone()&&!m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_aliveWorkers[workerId]->work();
		}

		if(m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_waitingWorkers.push_back(workerId);
		}
		if(m_aliveWorkers[workerId]->isDone()){
			m_workersDone.push_back(workerId);
		}
		m_activeWorkerIterator++;

		return true;
	}else{
		updateStates();

		//  add one worker to active workers
		//  reason is that those already in the pool don't communicate anymore -- 
		//  as for they need responses.
		if(!m_virtualCommunicator->getGlobalPushedMessageStatus()&&m_activeWorkers.empty()){
			/** if no more worker will be added, we need to forceFlush */
			if(!m_moreTasksAreComing){
				m_virtualCommunicator->forceFlush();
			}
		}

		m_activeWorkerIterator=m_activeWorkers.begin();

		return false;
	}

	return false;
}

/** get the current worker */
Worker*VirtualProcessor::getCurrentWorker(){
	#ifdef ASSERT
	assert(m_aliveWorkers.count(m_currentWorker) > 0);
	#endif

	return m_aliveWorkers[m_currentWorker];
}

/** can a worker be added ? */
bool VirtualProcessor::canAddWorker(){
	return (int)m_aliveWorkers.size() < m_maximumAliveWorkers;
}

/** add a worker */
void VirtualProcessor::addWorker(Worker*worker){
	#ifdef ASSERT
	assert(worker != NULL);
	#endif

	uint64_t workerId=worker->getWorkerIdentifier();

	#ifdef ASSERT
	if(m_aliveWorkers.count(workerId)>0)
		cout<<"Worker "<<workerId<<" already here"<<endl;
	assert(m_aliveWorkers.count(workerId) == 0);
	#endif

	m_aliveWorkers[workerId]=worker;
	m_activeWorkers.insert(workerId);

	int population=m_aliveWorkers.size();
	if(population>m_maximumWorkers){
		m_maximumWorkers=population;
	}
}

void VirtualProcessor::noMoreTasksAreComing(){
	m_moreTasksAreComing=false;
}

void VirtualProcessor::reset(){
	m_moreTasksAreComing=true;
	m_initiatedIterator=false;
	m_completedJobs=0;
}

bool VirtualProcessor::hasWorkToDo(){
	return m_aliveWorkers.size()>0;
}

void VirtualProcessor::printStatistics(){
	cout<<"VirtualProcessor: completed jobs: "<<m_completedJobs<<endl;
}
