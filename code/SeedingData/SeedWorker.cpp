/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert

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

#include "SeedWorker.h"

#include <code/Mock/constants.h>
#include <code/Mock/common_functions.h>

#include <RayPlatform/communication/Message.h>
#include <RayPlatform/communication/mpi_tags.h>

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
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
				m_SEEDING_seed.setKmerLength(m_wordSize);

				// restore original starter.
				m_SEEDING_currentVertex=m_SEEDING_first;
				m_SEEDING_testInitiated=false;
				m_SEEDING_1_1_test_done=false;
			}
		}
	// check if currentVertex has 1 ingoing edge and 1 outgoing edge, if yes, add it
	}else if(m_elongationMode){

		// attempt to add m_SEEDING_currentVertex
		if(!m_SEEDING_1_1_test_done){
			do_1_1_test();
		}else{
			if(m_SEEDING_vertices.count(m_SEEDING_currentVertex)>0){// avoid infinite loops.
				m_SEEDING_1_1_test_result=false;
			}
			if(!m_SEEDING_1_1_test_result){

				if(m_debugSeeds){
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
					if((int)m_SEEDING_seed.size()<n){
						n=m_SEEDING_seed.size();
					}
					printf("Rank %i last %i coverage values in the seed:",m_rank,n);
					for(int i=n-1;i>=0;i--){
						int theCoverage=m_SEEDING_seed.getCoverageAt(m_SEEDING_seed.size()-1-i);

						printf(" %i",theCoverage);
					}
					printf("\n");
				}


				m_elongationMode=false;
				m_endChecksMode=true;
			}else{

				Kmer object;
				if(m_SEEDING_seed.size()>0)
					m_SEEDING_seed.at(m_SEEDING_seed.size()-1,&object);

				// we want some coherence...
				if(m_SEEDING_seed.size()>0
					&&!(object.isEqual(&m_SEEDING_currentParentVertex))){

					m_finished=true;
				}else{
					m_SEEDING_seed.push_back(&m_SEEDING_currentVertex);
					m_SEEDING_seed.addCoverageValue(m_cache[m_SEEDING_currentVertex]);

					m_SEEDING_vertices.insert(m_SEEDING_currentVertex);
					m_SEEDING_currentVertex=m_SEEDING_currentChildVertex;
					m_SEEDING_testInitiated=false;
					m_SEEDING_1_1_test_done=false;
				}
			}
		}
	}else if(m_endChecksMode){

		if(m_SEEDING_seed.size() > 3 * m_parameters->getWordSize()) {
			m_endChecksMode = false;
			return;
		}

		// I wonder why this code was disabled...
#if 1
		performChecksOnPathEnds();
#endif
	}else{
		m_finished=true;
	}
}

bool SeedWorker::getPathAfter(Kmer*kmer,int depth){

	Kmer object=*kmer;

	if(m_verticesAfter.size()>0){
		object=m_verticesAfter[m_verticesAfter.size()-1];
	}

// we reached the desired depth
	if((int)m_verticesAfter.size()>=depth){
		m_verticesAfter.clear();
		return true;
	}

// query data remotely
	if(fetchVertexData(&object)){

		m_vertexFetcherStarted=false;

		// only one parent
		if(m_vertexFetcherChildren.size()==1){
			m_verticesAfter.push_back(m_vertexFetcherChildren[0]);

		// more than 1 parent
		}else if(m_vertexFetcherChildren.size()>1){
			return true;

		// this is a dead end
		}else if(m_vertexFetcherChildren.size()==0){

			m_tailIsDeadEnd=true;

			return true;
		}
	}

	return false;
}

bool SeedWorker::getPathBefore(Kmer*kmer,int depth){

	Kmer object=*kmer;

	if(m_verticesBefore.size()>0){
		object=m_verticesBefore[m_verticesBefore.size()-1];
	}

// we reached the desired depth
	if((int)m_verticesBefore.size()>=depth){
		m_verticesBefore.clear();
		return true;
	}

// query data remotely
	if(fetchVertexData(&object)){

		m_vertexFetcherStarted=false;

		// only one parent
		if(m_vertexFetcherParents.size()==1){
			m_verticesBefore.push_back(m_vertexFetcherParents[0]);

		// more than 1 parent
		}else if(m_vertexFetcherParents.size()>1){
			return true;

		// this is a dead end
		}else if(m_vertexFetcherParents.size()==0){

			m_headIsDeadEnd=true;
			return true;
		}
	}

	return false;
}

