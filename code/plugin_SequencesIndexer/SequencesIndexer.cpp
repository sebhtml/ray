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

#include <plugin_SequencesIndexer/SequencesIndexer.h>
#include <string.h>
#include <core/OperatingSystem.h>
#include <stdlib.h>
#include <assert.h>
#include <application_core/Parameters.h>
#include <plugin_SequencesLoader/Loader.h>
#include <application_core/common_functions.h>
#include <communication/Message.h>

__CreatePlugin(SequencesIndexer);

 /**/
 /**/
__CreateSlaveModeAdapter(SequencesIndexer,RAY_SLAVE_MODE_INDEX_SEQUENCES); /**/
 /**/
 /**/


void SequencesIndexer::call_RAY_SLAVE_MODE_INDEX_SEQUENCES(){
	if(!m_initiatedIterator){
		m_theSequenceId=0;

		m_activeWorkerIterator.constructor(&m_activeWorkers);
		m_initiatedIterator=true;
		m_maximumAliveWorkers=32768;


		m_virtualCommunicator->resetCounters();
	}

	if(!m_checkedCheckpoint){
		if(m_parameters->hasCheckpoint("OptimalMarkers") && m_parameters->hasCheckpoint("ReadOffsets")){
			cout<<"Rank "<<m_parameters->getRank()<<": checkpoint OptimalMarkers exists, not selecting markers."<<endl;
			(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,m_rank);
			m_outbox->push_back(aMessage);
			return;
		}
		m_checkedCheckpoint=true;
	}

	m_virtualCommunicator->processInbox(&m_activeWorkersToRestore);

	if(!m_virtualCommunicator->isReady()){
		return;
	}

	if(m_activeWorkerIterator.hasNext()){
		WorkerHandle workerId=m_activeWorkerIterator.next()->getKey();

		#ifdef ASSERT
		assert(m_aliveWorkers.find(workerId,false)!=NULL);
		assert(!m_aliveWorkers.find(workerId,false)->getValue()->isDone());
		#endif
		m_virtualCommunicator->resetLocalPushedMessageStatus();

		//force the worker to work until he finishes or pushes something on the stack
		while(!m_aliveWorkers.find(workerId,false)->getValue()->isDone()&&!m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_aliveWorkers.find(workerId,false)->getValue()->work();
		}

		if(m_virtualCommunicator->getLocalPushedMessageStatus()){
			m_waitingWorkers.push_back(workerId);
		}
		if(m_aliveWorkers.find(workerId,false)->getValue()->isDone()){
			m_workersDone.push_back(workerId);
		}
	}else{
		updateStates();

		//  add one worker to active workers
		//  reason is that those already in the pool don't communicate anymore -- 
		//  as for they need responses.
		if(!m_virtualCommunicator->getGlobalPushedMessageStatus()&&m_activeWorkers.size()==0){
			// there is at least one worker to start
			// AND
			// the number of alive workers is below the maximum
			if(m_theSequenceId<(int)m_myReads->size()&&(int)m_aliveWorkers.size()<m_maximumAliveWorkers){
				if(m_theSequenceId%10000==0){
					printf("Rank %i is selecting optimal read markers [%i/%i]\n",m_rank,m_theSequenceId+1,(int)m_myReads->size());
					fflush(stdout);

					m_derivative.addX(m_theSequenceId);
					m_derivative.printStatus(SLAVE_MODES[RAY_SLAVE_MODE_INDEX_SEQUENCES],RAY_SLAVE_MODE_INDEX_SEQUENCES);
					m_derivative.printEstimatedTime(m_myReads->size());

					if(m_parameters->showMemoryUsage())
						showMemoryUsage(m_rank);
				}

				#ifdef ASSERT
				if(m_theSequenceId==0){
					assert(m_completedJobs==0&&m_activeWorkers.size()==0&&m_aliveWorkers.size()==0);
				}
				assert(m_theSequenceId<(int)m_myReads->size());
				#endif


				bool flag;
				m_aliveWorkers.insert(m_theSequenceId,&m_workAllocator,&flag)->getValue()->constructor(m_theSequenceId,m_parameters,m_outboxAllocator,m_virtualCommunicator,
					m_theSequenceId,m_myReads,&m_workAllocator,&m_readMarkerFile,&m_forwardStatistics,
					&m_reverseStatistics,
	RAY_MPI_TAG_ATTACH_SEQUENCE,
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE
);

				m_activeWorkers.insert(m_theSequenceId,&m_workAllocator,&flag);
				int population=m_aliveWorkers.size();
				if(population>m_maximumWorkers){
					m_maximumWorkers=population;
				}

				m_theSequenceId++;
			}else{
				m_virtualCommunicator->forceFlush();
			}
		}

		m_activeWorkerIterator.constructor(&m_activeWorkers);
	}

	#ifdef ASSERT
	assert((int)m_aliveWorkers.size()<=m_maximumAliveWorkers);
	#endif

	if((int)m_myReads->size()==m_completedJobs){
		printf("Rank %i is selecting optimal read markers [%i/%i] (completed)\n",m_rank,(int)m_myReads->size(),(int)m_myReads->size());
		fflush(stdout);
		printf("Rank %i: peak number of workers: %i, maximum: %i\n",m_rank,m_maximumWorkers,m_maximumAliveWorkers);
		fflush(stdout);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,m_rank);
		m_outbox->push_back(aMessage);

		m_derivative.writeFile(&cout);

		m_virtualCommunicator->printStatistics();

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_rank);
		}

		#ifdef ASSERT
		assert(m_aliveWorkers.size()==0);
		assert(m_activeWorkers.size()==0);
		#endif

		int freed=m_workAllocator.getNumberOfChunks()*m_workAllocator.getChunkSize();
		m_workAllocator.clear();

		if(m_parameters->showMemoryUsage()){
			cout<<"Rank "<<m_parameters->getRank()<<": Freeing unused assembler memory: "<<freed/1024<<" KiB freed"<<endl;
			showMemoryUsage(m_rank);
		}

		if(m_parameters->hasOption("-write-read-markers")){
			m_readMarkerFile.close();
		}

		if(m_parameters->hasOption("-write-marker-summary")){

			ostringstream file1;
			file1<<m_parameters->getPrefix()<<"Rank"<<m_parameters->getRank()<<".ForwardMarkerSummary.txt";
			string fileName1=file1.str();
			ofstream f1(fileName1.c_str());

			for(map<int,map<int,int> >::iterator i=m_forwardStatistics.begin();i!=m_forwardStatistics.end();i++){
				int offset=i->first;
				for(map<int,int>::iterator j=i->second.begin();j!=i->second.end();j++){
					int coverage=j->first;
					int count=j->second;
					f1<<offset<<"	"<<coverage<<"	"<<count<<endl;
				}
			}
			f1.close();

			ostringstream file2;
			file2<<m_parameters->getPrefix()<<"Rank"<<m_parameters->getRank()<<".ReverseMarkerSummary.txt";
			string fileName2=file2.str();
			ofstream f2(fileName2.c_str());

			for(map<int,map<int,int> >::iterator i=m_reverseStatistics.begin();i!=m_reverseStatistics.end();i++){
				int offset=i->first;
				for(map<int,int>::iterator j=i->second.begin();j!=i->second.end();j++){
					int coverage=j->first;
					int count=j->second;
					f2<<offset<<"	"<<coverage<<"	"<<count<<endl;
				}
			}
			f2.close();

		}

		m_forwardStatistics.clear();
		m_reverseStatistics.clear();
	}
}

