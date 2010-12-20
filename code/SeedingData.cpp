/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<assert.h>
#include<SeedingData.h>
#include<Message.h>
#include<mpi.h>

void SeedingData::computeSeeds(){
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
		m_currentTreeIndex=0;

		#ifdef ASSERT
		assert(!m_splayTreeIterator.hasNext());
		#endif

		m_splayTreeIterator.constructor(m_subgraph->getTree(m_currentTreeIndex));
		m_splayTreeIterator.setId(m_currentTreeIndex);
		m_splayTreeIterator.setRank(getRank());
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
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_SEEDING_IS_OVER,getRank());
			m_outbox->push_back(aMessage);
		}else if(!m_splayTreeIterator.hasNext()){
			m_currentTreeIndex++;
			#ifdef ASSERT
			assert(m_SEEDING_i<m_subgraph->size());
			assert(m_currentTreeIndex<m_subgraph->getNumberOfTrees());
			#endif

			#ifdef ASSERT
			assert(m_subgraph->getNumberOfTrees()>1);
			#endif

			m_splayTreeIterator.constructor(m_subgraph->getTree(m_currentTreeIndex));
			m_splayTreeIterator.setId(m_currentTreeIndex);
			m_splayTreeIterator.setRank(getRank());
		}else{
			if(m_SEEDING_i % 100000 ==0){
				printf("Rank %i is creating seeds [%i/%i]\n",getRank(),(int)m_SEEDING_i+1,(int)m_subgraph->size());
				fflush(stdout);
			}
			#ifdef ASSERT
			assert(m_splayTreeIterator.hasNext());
			#endif

			//cout<<"Calling next SeedingI="<<m_SEEDING_i<<endl;
			SplayNode<uint64_t,Vertex>*node=m_splayTreeIterator.next();
			m_SEEDING_currentVertex=node->getKey();

			m_SEEDING_first=m_SEEDING_currentVertex;
			m_SEEDING_testInitiated=false;
			m_SEEDING_1_1_test_done=false;
			m_SEEDING_i++;
			m_SEEDING_NodeInitiated=true;
			m_SEEDING_firstVertexTestDone=false;

			#ifdef ASSERT
			m_splayTreeIterator.hasNext();
			#endif
		}
	// check that this node has 1 ingoing edge and 1 outgoing edge.
	}else if(!m_SEEDING_firstVertexTestDone){
		if(!m_SEEDING_1_1_test_done){
			do_1_1_test();
		}else{
			if(!m_SEEDING_1_1_test_result){
				m_SEEDING_NodeInitiated=false;// abort
			}else{
				m_SEEDING_firstVertexParentTestDone=false;
				m_SEEDING_firstVertexTestDone=true;
				m_SEEDING_currentVertex=m_SEEDING_currentParentVertex;
				m_SEEDING_testInitiated=false;
				m_SEEDING_1_1_test_done=false;
			}
		}

	// check that the parent does not have 1 ingoing edge and 1 outgoing edge
	}else if(!m_SEEDING_firstVertexParentTestDone){
		if(!m_SEEDING_1_1_test_done){
			do_1_1_test();
		}else{
			if(m_SEEDING_1_1_test_result){
				m_SEEDING_NodeInitiated=false;//abort
			}else{
				m_SEEDING_firstVertexParentTestDone=true;
				m_SEEDING_vertices.clear();
				m_SEEDING_seed.clear();
				// restore original starter.
				m_SEEDING_currentVertex=m_SEEDING_first;
				m_SEEDING_testInitiated=false;
				m_SEEDING_1_1_test_done=false;
			}
		}

	// check if currentVertex has 1 ingoing edge and 1 outgoing edge, if yes, add it
	}else{
		// attempt to add m_SEEDING_currentVertex
		if(!m_SEEDING_1_1_test_done){
			do_1_1_test();
		}else{
			if(m_SEEDING_vertices.count(m_SEEDING_currentVertex)>0){
				m_SEEDING_1_1_test_result=false;
			}
			if(!m_SEEDING_1_1_test_result){
				m_SEEDING_NodeInitiated=false;
				int nucleotides=m_SEEDING_seed.size()+(*m_wordSize)-1;
				// only consider the long ones.
				if(nucleotides>=m_parameters->getMinimumContigLength()){
		
					// if both seeds are on the same rank
					// dump the reverse and keep the forward

					m_SEEDING_seeds.push_back(m_SEEDING_seed);
					u64 firstVertex=m_SEEDING_seed[0];
					u64 lastVertex=m_SEEDING_seed[m_SEEDING_seed.size()-1];
					u64 lastVertexReverse=complementVertex(lastVertex,(*m_wordSize),(*m_colorSpaceMode));
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
			}else{
				m_SEEDING_seed.push_back(m_SEEDING_currentVertex);
				m_SEEDING_vertices.insert(m_SEEDING_currentVertex);
				m_SEEDING_currentVertex=m_SEEDING_currentChildVertex;
				m_SEEDING_testInitiated=false;
				m_SEEDING_1_1_test_done=false;
			}
		}
	}
}

