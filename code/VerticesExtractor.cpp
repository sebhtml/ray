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

#include<string.h>
#include<stdlib.h>
#include<VerticesExtractor.h>
#include<assert.h>
#include<Message.h>
#include<time.h>
#include<StaticVector.h>
#include<common_functions.h>

void VerticesExtractor::process(int*m_mode_send_vertices_sequence_id,
				ArrayOfReads*m_myReads,
				bool*m_reverseComplementVertex,
				int*m_mode_send_vertices_sequence_id_position,
				int rank,
				StaticVector*m_outbox,
				bool*m_mode_send_vertices,
				int m_wordSize,
				int size,
				RingAllocator*m_outboxAllocator,
				bool m_colorSpaceMode,int*m_mode
				){
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif
	if(m_pendingMessages!=0){
		return;
	}
	#ifdef SHOW_PROGRESS
	if(*m_mode_send_vertices_sequence_id%100000==0 and *m_mode_send_vertices_sequence_id_position==0){
		string reverse="";
		if(*m_reverseComplementVertex==true){
			reverse="(reverse complement) ";
		}
		printf("Rank %i is computing vertices & edges %s[%i/%i]\n",rank,reverse.c_str(),(int)*m_mode_send_vertices_sequence_id+1,(int)m_myReads->size());
		fflush(stdout);
	}
	#endif

	if(*m_mode_send_vertices_sequence_id>(int)m_myReads->size()-1){
		// flush data
		flushAll(m_outboxAllocator,m_outbox,rank);

		Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, RAY_MPI_TAG_VERTICES_DISTRIBUTED,rank);
		m_outbox->push_back(aMessage);
		*m_mode_send_vertices=false;
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		printf("Rank %i is computing vertices & edges [%i/%i] (completed)\n",rank,(int)*m_mode_send_vertices_sequence_id,(int)m_myReads->size());
		fflush(stdout);
		m_bufferedData.clear();
		m_finished=true;
	}else{
		char*readSequence=(*m_myReads)[(*m_mode_send_vertices_sequence_id)]->getSeq();
		int len=strlen(readSequence);
		char memory[100];
		int lll=len-m_wordSize;
		
		#ifdef ASSERT
		assert(readSequence!=NULL);
		assert(m_wordSize<=32);
		#endif

		m_hasPreviousVertex=false;

		for(int p=0;p<lll;p++){
			memcpy(memory,readSequence+p,m_wordSize);
			memory[m_wordSize]='\0';
			if(isValidDNA(memory)){
				uint64_t a=wordId(memory);
				int rankToFlush=0;

				rankToFlush=vertexRank(a,size);
				m_bufferedData.addAt(rankToFlush,a);

				if(m_bufferedData.flush(rankToFlush,1,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,false)){
					m_pendingMessages++;
				}

				if(m_hasPreviousVertex){
					// outgoing edge
					int outgoingRank=vertexRank(m_previousVertex,size);
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,m_previousVertex);
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,a);

					if(m_bufferedDataForOutgoingEdges.needsFlushing(outgoingRank,2)){
						if(m_bufferedData.flush(outgoingRank,1,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,true)){
							m_pendingMessages++;
						}
					}

					if(m_bufferedDataForOutgoingEdges.flush(outgoingRank,2,RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,rank,false)){
						m_pendingMessages++;
					}

					// ingoing edge
					int ingoingRank=vertexRank(a,size);
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,m_previousVertex);
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,a);

					if(m_bufferedDataForIngoingEdges.needsFlushing(ingoingRank,2)){
						if(m_bufferedData.flush(ingoingRank,1,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,true)){
							m_pendingMessages++;
						}
					}

					if(m_bufferedDataForIngoingEdges.flush(ingoingRank,2,RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,rank,false)){
						m_pendingMessages++;
					}
				}

				// reverse complement
				uint64_t b=complementVertex(a,m_wordSize,m_colorSpaceMode);

				rankToFlush=vertexRank(b,size);
				m_bufferedData.addAt(rankToFlush,b);

				if(m_bufferedData.flush(rankToFlush,1,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,false)){
					m_pendingMessages++;
				}

				if(m_hasPreviousVertex){
					// outgoing edge
					int outgoingRank=vertexRank(b,size);
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,b);
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,m_previousVertexRC);

					if(m_bufferedDataForOutgoingEdges.needsFlushing(outgoingRank,2)){
						if(m_bufferedData.flush(outgoingRank,1,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,true)){
							m_pendingMessages++;
						}
					}

					if(m_bufferedDataForOutgoingEdges.flush(outgoingRank,2,RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,rank,false)){
						m_pendingMessages++;
					}

					// ingoing edge
					int ingoingRank=vertexRank(m_previousVertexRC,size);
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,b);
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,m_previousVertexRC);

					if(m_bufferedDataForIngoingEdges.needsFlushing(ingoingRank,2)){
						if(m_bufferedData.flush(ingoingRank,1,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,true)){
							m_pendingMessages++;
						}
					}

					if(m_bufferedDataForIngoingEdges.flush(ingoingRank,2,RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,rank,false)){
						m_pendingMessages++;
					}
				}

				// there is a previous vertex.
				m_hasPreviousVertex=true;
				m_previousVertex=a;
				m_previousVertexRC=b;
			}else{
				m_hasPreviousVertex=false;
			}
		}
		(*m_mode_send_vertices_sequence_id)++;
	}
}

