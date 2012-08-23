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

#include <application_core/constants.h>
#include <plugin_SeedingData/SeedWorker.h>
#include <assert.h>
#include <communication/Message.h>
#include <stdlib.h>
#include <stdio.h>
#include <communication/mpi_tags.h>
#include <application_core/common_functions.h>
#include <stdint.h>
#include <iostream>
using namespace std;

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

				if(m_parameters->debugSeeds()){
					printf("Rank %i next vertex: Coverage= %i, ingoing coverages:",m_rank,m_cache[m_SEEDING_currentVertex]);
					for(int i=0;i<(int)m_ingoingCoverages.size();i++){
						printf(" %i",m_ingoingCoverages[i]);
					}
					printf(" outgoing coverages:");
					for(int i=0;i<(int)m_outgoingCoverages.size();i++){
						printf(" %i",m_outgoingCoverages[i]);
					}
					printf("\n");

					int n=100;
					if((int)m_coverages.size()<n){
						n=m_coverages.size();
					}
					printf("Rank %i last %i coverage values in the seed:",m_rank,n);
					for(int i=n-1;i>=0;i--){
						printf(" %i",m_coverages[m_coverages.size()-i-1]);
					}
					printf("\n");
				}
			}else{
				// we want some coherence...
				if(m_SEEDING_seed.size()>0
				&&!(m_SEEDING_seed[m_SEEDING_seed.size()-1].isEqual(&m_SEEDING_currentParentVertex))){
					m_finished=true;
				}else{
					m_SEEDING_seed.push_back(m_SEEDING_currentVertex);
					m_coverages.push_back(m_cache[m_SEEDING_currentVertex]);
					m_SEEDING_vertices.insert(m_SEEDING_currentVertex);
					m_SEEDING_currentVertex=m_SEEDING_currentChildVertex;
					m_SEEDING_testInitiated=false;
					m_SEEDING_1_1_test_done=false;
				}
			}
		}
	}
}

bool SeedWorker::isDone(){
	return m_finished;
}