/*
 * check if (m_SEEDING_currentRank,m_SEEDING_currentPointer) has
 * 1 ingoing edge and 1 outgoing edge
 *
 * before entering the first call, m_SEEDING_testInitiated and m_SEEDING_1_1_test_done must be false
 *
 * outputs:
 *
 *  m_SEEDING_1_1_test_done
 *  m_SEEDING_currentChildVertex
 *  m_SEEDING_currentChildRank
 *  m_SEEDING_currentChildPointer
 *  m_SEEDING_currentParentRank
 *  m_SEEDING_currentParentPointer
 *
 *
 *  internals:
 *
 *  m_SEEDING_InedgesRequested
 *  m_SEEDING_InedgesReceived
 *  m_SEEDING_Inedge
 *  m_SEEDING_edgesRequested
 *  m_SEEDING_edgesReceived
 */
void SeedingData::do_1_1_test(){
	if(m_SEEDING_1_1_test_done){
		return;
	}else if(!m_SEEDING_testInitiated){
		m_SEEDING_testInitiated=true;
		m_SEEDING_ingoingEdgesDone=false;
		m_SEEDING_InedgesRequested=false;
	}else if(!m_SEEDING_ingoingEdgesDone){
		if(!m_SEEDING_InedgesRequested){
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
			message[0]=(uint64_t)m_SEEDING_currentVertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex,getSize()),TAG_REQUEST_VERTEX_INGOING_EDGES,getRank());
			m_outbox->push_back(aMessage);
			m_SEEDING_numberOfIngoingEdges=0;
			m_SEEDING_numberOfIngoingEdgesWithSeedCoverage=0;
			m_SEEDING_vertexCoverageRequested=false;
			m_SEEDING_InedgesReceived=false;
			m_SEEDING_InedgesRequested=true;
			m_SEEDING_ingoingEdgeIndex=0;
		}else if(m_SEEDING_InedgesReceived){
			if(m_SEEDING_ingoingEdgeIndex<(int)m_SEEDING_receivedIngoingEdges.size()){
				if(!m_SEEDING_vertexCoverageRequested){
					uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
					message[0]=(uint64_t)m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],getSize()),TAG_REQUEST_VERTEX_COVERAGE,getRank());
					m_outbox->push_back(aMessage);
					m_SEEDING_vertexCoverageRequested=true;
					m_SEEDING_vertexCoverageReceived=false;
					m_SEEDING_receivedVertexCoverage=-1;
				}else if(m_SEEDING_vertexCoverageReceived){
					if(m_SEEDING_receivedIngoingEdges.size()==1){//there is only one anyway
						m_SEEDING_currentParentVertex=m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
					}
					if(m_SEEDING_receivedVertexCoverage>=(*m_seedCoverage)){
						m_SEEDING_currentParentVertex=m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
						m_SEEDING_numberOfIngoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_ingoingEdgeIndex++;
					m_SEEDING_numberOfIngoingEdges++;
					m_SEEDING_vertexCoverageRequested=false;
				}
			}else{// done analyzing ingoing edges.
				m_SEEDING_ingoingEdgesDone=true;
				m_SEEDING_outgoingEdgesDone=false;
				m_SEEDING_edgesRequested=false;
			}
		}
	}else if(!m_SEEDING_outgoingEdgesDone){
		if(!m_SEEDING_edgesRequested){
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
			message[0]=(uint64_t)m_SEEDING_currentVertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex,getSize()),TAG_REQUEST_VERTEX_OUTGOING_EDGES,getRank());
			m_outbox->push_back(aMessage);
			m_SEEDING_edgesRequested=true;
			m_SEEDING_numberOfOutgoingEdges=0;
			m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage=0;
			m_SEEDING_vertexCoverageRequested=false;
			m_SEEDING_edgesReceived=false;
			m_SEEDING_outgoingEdgeIndex=0;
		}else if(m_SEEDING_edgesReceived){
			if(m_SEEDING_outgoingEdgeIndex<(int)m_SEEDING_receivedOutgoingEdges.size()){
				// TODO: don't check the coverage if there is only one
				if(!m_SEEDING_vertexCoverageRequested){
					uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
					message[0]=(uint64_t)m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],getSize()),TAG_REQUEST_VERTEX_COVERAGE,getRank());
					m_outbox->push_back(aMessage);
					m_SEEDING_vertexCoverageRequested=true;
					m_SEEDING_vertexCoverageReceived=false;
					m_SEEDING_receivedVertexCoverage=-1;
				}else if(m_SEEDING_vertexCoverageReceived){
					if(m_SEEDING_receivedOutgoingEdges.size()==1){//there is only one anyway
						m_SEEDING_currentChildVertex=m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
					}
					if(m_SEEDING_receivedVertexCoverage>=(*m_seedCoverage)){
						m_SEEDING_currentChildVertex=m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
						m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_outgoingEdgeIndex++;
					m_SEEDING_numberOfOutgoingEdges++;
					m_SEEDING_vertexCoverageRequested=false;
				}
			}else{// done analyzing ingoing edges.
				m_SEEDING_outgoingEdgesDone=true;
			}
		}


	}else{
		m_SEEDING_1_1_test_done=true;
		m_SEEDING_1_1_test_result=(m_SEEDING_numberOfIngoingEdgesWithSeedCoverage==1)and
			(m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage==1);
	}
}

void SeedingData::constructor(SeedExtender*seedExtender,int rank,int size,StaticVector*outbox,RingAllocator*outboxAllocator,int*seedCoverage,int*mode,
	Parameters*parameters,int*wordSize,MyForest*subgraph,bool*colorSpaceMode){
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
}

int SeedingData::getRank(){
	return m_rank;
}

int SeedingData::getSize(){
	return m_size;
}

SeedingData::SeedingData(){
}
