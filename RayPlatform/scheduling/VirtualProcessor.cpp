/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#include <scheduling/VirtualProcessor.h>
#include <core/ComputeCore.h>

#include <iostream>
using namespace std;

#ifdef ASSERT
#include <assert.h>
#endif

/* #define DEBUG_VIRTUAL_PROCESSOR */

void VirtualProcessor::updateStates(){
	#ifdef DEBUG_VIRTUAL_PROCESSOR
	cout<<"Removing "<<m_workersDone.size()<<" workers done."<<endl;
	#endif

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

	#ifdef DEBUG_VIRTUAL_PROCESSOR
	cout<<"Moving "<<m_waitingWorkers.size()<<" workers to sleep mode."<<endl;
	#endif

	/** make some worker sleep */
	for(int i=0;i<(int)m_waitingWorkers.size();i++){
		uint64_t workerId=m_waitingWorkers[i];

		#ifdef ASSERT
		assert(m_activeWorkers.count(workerId)>0);
		#endif

		m_activeWorkers.erase(workerId);
	}
	m_waitingWorkers.clear();

	#ifdef DEBUG_VIRTUAL_PROCESSOR
	cout<<"Moving "<<m_activeWorkersToRestore.size()<<" workers to active mode"<<endl;
	#endif

	/** wake up some workers */
	for(int i=0;i<(int)m_activeWorkersToRestore.size();i++){
		uint64_t workerId=m_activeWorkersToRestore[i];
		m_activeWorkers.insert(workerId);
	}

	m_activeWorkersToRestore.clear();

	m_virtualCommunicator->resetGlobalPushedMessageStatus();

	#ifdef DEBUG_VIRTUAL_PROCESSOR
	int activeWorkers=m_activeWorkers.size();
	int aliveWorkers=m_aliveWorkers.size();
	int sleepingWorkers=aliveWorkers-activeWorkers;
	cout<<"VirtualProcessor statistics: active: "<<activeWorkers<<" sleeping: "<<sleepingWorkers<<" total: "<<aliveWorkers<<endl;
	#endif
}

/** actually initialize the VirtualProcessor */
void VirtualProcessor::constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,
		VirtualCommunicator*vc){

	#ifdef DEBUG_VIRTUAL_PROCESSOR
	int rank=parameters->getRank();
	cout<<"Rank "<<rank<<" Initializing VirtualProcessor"<<endl;
	#endif

	m_virtualCommunicator=vc;
	m_inbox=inbox;
	m_outboxAllocator=outboxAllocator;

	/** the maximum number of workers on this VirtualProcessor */
	m_maximumAliveWorkers=32768; /* 2**15 */

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

		/* make the worker work a little bit */
		m_aliveWorkers[workerId]->work();

		//force the worker to work until he finishes or pushes something on the stack
		/*
		while(!m_aliveWorkers[workerId]->isDone()&&!m_virtualCommunicator->getLocalPushedMessageStatus()){
		}
		*/

		if(m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_waitingWorkers.push_back(workerId);
		}else if(m_aliveWorkers[workerId]->isDone()){
			m_workersDone.push_back(workerId);
		} 

		m_activeWorkerIterator++;

		return true;
	}else{
		updateStates();

		//  add one worker to active workers
		//  reason is that those already in the pool don't communicate anymore -- 
		//  as for they need responses.
		if(!m_virtualCommunicator->getGlobalPushedMessageStatus() && m_activeWorkers.empty()){
			/** if no more worker will be added, we need to forceFlush */

			bool noMoreWorkerCanBeAdded=false;

			/* we have a full array of idle workers */
			if(!canAddWorker()){
				noMoreWorkerCanBeAdded=true;
			}

			/* we have space to add further workers, but no workers will be added in the future */
			if(canAddWorker() && !m_moreTasksAreComing){
				noMoreWorkerCanBeAdded=true;
			}

			/* we have to force-flush something to get rid of these endless waiting */
			if(noMoreWorkerCanBeAdded){
				#ifdef DEBUG_VIRTUAL_PROCESSOR
				cout<<"VirtualProcessor: calling forceFlush on VirtualCommunicator"<<endl;
				#endif

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

	m_virtualCommunicator->printStatistics();
}

void VirtualProcessor::registerPlugin(ComputeCore*core){
	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"VirtualProcessor");
	core->setPluginDescription(m_plugin,"A thread pool running on 1 physical thread (bundled with RayPlatform)");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU Lesser General License version 3");

}

void VirtualProcessor::resolveSymbols(ComputeCore*core){

}
