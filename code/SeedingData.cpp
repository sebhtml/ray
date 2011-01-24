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

#include <VirtualCommunicator.h>
#include <assert.h>
#include <SeedingData.h>
#include <Message.h>
#include <mpi.h>
#include <mpi_tags.h>
#include <SeedWorker.h>

void SeedingData::computeSeeds(){
	if(!m_initiatedIterator){
		m_last=time(NULL);
		#ifdef ASSERT
		SplayTreeIterator<uint64_t,Vertex> iter0;
		iter0.constructor(m_subgraph->getTree(0));
		int n=m_subgraph->getTree(0)->size();
		int oo=0;
		while(iter0.hasNext()){
			iter0.next();
			oo++;
		}
		assert(n==oo);
		//cout<<"N="<<n<<endl;
		#endif

		m_SEEDING_i=0;

		#ifdef ASSERT
		assert(!m_splayTreeIterator.hasNext());
		#endif

		m_activeWorkerIterator=m_activeWorkers.begin();
		m_splayTreeIterator.constructor(m_subgraph);
		m_initiatedIterator=true;
		m_completedJobs=0;
		m_maximumAliveWorker=m_size*100;
		#ifdef ASSERT
		m_splayTreeIterator.hasNext();
		#endif
	}

	m_virtualCommunicator.processInbox(&m_activeWorkersToRestore);

	if(!m_virtualCommunicator.isReady()){
		return;
	}

	// 1. iterate on active workers
	if(m_activeWorkerIterator!=m_activeWorkers.end()){
		uint64_t workerId=*m_activeWorkerIterator;
		#ifdef ASSERT
		assert(m_aliveWorkers.count(workerId)>0);
		assert(!m_aliveWorkers[workerId].isDone());
		#endif
		m_virtualCommunicator.resetLocalPushedMessageStatus();

		//cout<<"Rank "<<m_rank<<" Worker="<<workerId<<" work()"<<endl;
		//
		//force the worker to work until he finishes or pushes something on the stack
		while(!m_aliveWorkers[workerId].isDone()&&!m_virtualCommunicator.getLocalPushedMessageStatus()){
			m_aliveWorkers[workerId].work();
		}

		if(m_virtualCommunicator.getLocalPushedMessageStatus()){
			m_waitingWorkers.push_back(workerId);
		}
		if(m_aliveWorkers[workerId].isDone()){
			m_workersDone.push_back(workerId);
			vector<uint64_t> seed=m_aliveWorkers[workerId].getSeed();

			int nucleotides=seed.size()+(m_wordSize)-1;
			// only consider the long ones.
			if(nucleotides>=m_parameters->getMinimumContigLength()){
	
				// if both seeds are on the same rank
				// dump the reverse and keep the forward
				/*
				printf("Rank %i added a seed with %i vertices\n",m_rank,(int)seed.size());
				fflush(stdout);
				showMemoryUsage(m_rank);
				*/
				m_SEEDING_seeds.push_back(seed);

				uint64_t firstVertex=seed[0];
				uint64_t lastVertex=seed[seed.size()-1];
				uint64_t lastVertexReverse=complementVertex(lastVertex,(m_wordSize),(*m_colorSpaceMode));
				int aRank=vertexRank(firstVertex,getSize());
				int bRank=vertexRank(lastVertexReverse,getSize());

				if(aRank==bRank){
					if(m_seedExtender->getEliminatedSeeds()->count(firstVertex)==0 && m_seedExtender->getEliminatedSeeds()->count(lastVertexReverse)==0){
						m_seedExtender->getEliminatedSeeds()->insert(firstVertex);
						//m_SEEDING_seeds.push_back(m_SEEDING_seed);
					}
				// if they are on two ranks,
				// keep the one on the rank with the lower number.
				}else if((aRank+bRank)%2==0 && aRank<bRank){
					m_seedExtender->getEliminatedSeeds()->insert(firstVertex);
				}else if(((aRank+bRank)%2==1 && aRank>bRank)){
					m_seedExtender->getEliminatedSeeds()->insert(firstVertex);
				}
			}
		}
		m_activeWorkerIterator++;
	}else{
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
			//cout<<"Rank "<<m_rank<<" Worker="<<workerId<<" SET STATE SLEEPY"<<endl;
		}
		m_waitingWorkers.clear();

		for(int i=0;i<(int)m_activeWorkersToRestore.size();i++){
			uint64_t workerId=m_activeWorkersToRestore[i];
			m_activeWorkers.insert(workerId);
			//cout<<"Rank "<<m_rank<<" Worker="<<workerId<<" SET STATE ACTIVE"<<endl;
		}
		m_activeWorkersToRestore.clear();

		//  add one worker to active workers
		//  reason is that those already in the pool don't communicate anymore -- 
		//  as for they need responses.
		if(!m_virtualCommunicator.getGlobalPushedMessageStatus()){
			// there is at least one worker to start
			// AND
			// the number of alive workers is below the maximum
			if(m_SEEDING_i<(uint64_t)m_subgraph->size()&&(int)m_aliveWorkers.size()<m_maximumAliveWorker){
				if(m_SEEDING_i % 100000 ==0){
					printf("Rank %i is creating seeds [%i/%i]\n",getRank(),(int)m_SEEDING_i+1,(int)m_subgraph->size());
					fflush(stdout);
					showMemoryUsage(m_rank);
				}
				#ifdef ASSERT
				if(m_SEEDING_i==0){
					assert(m_completedJobs==0&&m_activeWorkers.size()==0&&m_aliveWorkers.size()==0);
				}
				#endif
				SplayNode<uint64_t,Vertex>*node=m_splayTreeIterator.next();
				m_aliveWorkers[m_SEEDING_i].constructor(node->getKey(),m_parameters,m_outboxAllocator,&m_virtualCommunicator,m_SEEDING_i);
				m_activeWorkers.insert(m_SEEDING_i);

				//if(m_SEEDING_i%10000==0){
					//cout<<"Rank "<<m_rank<<" Adding worker WorkerId="<<m_SEEDING_i<<" ActiveWorkers="<<m_activeWorkers.size()<<" AliveWorkers="<<m_aliveWorkers.size()<<" CompletedJobs="<<m_completedJobs<<endl;
				//}

				m_SEEDING_i++;
/*
				if(m_SEEDING_i==(uint64_t)m_subgraph->size()){
					cout<<"Rank "<<m_rank<<" ActiveWorkers="<<m_activeWorkers.size()<<" AliveWorkers="<<m_aliveWorkers.size()<<" CompletedJobs="<<m_completedJobs<<"/"<<m_subgraph->size()<<endl;
					cout<<"Rank "<<m_rank<<": no more workers to start."<<endl;
					cout.flush();
				}
*/

			}else{
				//cout<<"Rank "<<m_rank<<" forceFlush()"<<endl;
				// no worker to start OR the maximum is reached
				// must flush buffers manually because no more workers are to be created
				m_virtualCommunicator.forceFlush(m_SEEDING_i==(uint64_t)m_subgraph->size());

			}
		}

		//cout<<"Rank "<<m_rank<<" RestartingIterator."<<endl;
		//cout<<"Rank "<<m_rank<<" ActiveWorkers="<<m_activeWorkers.size()<<" AliveWorkers="<<m_aliveWorkers.size()<<" CompletedJobs="<<m_completedJobs<<"/"<<m_subgraph->size()<<endl;

		// brace yourself for the next round
		m_activeWorkerIterator=m_activeWorkers.begin();
		m_virtualCommunicator.resetGlobalPushedMessageStatus();
	}

	#ifdef ASSERT
	assert((int)m_aliveWorkers.size()<=m_maximumAliveWorker);
	#endif

	if((int)m_subgraph->size()==m_completedJobs){
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		printf("Rank %i is creating seeds [%i/%i] (completed)\n",getRank(),(int)m_SEEDING_i,(int)m_subgraph->size());
		fflush(stdout);
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_SEEDING_IS_OVER,getRank());
		m_outbox->push_back(aMessage);

		showMemoryUsage(m_rank);
		#ifdef ASSERT