void VerticesExtractor::constructor(int size){
	m_hasPreviousVertex=false;
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_bufferedDataForOutgoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_bufferedDataForIngoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	
	m_pendingMessages=0;
	m_size=size;
	m_ranksDoneWithReduction=0;
	m_ranksReadyForReduction=0;
	m_reductionPeriod=200000;
	m_thresholdForReduction=m_reductionPeriod;
	m_triggered=false;
	m_finished=false;
	m_mustTriggerReduction=false;
}

void VerticesExtractor::setReadiness(StaticVector*outbox,int rank){
	#ifdef ASSERT
	assert(m_pendingMessages>0);
	#endif
	m_pendingMessages--;

	if(m_pendingMessages==0 && mustTriggerReduction()){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_ASK_BEGIN_REDUCTION_REPLY,rank);
		outbox->push_back(aMessage);
		m_mustTriggerReduction=false;
	}
}

bool VerticesExtractor::mustRunReducer(){
	return (int)m_ranksThatMustRunReducer.size()==m_size;
}

void VerticesExtractor::addRankForReduction(int a){
	m_ranksThatMustRunReducer.insert(a);
}

void VerticesExtractor::resetRanksForReduction(){
	m_ranksThatMustRunReducer.clear();
}

void VerticesExtractor::incrementRanksReadyForReduction(){
	m_ranksReadyForReduction++;
}

bool VerticesExtractor::readyForReduction(){
	return m_size==m_ranksReadyForReduction;
}

void VerticesExtractor::incrementRanksDoneWithReduction(){
	m_ranksDoneWithReduction++;
}

bool VerticesExtractor::reductionIsDone(){
	return m_size==m_ranksDoneWithReduction;
}

void VerticesExtractor::resetRanksReadyForReduction(){
	m_ranksReadyForReduction=0;
}

void VerticesExtractor::resetRanksDoneForReduction(){
	m_ranksDoneWithReduction=0;
}

void VerticesExtractor::updateThreshold(MyForest*a){
	m_thresholdForReduction=a->size()+m_reductionPeriod;
}

uint64_t VerticesExtractor::getThreshold(){
	return m_thresholdForReduction;
}

bool VerticesExtractor::isTriggered(){
	return m_triggered;
}

void VerticesExtractor::trigger(){
	m_triggered=true;
}

void VerticesExtractor::flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int rank){
	m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank);

	m_pendingMessages+=m_bufferedDataForOutgoingEdges.flushAll(RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,rank);

	m_pendingMessages+=m_bufferedDataForIngoingEdges.flushAll(RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,rank);
}

void VerticesExtractor::removeTrigger(){
	m_triggered=false;
}

bool VerticesExtractor::finished(){
	return m_finished;
}

void VerticesExtractor::assertBuffersAreEmpty(){
	assert(m_bufferedData.isEmpty());
	assert(m_bufferedDataForOutgoingEdges.isEmpty());
	assert(m_bufferedDataForIngoingEdges.isEmpty());
}

bool VerticesExtractor::mustTriggerReduction(){
	return m_mustTriggerReduction;
}

void VerticesExtractor::scheduleReduction(){
	m_mustTriggerReduction=true;
}

/*
 * send the vertices to the owners for permanent deletions
 *
 */
bool VerticesExtractor::deleteVertices(vector<uint64_t>*verticesToRemove,MyForest*subgraph,int size,int rank,RingAllocator*m_outboxAllocator,
	StaticVector*m_outbox
){
	if(m_pendingMessages!=0){
		return false;
	}

	if(!m_deletionsInitiated){
		m_deletionsInitiated=true;
		m_deletionIterator=0;
	}else if(m_deletionIterator<(uint64_t)verticesToRemove->size()){
		uint64_t vertex=verticesToRemove->at(m_deletionIterator);
		m_deletionIterator++;
		int rankToFlush=vertexRank(vertex,size);
		m_bufferedData.addAt(rankToFlush,vertex);

		if(m_bufferedData.flush(rankToFlush,1,RAY_MPI_TAG_DELETE_VERTEX,m_outboxAllocator,m_outbox,rank,false)){
			m_pendingMessages++;
		}
	}else{
		m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_DELETE_VERTEX,m_outboxAllocator,m_outbox,rank);
		return true;
	}
	return false;
}

void VerticesExtractor::prepareDeletions(){
	m_deletionsInitiated=false;
}
