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

#include <Message.h>
#include <mpi_tags.h>
#include <common_functions.h>
#include <stdint.h>
#include <SeedWorker.h>

void SeedWorker::work(){
	if(m_finished){
		return;
	}
	// check that this node has 1 ingoing edge and 1 outgoing edge.
	if(!m_SEEDING_firstVertexTestDone){
		if(!m_SEEDING_1_1_test_done){
			do_1_1_test();
		}else{
			if(!m_SEEDING_1_1_test_result){
				m_finished=true;
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
				m_finished=true;
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
			if(m_SEEDING_vertices.count(m_SEEDING_currentVertex)>0){// avoid infinite loops.
				m_SEEDING_1_1_test_result=false;
			}
			if(!m_SEEDING_1_1_test_result){
				m_finished=true;
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

bool SeedWorker::isDone(){
	return m_finished;
}

void SeedWorker::constructor(uint64_t key,Parameters*parameters,RingAllocator*outboxAllocator,
		VirtualCommunicator*virtualCommunicator,uint64_t workerId){
	m_workerIdentifier=workerId;
	m_virtualCommunicator=virtualCommunicator;
	m_finished=false;
	m_outboxAllocator=outboxAllocator;
	m_SEEDING_currentVertex=key;
	m_SEEDING_first=m_SEEDING_currentVertex;
	m_SEEDING_testInitiated=false;
	m_SEEDING_1_1_test_done=false;
	m_SEEDING_firstVertexTestDone=false;
	m_size=parameters->getSize();
	m_rank=parameters->getRank();
	m_seedCoverage=parameters->getSeedCoverage();
	m_SEEDING_seed.clear();
	m_wordSize=parameters->getWordSize();
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
void SeedWorker::do_1_1_test(){
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
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(m_SEEDING_currentVertex,getSize()),RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,getRank());
			m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
			m_SEEDING_numberOfIngoingEdgesWithSeedCoverage=0;
			m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage=0;
			m_SEEDING_vertexCoverageRequested=false;
			m_SEEDING_InedgesReceived=false;
			m_SEEDING_InedgesRequested=true;
			m_ingoingEdgesReceived=false;
			m_SEEDING_ingoingEdgeIndex=0;
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)
			&&!m_ingoingEdgesReceived){
			m_ingoingEdgesReceived=true;
			vector<uint64_t> elements=m_virtualCommunicator->getResponseElements(m_workerIdentifier);
			uint8_t edges=elements[0];
			m_SEEDING_receivedIngoingEdges=_getIngoingEdges(m_SEEDING_currentVertex,edges,m_wordSize);
			m_SEEDING_receivedOutgoingEdges=_getOutgoingEdges(m_SEEDING_currentVertex,edges,m_wordSize);
			#ifdef ASSERT
			if(m_SEEDING_receivedIngoingEdges.size()>4){
				cout<<"size="<<m_SEEDING_receivedIngoingEdges.size()<<endl;
			}
			assert(m_SEEDING_receivedIngoingEdges.size()<=4);
			#endif
			m_SEEDING_outgoingEdgeIndex=0;
			if(m_SEEDING_receivedIngoingEdges.size()==0||m_SEEDING_receivedOutgoingEdges.size()==0){
				m_SEEDING_1_1_test_done=true;
				m_SEEDING_1_1_test_result=false;
			}
		}else if(m_ingoingEdgesReceived){
			if(m_SEEDING_ingoingEdgeIndex<(int)m_SEEDING_receivedIngoingEdges.size()){
				uint64_t vertex=(uint64_t)m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
				if(m_cache.count(vertex)>0){
					m_SEEDING_receivedVertexCoverage=m_cache[vertex];
					if(m_SEEDING_receivedVertexCoverage>=m_seedCoverage){
						m_SEEDING_currentParentVertex=m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
						m_SEEDING_numberOfIngoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_ingoingEdgeIndex++;
				}else if(!m_SEEDING_vertexCoverageRequested){
					uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
					message[0]=vertex;
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],getSize()),RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,getRank());
					m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
					m_SEEDING_vertexCoverageRequested=true;
				}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
					m_SEEDING_receivedVertexCoverage=m_virtualCommunicator->getResponseElements(m_workerIdentifier)[0];
					m_cache[vertex]=m_SEEDING_receivedVertexCoverage;
					if(m_SEEDING_receivedVertexCoverage>=(m_seedCoverage)){
						m_SEEDING_currentParentVertex=m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
						m_SEEDING_numberOfIngoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_ingoingEdgeIndex++;
					m_SEEDING_vertexCoverageRequested=false;
					if(m_SEEDING_ingoingEdgeIndex==(int)m_SEEDING_receivedIngoingEdges.size()&&m_SEEDING_numberOfIngoingEdgesWithSeedCoverage!=1){
						m_SEEDING_1_1_test_done=true;
						m_SEEDING_1_1_test_result=false;
					}
				}
			}else if(m_SEEDING_outgoingEdgeIndex<(int)m_SEEDING_receivedOutgoingEdges.size()){
				uint64_t vertex=(uint64_t)m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
				if(m_cache.count(vertex)>0){
					m_SEEDING_receivedVertexCoverage=m_cache[vertex];
					if(m_SEEDING_receivedVertexCoverage>=(m_seedCoverage)){
						m_SEEDING_currentChildVertex=m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
						m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_outgoingEdgeIndex++;
				}else if(!m_SEEDING_vertexCoverageRequested){
					uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
					message[0]=vertex;
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],getSize()),RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,getRank());
					m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
					m_SEEDING_vertexCoverageRequested=true;
				}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
					m_SEEDING_receivedVertexCoverage=m_virtualCommunicator->getResponseElements(m_workerIdentifier)[0];
					m_cache[vertex]=m_SEEDING_receivedVertexCoverage;
					if(m_SEEDING_receivedVertexCoverage>=(m_seedCoverage)){
						m_SEEDING_currentChildVertex=m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
						m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_outgoingEdgeIndex++;
					m_SEEDING_vertexCoverageRequested=false;
				}
			}else{// done analyzing ingoing edges.
				m_SEEDING_ingoingEdgesDone=true;
			}
		}
	}else{
		m_SEEDING_1_1_test_done=true;
		m_SEEDING_1_1_test_result=(m_SEEDING_numberOfIngoingEdgesWithSeedCoverage==1)&&
			(m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage==1);
	}
}

int SeedWorker::getSize(){
	return m_size;
}

int SeedWorker::getRank(){
	return m_rank;
}

vector<uint64_t> SeedWorker::getSeed(){
	return m_SEEDING_seed;
}

uint64_t SeedWorker::getWorkerId(){
	return m_workerIdentifier;
}