/*
		if(m_aliveWorkers.size()!=0){
			cout<<"Total="<<m_subgraph->size()<<" Completed="<<m_completedJobs<<" Alive="<<m_aliveWorkers.size()<<" Active="<<m_activeWorkers.size()<<endl;
		}
*/
		assert(m_aliveWorkers.size()==0);
		assert(m_activeWorkers.size()==0);
		#endif
	}
/*
	time_t t=time(NULL);
	if(t!=m_last){
		cout<<"Rank "<<m_rank<<" ActiveWorkers="<<m_activeWorkers.size()<<" AliveWorkers="<<m_aliveWorkers.size()<<" CompletedJobs="<<m_completedJobs<<"/"<<m_subgraph->size()<<endl;
		m_last=t;
	}
*/
}

void SeedingData::constructor(SeedExtender*seedExtender,int rank,int size,StaticVector*outbox,RingAllocator*outboxAllocator,int*seedCoverage,int*mode,
	Parameters*parameters,int*wordSize,MyForest*subgraph,bool*colorSpaceMode,StaticVector*inbox){
	m_seedExtender=seedExtender;
	m_size=size;
	m_rank=rank;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_seedCoverage=seedCoverage;
	m_mode=mode;
	m_parameters=parameters;
	m_wordSize=m_parameters->getWordSize();
	#ifdef ASSERT
	assert(m_wordSize>=15&&m_wordSize<=32);
	#endif
	m_subgraph=subgraph;
	m_colorSpaceMode=colorSpaceMode;
	m_initiatedIterator=false;

	m_virtualCommunicator.constructor(rank,size,outboxAllocator,inbox,outbox);

	m_virtualCommunicator.setReplyType(RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES,
						RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY);
	m_virtualCommunicator.setElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES,5);

	m_virtualCommunicator.setReplyType(RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES,
						RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY);
	m_virtualCommunicator.setElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES,5);

	m_virtualCommunicator.setReplyType(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY);
	m_virtualCommunicator.setElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,1);
}

int SeedingData::getRank(){
	return m_rank;
}

int SeedingData::getSize(){
	return m_size;
}

SeedingData::SeedingData(){
}