void SeedWorker::performChecksOnPathEnds(){

	//m_endChecksMode = false;

/*
 * Check if it's a dead end. We don't want dead ends because they consume too
 * much time.
 *
 * First, check that the first k-mer is connected to something in the graph.
 *
 * Then, check that the last k-mer is connected to something in the graph too.
 */

	if(!m_endChecksModeStarted){
		m_endChecksModeStarted=true;
		m_checkedHead=false;
		m_vertexFetcherStarted=false;

/*
 * Check parents to see if it's a dead end.
 */
	}else if(!m_checkedHead){

		Kmer kmer;
		int positionInPath=0;
		m_SEEDING_seed.at(positionInPath,&kmer);

		if(getPathBefore(&kmer,10)){

			m_checkedHead=true;
			m_checkedTail=false;
			m_vertexFetcherStarted=false;
		}

/*
 * Check children of the last k-mer to see if there is a dead end.
 */
	}else if(!m_checkedTail){

		Kmer kmer;
		int positionInPath=m_SEEDING_seed.size()-1;
		m_SEEDING_seed.at(positionInPath,&kmer);

		if(getPathAfter(&kmer,10)){
			m_checkedTail=true;
			m_vertexFetcherStarted=false;
		}

	}else{
		m_endChecksMode=false;
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

#ifdef CONFIG_ASSERT
	assert(m_wordSize!=0);
#endif

	m_SEEDING_seed.setKmerLength(m_wordSize);

	m_debugSeeds=false;

	m_elongationMode=true;

	m_endChecksMode=false;

	m_headIsDeadEnd=false;
	m_tailIsDeadEnd=false;
}

void SeedWorker::enableDebugMode(){
	m_debugSeeds=true;
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
			m_parameters->vertexRank(&m_SEEDING_currentVertex),
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

			m_SEEDING_receivedIngoingEdges=m_SEEDING_currentVertex.getIngoingEdges(edges,m_wordSize);

			m_SEEDING_receivedOutgoingEdges=m_SEEDING_currentVertex.getOutgoingEdges(edges,m_wordSize);

			m_ingoingCoverages.clear();
			m_outgoingCoverages.clear();

			#ifdef CONFIG_ASSERT
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
					int dest=m_parameters->vertexRank(&vertex);

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
					int dest=m_parameters->vertexRank(&vertex);

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

GraphPath*SeedWorker::getSeed(){
	return &m_SEEDING_seed;
}

WorkerHandle SeedWorker::getWorkerIdentifier(){
	return m_workerIdentifier;
}

bool SeedWorker::isHeadADeadEnd(){
	return m_headIsDeadEnd;
}

bool SeedWorker::isTailADeadEnd(){
	return m_tailIsDeadEnd;
}

/**
 * Fetch parents and children and coverage depth.
 */
bool SeedWorker::fetchVertexData(Kmer*kmer){

	if(!m_vertexFetcherStarted){

		m_vertexFetcherRequestedData=false;

		m_vertexFetcherCoverage=0;
		m_vertexFetcherParents.clear();
		m_vertexFetcherChildren.clear();

		m_vertexFetcherStarted=true;

	}else if(!m_vertexFetcherRequestedData){

		MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
		int bufferPosition=0;
		kmer->pack(message,&bufferPosition);
		Message aMessage(message,m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT),
			m_parameters->vertexRank(kmer),
			RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,getRank());

		m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);

		m_vertexFetcherRequestedData=true;
		m_vertexFetcherReceivedData=false;

	}else if(!m_vertexFetcherReceivedData && m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){

		vector<MessageUnit> elements;
		m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&elements);

		int bufferPosition=0;

		uint8_t edges=elements[bufferPosition++];
		m_vertexFetcherCoverage=elements[bufferPosition++];

		m_vertexFetcherParents=kmer->getIngoingEdges(edges,m_wordSize);
		m_vertexFetcherChildren=kmer->getOutgoingEdges(edges,m_wordSize);

		return true;
	}

	return false;
}

bool SeedWorker::isBubbleWeakComponent(){
	return false;
}
