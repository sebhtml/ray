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

#include <core/constants.h>
#include <communication/VirtualCommunicator.h>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <assembler/SeedingData.h>
#include <communication/Message.h>
#include <mpi.h>
#include <communication/mpi_tags.h>
#include <assembler/SeedWorker.h>

bool myComparator_sort(const vector<Kmer>&a,const vector<Kmer>&b){
	return a.size()>b.size();
}

void SeedingData::computeSeeds(){
	if(!m_initiatedIterator){
		m_last=time(NULL);

		m_SEEDING_i=0;

		m_activeWorkerIterator=m_activeWorkers.begin();
		m_splayTreeIterator.constructor(m_subgraph,m_wordSize,m_parameters);
		m_initiatedIterator=true;
		m_maximumAliveWorkers=30000;
		#ifdef ASSERT
		m_splayTreeIterator.hasNext();
		#endif
	}

	m_virtualCommunicator->processInbox(&m_activeWorkersToRestore);

	if(!m_virtualCommunicator->isReady()){
		return;
	}

	// flush all mode is necessary to empty buffers and 
	// restart things from scratch..

	// 1. iterate on active workers
	if(m_activeWorkerIterator!=m_activeWorkers.end()){
		uint64_t workerId=*m_activeWorkerIterator;
		#ifdef ASSERT
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
			vector<Kmer> seed=*(m_aliveWorkers[workerId].getSeed());

			int nucleotides=seed.size()+(m_wordSize)-1;
			// only consider the long ones.
			if(nucleotides>=m_parameters->getMinimumContigLength()){
				
				Kmer firstVertex=seed[0];
				Kmer lastVertex=seed[seed.size()-1];
				Kmer firstReverse=m_parameters->_complementVertex(&lastVertex);

				if(firstVertex<firstReverse){
					printf("Rank %i discovered a seed with %i vertices\n",m_rank,(int)seed.size());
					fflush(stdout);

					if(m_parameters->showMemoryUsage()){
						showMemoryUsage(m_rank);
					}
					m_SEEDING_seeds.push_back(seed);
				}
			}
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
			if(m_SEEDING_i<(uint64_t)m_subgraph->size()&&(int)m_aliveWorkers.size()<m_maximumAliveWorkers){
				if(m_SEEDING_i % 100000 ==0){
					printf("Rank %i is creating seeds [%i/%i]\n",getRank(),(int)m_SEEDING_i+1,(int)m_subgraph->size());
					fflush(stdout);

					if(m_parameters->showMemoryUsage()){
						showMemoryUsage(m_rank);
					}
				}
				#ifdef ASSERT
				if(m_SEEDING_i==0){
					assert(m_completedJobs==0&&m_activeWorkers.size()==0&&m_aliveWorkers.size()==0);
				}
				#endif
				Vertex*node=m_splayTreeIterator.next();
				Kmer vertexKey=*(m_splayTreeIterator.getKey());

				int coverage=node->getCoverage(&vertexKey);
				int minimum=5;
				if(coverage<minimum){
					m_completedJobs++;
				}else{
					m_aliveWorkers[m_SEEDING_i].constructor(&vertexKey,m_parameters,m_outboxAllocator,m_virtualCommunicator,m_SEEDING_i);
					m_activeWorkers.insert(m_SEEDING_i);
				}

				int population=m_aliveWorkers.size();
				if(population>m_maximumWorkers){
					m_maximumWorkers=population;
				}

				m_SEEDING_i++;

				// skip the reverse complement as we don't really need it anyway.
			}else{
				m_virtualCommunicator->forceFlush();
			}
		}

		// brace yourself for the next round
		m_activeWorkerIterator=m_activeWorkers.begin();
	}

	#ifdef ASSERT
	assert((int)m_aliveWorkers.size()<=m_maximumAliveWorkers);
	#endif

	if((int)m_subgraph->size()==m_completedJobs){
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		printf("Rank %i has %i seeds\n",m_rank,(int)m_SEEDING_seeds.size());
		fflush(stdout);
		printf("Rank %i is creating seeds [%i/%i] (completed)\n",getRank(),(int)m_SEEDING_i,(int)m_subgraph->size());
		fflush(stdout);
		printf("Rank %i: peak number of workers: %i, maximum: %i\n",m_rank,m_maximumWorkers,m_maximumAliveWorkers);
		fflush(stdout);
		m_virtualCommunicator->printStatistics();
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_SEEDING_IS_OVER,getRank());
		m_outbox->push_back(aMessage);

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_rank);
		}

		#ifdef ASSERT
		assert(m_aliveWorkers.size()==0);
		assert(m_activeWorkers.size()==0);
		#endif

		// sort the seeds by length
		std::sort(m_SEEDING_seeds.begin(),m_SEEDING_seeds.end(),myComparator_sort);
	}
}

void SeedingData::constructor(SeedExtender*seedExtender,int rank,int size,StaticVector*outbox,RingAllocator*outboxAllocator,
int*mode,
	Parameters*parameters,int*wordSize,GridTable*subgraph,StaticVector*inbox,
	VirtualCommunicator*vc){
	m_virtualCommunicator=vc;
	m_seedExtender=seedExtender;
	m_size=size;
	m_rank=rank;
	m_outbox=outbox;
	m_inbox=inbox;
	m_completedJobs=0;
	m_maximumWorkers=0;
	m_flushAllMode=false;
	m_outboxAllocator=outboxAllocator;
	m_mode=mode;
	m_parameters=parameters;
	m_wordSize=m_parameters->getWordSize();
	#ifdef ASSERT
	assert(m_wordSize>=15&&m_wordSize<=MAXKMERLENGTH);
	#endif
	m_subgraph=subgraph;
	m_initiatedIterator=false;
}

int SeedingData::getRank(){
	return m_rank;
}

int SeedingData::getSize(){
	return m_size;
}

void SeedingData::updateStates(){
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

void SeedingData::sendSeedLengths(){
	if(!m_initialized){
		for(int i=0;i<(int)m_SEEDING_seeds.size();i++){
			int length=m_SEEDING_seeds[i].size()+m_parameters->getWordSize()-1;
			m_slaveSeedLengths[length]++;
		}
		m_iterator=m_slaveSeedLengths.begin();
		m_initialized=true;
		m_communicatorWasTriggered=false;
	}

	if(m_inbox->size()==1&&(*m_inbox)[0]->getTag()==RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY)
		m_communicatorWasTriggered=false;
	
	if(m_communicatorWasTriggered)
		return;

	if(m_iterator==m_slaveSeedLengths.end()){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,
			RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS,getRank());
		m_outbox->push_back(aMessage);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		return;
	}
	
	uint64_t*messageBuffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int maximumPairs=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)/2;
	int i=0;
	while(i<maximumPairs && m_iterator!=m_slaveSeedLengths.end()){
		int length=m_iterator->first;
		int count=m_iterator->second;
		messageBuffer[2*i]=length;
		messageBuffer[2*i+1]=count;
		i++;
		m_iterator++;
	}

	Message aMessage(messageBuffer,2*i,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,
		RAY_MPI_TAG_SEND_SEED_LENGTHS,getRank());
	m_outbox->push_back(aMessage);
}

void SeedingData::writeSeedStatistics(){
	ostringstream file;
	file<<m_parameters->getPrefix();
	file<<".SeedLengthDistribution.txt";
	ofstream f(file.str().c_str());
	for(map<int,int>::iterator i=m_masterSeedLengths.begin();i!=m_masterSeedLengths.end();i++){
		int length=i->first;
		int count=i->second;
		f<<length<<"\t"<<count<<endl;
	}
	f.close();
}
