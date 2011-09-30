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
#include <core/OperatingSystem.h>
#include <memory/malloc_types.h>

//#define DEBUG_EdgePurger

void EdgePurger::constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
		int*slaveMode,int*masterMode,VirtualCommunicator*vc,GridTable*subgraph,
	VirtualProcessor*virtualProcessor){
	m_checkedCheckpoint=false;
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

	/* for TaskCreator */
	m_initialized=false;
	m_virtualProcessor=virtualProcessor;
}

void EdgePurger::work(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	/* master control */
	if(m_inbox->size()>0&&m_inbox->at(0)->getTag()==RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY){
		m_masterCountFinished++;
		//cout<<"Receiving RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY"<<endl;
		if(m_masterCountFinished==m_parameters->getSize()){
			(*m_masterMode)=RAY_MASTER_MODE_WRITE_KMERS;
		}
	}
	if(m_done){
		return;
	}

	MACRO_COLLECT_PROFILING_INFORMATION();
	if(!m_checkedCheckpoint){
		if(m_parameters->hasCheckpoint("GenomeGraph")){
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY,m_parameters->getRank());
			m_outbox->push_back(aMessage);
			m_done=true;
			return;
		}
		m_checkedCheckpoint=true;
	}

	MACRO_COLLECT_PROFILING_INFORMATION();

	mainLoop();

	MACRO_COLLECT_PROFILING_INFORMATION();
}

void EdgePurger::finalizeMethod(){
	printf("Rank %i is purging edges [%i/%i] (completed)\n",m_parameters->getRank(),(int)m_subgraph->size(),(int)m_subgraph->size());
	fflush(stdout);
	
	m_done=true;
	Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY,
	m_parameters->getRank());
	m_outbox->push_back(aMessage);

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
	}
}

bool EdgePurger::hasUnassignedTask(){
	#ifdef DEBUG_EdgePurger
	cout<<"hasUnassignedTask "<<m_SEEDING_i<<" "<<m_subgraph->size<<endl;
	#endif
	return m_SEEDING_i<m_subgraph->size();
}

Worker* EdgePurger::assignNextTask(){
	#ifdef DEBUG_EdgePurger
	cout<<"EdgePurger::assignNextTask"<<endl;
	#endif

	#ifdef ASSERT
	assert(m_graphIterator.hasNext());
	#endif

	Vertex*vertex=m_graphIterator.next();
	Kmer*currentKmer=m_graphIterator.getKey();

	//EdgePurgerWorker*worker=(EdgePurgerWorker*)m_workerAllocator.allocate(sizeof(EdgePurgerWorker));
	EdgePurgerWorker*worker=new EdgePurgerWorker;
	worker->constructor(m_SEEDING_i,vertex,currentKmer,m_subgraph,m_virtualCommunicator,m_outboxAllocator,m_parameters,m_inbox,m_outbox);

	m_SEEDING_i++;

	#ifdef ASSERT
	assert(worker != NULL);
	#endif

	return worker;
}

void EdgePurger::processWorkerResult(Worker*){
	/* nothing to do */
	if(m_completedJobs%50000==0){
		cout<<"Rank "<<m_parameters->getRank()<<" is purging edges ["<<m_completedJobs+1;
		cout<<"/"<<m_subgraph->size()<<"]"<<endl;
		cout.flush();
	}
}

void EdgePurger::destroyWorker(Worker*worker){
	delete worker;
}

void EdgePurger::initializeMethod(){
	#ifdef DEBUG_EdgePurger
	cout<<"EdgePurger::initializeMethod"<<endl;
	#endif

	m_SEEDING_i=0;
	m_graphIterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	#ifdef DEBUG_EdgePurger
	cout<<"Will process "<<m_subgraph->size()<<endl;
	cout<<"Exiting initializeMethod"<<endl;
	#endif
}

void EdgePurger::setProfiler(Profiler*profiler){
	m_profiler = profiler;
}
