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

#include <plugin_EdgePurger/EdgePurgerWorker.h>

bool EdgePurgerWorker::isDone(){
	return m_isDone;
}

void EdgePurgerWorker::work(){
	if(!m_doneIngoingEdges){
		if(!m_ingoingInitialised){
			m_edges=m_vertex->getIngoingEdges(&m_currentKmer,m_parameters->getWordSize());
			m_iterator=0;
			m_ingoingInitialised=true;
			m_coverageRequested=false;
		}else if(m_iterator<(int)m_edges.size()){
			Kmer vertex=m_edges[m_iterator];
			if(!m_coverageRequested){
				Rank sendTo=m_parameters->_vertexRank(&vertex);
				MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
				int bufferPosition=0;
				vertex.pack(message,&bufferPosition);
				Message aMessage(message,bufferPosition,sendTo,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
				m_coverageRequested=true;
			}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
				vector<MessageUnit> response;
				m_virtualCommunicator->getMessageResponseElements(m_workerId,&response);

				CoverageDepth coverage=response[0];

/*
 * The vertex is not in the graph.
 */
				if(coverage==0){
					m_vertex->deleteIngoingEdge(&m_currentKmer,&vertex,m_parameters->getWordSize());
				}

				m_iterator++;
				m_coverageRequested=false;
			}
		}else{
			m_doneIngoingEdges=true;
			m_doneOutgoingEdges=false;
			m_outgoingInitialised=false;
		}
	}else if(!m_doneOutgoingEdges){
		if(!m_outgoingInitialised){
			m_outgoingInitialised=true;
			m_iterator=0;
			m_edges=m_vertex->getOutgoingEdges(&m_currentKmer,m_parameters->getWordSize());
			m_coverageRequested=false;
		}else if(m_iterator<(int)m_edges.size()){
			Kmer vertex=m_edges[m_iterator];
			if(!m_coverageRequested){
				Rank sendTo=m_parameters->_vertexRank(&vertex);
				MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
				int bufferPosition=0;
				vertex.pack(message,&bufferPosition);
				Message aMessage(message,bufferPosition,sendTo,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
				m_coverageRequested=true;
			}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
				vector<MessageUnit> response;
				m_virtualCommunicator->getMessageResponseElements(m_workerId,&response);

				CoverageDepth coverage=response[0];

/*
 * The vertex is not in the graph
 */
				if(coverage==0){
					m_vertex->deleteOutgoingEdge(&m_currentKmer,&vertex,m_parameters->getWordSize());
				}
				m_iterator++;
				m_coverageRequested=false;
			}
		}else{
			m_isDone=true;
			m_doneOutgoingEdges=true;
		}
	}
}

void EdgePurgerWorker::constructor(WorkerHandle workerId,Vertex*vertex,Kmer*currentKmer,GridTable*subgraph,VirtualCommunicator*virtualCommunicator,RingAllocator*outboxAllocator,Parameters*parameters,
		StaticVector*inbox,StaticVector*outbox,
	MessageTag tag){

	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=tag;

	m_workerId=workerId;
	m_vertex=vertex;
	m_currentKmer=*currentKmer;
	m_subgraph=subgraph;
	m_virtualCommunicator=virtualCommunicator;
	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;
	m_inbox=inbox;
	m_outbox=outbox;
	m_isDone=false;
	m_doneIngoingEdges=false;
	m_ingoingInitialised=false;
}

WorkerHandle EdgePurgerWorker::getWorkerIdentifier(){
	return m_workerId;
}
