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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <plugin_EdgePurger/EdgePurger.h>
#include <stdlib.h>
#include <stdio.h>
#include <core/OperatingSystem.h>

__CreatePlugin(EdgePurger);

 /**/
 /**/
__CreateSlaveModeAdapter(EdgePurger,RAY_SLAVE_MODE_PURGE_NULL_EDGES); /**/
 /**/
 /**/


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

void EdgePurger::call_RAY_SLAVE_MODE_PURGE_NULL_EDGES(){

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

	m_derivative.writeFile(&cout);

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
	worker->constructor(m_SEEDING_i,vertex,currentKmer,m_subgraph,m_virtualCommunicator,m_outboxAllocator,m_parameters,m_inbox,m_outbox,
		RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE);

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

		m_derivative.addX(m_completedJobs);
		m_derivative.printStatus(SLAVE_MODES[RAY_SLAVE_MODE_PURGE_NULL_EDGES],RAY_SLAVE_MODE_PURGE_NULL_EDGES);
		m_derivative.printEstimatedTime(m_subgraph->size());
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

void EdgePurger::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();

	m_plugin=plugin;

	core->setPluginName(plugin,"EdgePurger");
	core->setPluginDescription(plugin,"Deals with invalid edges");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_SLAVE_MODE_PURGE_NULL_EDGES=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_PURGE_NULL_EDGES, __GetAdapter(EdgePurger,RAY_SLAVE_MODE_PURGE_NULL_EDGES));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_PURGE_NULL_EDGES,"RAY_SLAVE_MODE_PURGE_NULL_EDGES");

	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY");

	RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY,"RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY");


}

void EdgePurger::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_PURGE_NULL_EDGES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_PURGE_NULL_EDGES");
	RAY_MASTER_MODE_WRITE_KMERS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_WRITE_KMERS");

	RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE");

	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY");

	__BindPlugin(EdgePurger);
}
