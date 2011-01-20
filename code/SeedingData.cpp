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
	if(!m_virtualCommunicator.isReady()){
		return;
	}

	if(!m_initiatedIterator){
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

		m_splayTreeIterator.constructor(m_subgraph);
		m_SEEDING_NodeInitiated=false;
		m_initiatedIterator=true;

		#ifdef ASSERT
		m_splayTreeIterator.hasNext();
		#endif
	}
	// assign a first vertex
	else if(!m_SEEDING_NodeInitiated){
		if(m_SEEDING_i==(int)m_subgraph->size()){
			(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
			printf("Rank %i is creating seeds [%i/%i] (completed)\n",getRank(),(int)m_SEEDING_i,(int)m_subgraph->size());
			fflush(stdout);
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_SEEDING_IS_OVER,getRank());
			m_outbox->push_back(aMessage);

			showMemoryUsage(m_rank);

		}else{
			m_SEEDING_NodeInitiated=true;

			if(m_SEEDING_i % 100000 ==0){
				printf("Rank %i is creating seeds [%i/%i]\n",getRank(),(int)m_SEEDING_i+1,(int)m_subgraph->size());
				fflush(stdout);
				showMemoryUsage(m_rank);
			}
			#ifdef ASSERT
			assert(m_splayTreeIterator.hasNext());
			#endif

			//cout<<"Calling next SeedingI="<<m_SEEDING_i<<endl;
			SplayNode<uint64_t,Vertex>*node=m_splayTreeIterator.next();
			m_worker.constructor(node->getKey(),m_parameters,m_outboxAllocator,&m_virtualCommunicator,m_SEEDING_i);

			m_SEEDING_i++;

			#ifdef ASSERT
			m_splayTreeIterator.hasNext();
			#endif
		}
	}else{
		m_worker.work();
		if(m_worker.isDone()){
			m_SEEDING_NodeInitiated=false;

			vector<uint64_t> seed=m_worker.getSeed();

			int nucleotides=seed.size()+(*m_wordSize)-1;
			// only consider the long ones.
			if(nucleotides>=m_parameters->getMinimumContigLength()){
	
				// if both seeds are on the same rank
				// dump the reverse and keep the forward
				printf("Rank %i added a seed with %i vertices\n",m_rank,(int)seed.size());
				fflush(stdout);
				showMemoryUsage(m_rank);

				m_SEEDING_seeds.push_back(seed);

				uint64_t firstVertex=seed[0];
				uint64_t lastVertex=seed[seed.size()-1];
				uint64_t lastVertexReverse=complementVertex(lastVertex,(*m_wordSize),(*m_colorSpaceMode));
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
	}
	m_virtualCommunicator.forceFlushIfNothingWasAppended();
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
	m_wordSize=wordSize;
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