void SequencesIndexer::constructor(Parameters*parameters,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,VirtualCommunicator*vc,
SlaveMode*mode,
	ArrayOfReads*myReads
){
	m_mode=mode;
	m_outboxAllocator=outboxAllocator;
	m_myReads=myReads;
	m_outbox=outbox;

	m_parameters=parameters;
	m_checkedCheckpoint=false;

	int chunkSize=4194304; // 4 MiB
	m_allocator.constructor(chunkSize,"RAY_MALLOC_TYPE_OPTIMAL_READ_MARKERS",
		m_parameters->showMemoryAllocations());

	m_workAllocator.constructor(chunkSize,"RAY_MALLOC_TYPE_OPTIMAL_READ_MARKER_WORKERS",
		m_parameters->showMemoryAllocations());

	m_initiatedIterator=false;
	m_rank=parameters->getRank();
	m_size=parameters->getSize();
	m_pendingMessages=0;

	m_completedJobs=0;
	m_maximumWorkers=0;
	m_theSequenceId=0;
	m_virtualCommunicator=vc;

	if(m_parameters->hasOption("-write-read-markers")){
		ostringstream file;
		file<<m_parameters->getPrefix()<<"Rank"<<m_parameters->getRank()<<".OptimalReadMarkers.txt";
		string fileName=file.str();
		m_readMarkerFile.open(fileName.c_str());
	}
}