void SeedWorker::constructor(Kmer*key,Parameters*parameters,RingAllocator*outboxAllocator,
		VirtualCommunicator*virtualCommunicator,WorkerHandle workerId,

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE
){
	this->RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT=RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;

	m_workerIdentifier=workerId;
	m_virtualCommunicator=virtualCommunicator;
	m_finished=false;
	m_outboxAllocator=outboxAllocator;
	m_SEEDING_currentVertex=*key;
	m_SEEDING_first=m_SEEDING_currentVertex;
	m_SEEDING_testInitiated=false;
	m_SEEDING_1_1_test_done=false;
	m_SEEDING_firstVertexTestDone=false;
	m_size=parameters->getSize();
	m_rank=parameters->getRank();
	m_SEEDING_seed.clear();
	m_wordSize=parameters->getWordSize();
	m_parameters=parameters;
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

			MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
			int bufferPosition=0;
			m_SEEDING_currentVertex.pack(message,&bufferPosition);
			Message aMessage(message,m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT),
			m_parameters->_vertexRank(&m_SEEDING_currentVertex),
				RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,getRank());
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
			vector<MessageUnit> elements;
			m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&elements);
			uint8_t edges=elements[0];
			m_mainVertexCoverage=elements[1];
			

			m_cache[m_SEEDING_currentVertex]=m_mainVertexCoverage;

			m_SEEDING_receivedIngoingEdges=m_SEEDING_currentVertex._getIngoingEdges(edges,m_wordSize);

			m_SEEDING_receivedOutgoingEdges=m_SEEDING_currentVertex._getOutgoingEdges(edges,m_wordSize);

			m_ingoingCoverages.clear();
			m_outgoingCoverages.clear();

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
				Kmer vertex=m_SEEDING_receivedIngoingEdges[m_SEEDING_ingoingEdgeIndex];
				if(m_cache.count(vertex)>0){
					m_SEEDING_receivedVertexCoverage=m_cache[vertex];
					m_SEEDING_ingoingEdgeIndex++;
					m_ingoingCoverages.push_back(m_SEEDING_receivedVertexCoverage);
				}else if(!m_SEEDING_vertexCoverageRequested){
					MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(KMER_U64_ARRAY_SIZE*sizeof(MessageUnit));
					int bufferPosition=0;
					vertex.pack(message,&bufferPosition);
					int dest=m_parameters->_vertexRank(&vertex);

					Message aMessage(message,bufferPosition,dest,
						RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,getRank());
					m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
					m_SEEDING_vertexCoverageRequested=true;
				}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
					vector<MessageUnit> response;
					m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
					m_SEEDING_receivedVertexCoverage=response[0];
					m_cache[vertex]=m_SEEDING_receivedVertexCoverage;
					m_SEEDING_ingoingEdgeIndex++;
					m_SEEDING_vertexCoverageRequested=false;
					m_ingoingCoverages.push_back(m_SEEDING_receivedVertexCoverage);
				}
			}else if(m_SEEDING_outgoingEdgeIndex<(int)m_SEEDING_receivedOutgoingEdges.size()){
				Kmer vertex=m_SEEDING_receivedOutgoingEdges[m_SEEDING_outgoingEdgeIndex];
				if(m_cache.count(vertex)>0){
					m_SEEDING_receivedVertexCoverage=m_cache[vertex];
					m_SEEDING_outgoingEdgeIndex++;
					m_outgoingCoverages.push_back(m_SEEDING_receivedVertexCoverage);
				}else if(!m_SEEDING_vertexCoverageRequested){
					MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(KMER_U64_ARRAY_SIZE*sizeof(MessageUnit));
					int bufferPosition=0;
					vertex.pack(message,&bufferPosition);
					int dest=m_parameters->_vertexRank(&vertex);

					Message aMessage(message,bufferPosition,
						dest,
						RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,getRank());
					m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
					m_SEEDING_vertexCoverageRequested=true;
				}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
					vector<MessageUnit> response;
					m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
					m_SEEDING_receivedVertexCoverage=response[0];
					m_cache[vertex]=m_SEEDING_receivedVertexCoverage;
					m_SEEDING_outgoingEdgeIndex++;
					m_SEEDING_vertexCoverageRequested=false;
					m_outgoingCoverages.push_back(m_SEEDING_receivedVertexCoverage);
				}
			}else{// done analyzing ingoing edges.
				m_SEEDING_ingoingEdgesDone=true;
			}
		}
	}else{
		m_SEEDING_1_1_test_done=true;
		int multiplicator=4;
		bool oneParent=false;

		// if OK, set m_SEEDING_currentChildVertex and m_SEEDING_currentParentVertex
		
		for(int i=0;i<(int)m_ingoingCoverages.size();i++){
			int coverage=m_ingoingCoverages[i];
			Kmer vertex=m_SEEDING_receivedIngoingEdges[i];
			oneParent=true;

			// we want seeds to be unique 
			if(coverage>= 2*m_mainVertexCoverage){
				oneParent=false;
				break;
			}

			for(int j=0;j<(int)m_ingoingCoverages.size();j++){
				if(i==j){
					continue;
				}
				int otherCoverage=m_ingoingCoverages[j];
				if((otherCoverage*multiplicator)>coverage){
					oneParent=false;
					break;
				}
			}
			if(oneParent){
				m_SEEDING_currentParentVertex=vertex;
				break;
			}
		}
		
		bool oneChild=false;

		for(int i=0;i<(int)m_outgoingCoverages.size();i++){
			int coverage=m_outgoingCoverages[i];
			Kmer vertex=m_SEEDING_receivedOutgoingEdges[i];
			oneChild=true;

			// we want seeds to be unique 
			if(coverage>= 2*m_mainVertexCoverage){
				oneParent=false;
				break;
			}

			for(int j=0;j<(int)m_outgoingCoverages.size();j++){
				if(i==j){
					continue;
				}
				int otherCoverage=m_outgoingCoverages[j];
				if((otherCoverage*multiplicator)>coverage){
					oneChild=false;
					break;
				}
			}
			if(oneChild){
				m_SEEDING_currentChildVertex=vertex;
				break;
			}
		}

		m_SEEDING_1_1_test_result=oneChild&&oneParent;

/** 
 * weed out vertices that don't have enough
 * k-mer coverage depth.
 * getMinimumCoverageToStore returns 2 as of 2012-08-23
 */
		int minimumCoverageToStore=
			m_parameters->getMinimumCoverageToStore();

		if(m_mainVertexCoverage<minimumCoverageToStore)
			m_SEEDING_1_1_test_result=false;

	}
}

int SeedWorker::getSize(){
	return m_size;
}

int SeedWorker::getRank(){
	return m_rank;
}

vector<Kmer>*SeedWorker::getSeed(){
	return &m_SEEDING_seed;
}

vector<int>*SeedWorker::getCoverageVector(){
	return &m_coverages;
}

WorkerHandle SeedWorker::getWorkerIdentifier(){
	return m_workerIdentifier;
}