void SequencesIndexer::setReadiness(){
	m_pendingMessages--;
}

MyAllocator*SequencesIndexer::getAllocator(){
	return &m_allocator;
}

void SequencesIndexer::updateStates(){
	// erase completed jobs
	for(int i=0;i<(int)m_workersDone.size();i++){
		WorkerHandle workerId=m_workersDone[i];
		#ifdef ASSERT
		assert(m_activeWorkers.find(workerId,false)!=NULL);
		assert(m_aliveWorkers.find(workerId,false)!=NULL);
		#endif
		m_activeWorkers.remove(workerId,true,&m_workAllocator);
		m_aliveWorkers.remove(workerId,true,&m_workAllocator);
		m_completedJobs++;
	}
	m_workersDone.clear();

	for(int i=0;i<(int)m_waitingWorkers.size();i++){
		WorkerHandle workerId=m_waitingWorkers[i];
		#ifdef ASSERT
		assert(m_activeWorkers.find(workerId,false)!=NULL);
		#endif
		m_activeWorkers.remove(workerId,true,&m_workAllocator);
	}
	m_waitingWorkers.clear();

	for(int i=0;i<(int)m_activeWorkersToRestore.size();i++){
		bool flag;
		WorkerHandle workerId=m_activeWorkersToRestore[i];
		m_activeWorkers.insert(workerId,&m_workAllocator,&flag);
	}
	m_activeWorkersToRestore.clear();

	m_virtualCommunicator->resetGlobalPushedMessageStatus();
}

void SequencesIndexer::registerPlugin(ComputeCore*core){

	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"SequencesIndexer");
	core->setPluginDescription(plugin,"Index the DNA readouts");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_SLAVE_MODE_INDEX_SEQUENCES=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_INDEX_SEQUENCES,__GetAdapter(SequencesIndexer,RAY_SLAVE_MODE_INDEX_SEQUENCES));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_INDEX_SEQUENCES,"RAY_SLAVE_MODE_INDEX_SEQUENCES");

	RAY_MPI_TAG_GET_READ_MARKERS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_READ_MARKERS_REPLY,"RAY_MPI_TAG_GET_READ_MARKERS_REPLY");

	RAY_MPI_TAG_GET_READ_MATE_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_READ_MATE_REPLY,"RAY_MPI_TAG_GET_READ_MATE_REPLY");

	RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY,"RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY");

	RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY,"RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY");

	RAY_MPI_TAG_VERTEX_READS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTEX_READS_REPLY,"RAY_MPI_TAG_VERTEX_READS_REPLY");

}

void SequencesIndexer::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_INDEX_SEQUENCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_INDEX_SEQUENCES");
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");

	RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY");

	RAY_MPI_TAG_ATTACH_SEQUENCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ATTACH_SEQUENCE");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE");

	RAY_MPI_TAG_GET_READ_MARKERS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MARKERS_REPLY");
	RAY_MPI_TAG_GET_READ_MATE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MATE_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY");
	RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY");
	RAY_MPI_TAG_VERTEX_READS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_READS_REPLY");

	__BindPlugin(SequencesIndexer);
}
