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
				int rank,
				StaticVector*m_outbox,
				bool*m_mode_send_vertices,
				int m_wordSize,
				int size,
				RingAllocator*m_outboxAllocator,
				bool m_colorSpaceMode,int*m_mode
				){
	if(this->m_outbox==NULL){
		m_rank=rank;
		this->m_mode=m_mode;
		this->m_outbox=m_outbox;
		this->m_outboxAllocator=m_outboxAllocator;
	}
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif
	if(m_pendingMessages!=0){
		return;
	}

	if(mustTriggerReduction()&&m_mode_send_vertices_sequence_id_position==0){
		flushAll(m_outboxAllocator,m_outbox,rank);
		checkPendingMessagesForReduction(m_outbox,rank);

		return;
	}
	
	if(*m_mode_send_vertices_sequence_id%100000==0 &&m_mode_send_vertices_sequence_id_position==0){
		string reverse="";
		if(*m_reverseComplementVertex==true){
			reverse="(reverse complement) ";
		}
		printf("Rank %i is computing vertices & edges %s[%i/%i]\n",rank,reverse.c_str(),(int)*m_mode_send_vertices_sequence_id+1,(int)m_myReads->size());
		fflush(stdout);
	}

	if(*m_mode_send_vertices_sequence_id>(int)m_myReads->size()-1){
		// flush data
		flushAll(m_outboxAllocator,m_outbox,rank);
		if(m_pendingMessages==0){
			#ifdef ASSERT
			assert(m_bufferedData.isEmpty());
			assert(m_bufferedDataForIngoingEdges.isEmpty());
			assert(m_bufferedDataForOutgoingEdges.isEmpty());
			assert(m_buffersForOutgoingEdgesToDelete.isEmpty());
			assert(m_buffersForIngoingEdgesToDelete.isEmpty());
			#endif

			Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, RAY_MPI_TAG_VERTICES_DISTRIBUTED,rank);
			m_outbox->push_back(aMessage);
			*m_mode_send_vertices=false;
			(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
			printf("Rank %i is computing vertices & edges [%i/%i] (completed)\n",rank,(int)*m_mode_send_vertices_sequence_id,(int)m_myReads->size());
			fflush(stdout);
			m_finished=true;
		}
	}else{
		char*readSequence=(*m_myReads)[(*m_mode_send_vertices_sequence_id)]->getSeq();
		int len=strlen(readSequence);

		if(len<m_wordSize){
			m_hasPreviousVertex=false;
			(*m_mode_send_vertices_sequence_id)++;
			(m_mode_send_vertices_sequence_id_position)=0;
			return;
		}

		char memory[100];
		int lll=len-m_wordSize+1;
		
		#ifdef ASSERT
		assert(readSequence!=NULL);
		assert(m_wordSize<=32);
		#endif

		int p=(m_mode_send_vertices_sequence_id_position);
		memcpy(memory,readSequence+p,m_wordSize);
		memory[m_wordSize]='\0';
		if(isValidDNA(memory)){
			uint64_t a=wordId(memory);

			#ifdef ASSERT
			bool hit=false;
			if(idToWord(a,m_wordSize)=="GGTAGAGGAAAATGTTGCCAC"){
				hit=true;
			}
			#endif

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

			#ifdef ASSERT
			if(hit){
				if(!(idToWord(b,m_wordSize)=="GTGGCAACATTTTCCTCTACC")){
					cout<<idToWord(a,m_wordSize)<<" and "<<idToWord(b,m_wordSize)<<" color="<<m_colorSpaceMode<<endl;
				}
				assert(idToWord(b,m_wordSize)=="GTGGCAACATTTTCCTCTACC");
			}
			#endif


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

		(m_mode_send_vertices_sequence_id_position++);
		if((m_mode_send_vertices_sequence_id_position)==lll){
			m_hasPreviousVertex=false;
			(*m_mode_send_vertices_sequence_id)++;
			(m_mode_send_vertices_sequence_id_position)=0;
		}
	}
}

void VerticesExtractor::constructor(int size,Parameters*parameters){
m_distributionIsCompleted=false;
	m_outbox=NULL;
	m_mustFlushBuffers=false;
	m_mode_send_vertices_sequence_id_position=0;
	m_hasPreviousVertex=false;
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_bufferedDataForOutgoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_bufferedDataForIngoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	
	m_buffersForOutgoingEdgesToDelete.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_buffersForIngoingEdgesToDelete.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);

	m_pendingMessages=0;
	m_size=size;
	m_ranksDoneWithReduction=0;
	m_ranksReadyForReduction=0;

	m_reductionPeriod=parameters->getReducerValue();
	
	m_triggered=false;
	m_finished=false;
	m_mustTriggerReduction=false;
	m_thresholdForReduction=9999999999999;
}

void VerticesExtractor::enableReducer(){
	m_thresholdForReduction=m_reductionPeriod;
}

void VerticesExtractor::setReadiness(){
	#ifdef ASSERT
	assert(m_pendingMessages>0);
	#endif
	m_pendingMessages--;

	if(m_mustFlushBuffers){
		flushBuffers(m_rank,m_outbox,m_outboxAllocator);
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
	#ifdef ASSERT
	assert(m_buffersForIngoingEdgesToDelete.isEmpty());
	assert(m_buffersForOutgoingEdgesToDelete.isEmpty());
	assert(m_pendingMessages==0);
	#endif

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
	if(!m_bufferedData.isEmpty()){
		m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank);
		return;
	}
	if(!m_bufferedDataForOutgoingEdges.isEmpty()){
		m_pendingMessages+=m_bufferedDataForOutgoingEdges.flushAll(RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,rank);
		return;
	}
	if(!m_bufferedDataForIngoingEdges.isEmpty()){
		m_pendingMessages+=m_bufferedDataForIngoingEdges.flushAll(RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,rank);
		return;
	}
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
	assert(m_mode_send_vertices_sequence_id_position==0);
}

bool VerticesExtractor::mustTriggerReduction(){
	return m_mustTriggerReduction;
}

void VerticesExtractor::scheduleReduction(StaticVector*outbox,int rank){
	m_mustTriggerReduction=true;

	flushAll(m_outboxAllocator,m_outbox,rank);
	checkPendingMessagesForReduction(outbox,rank);
}

void VerticesExtractor::checkPendingMessagesForReduction(StaticVector*outbox,int rank){
	if(m_pendingMessages==0 && mustTriggerReduction()
	&&(m_mode_send_vertices_sequence_id_position)==0){// trigger at the beginning of a read, not in the middle.
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_ASK_BEGIN_REDUCTION_REPLY,rank);
		//cout<<"Source="<<rank<<" Destination="<<MASTER_RANK<<" RAY_MPI_TAG_ASK_BEGIN_REDUCTION_REPLY (meanwhile, freezing)"<<endl;
		outbox->push_back(aMessage);
		m_mustTriggerReduction=false;
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		#ifdef ASSERT
		assertBuffersAreEmpty();
		#endif
	}
}

/*
 * send the vertices to the owners for permanent deletions
 *
 */
bool VerticesExtractor::deleteVertices(vector<uint64_t>*verticesToRemove,MyForest*subgraph,Parameters*parameters,RingAllocator*m_outboxAllocator,
	StaticVector*m_outbox
){
	if(m_pendingMessages!=0){
		return false;
	}

	int size=parameters->getSize();
	int rank=parameters->getRank();
	bool color=parameters->getColorSpaceMode();
	int wordSize=parameters->getWordSize();

	if(!m_deletionsInitiated){
		m_deletionsInitiated=true;
		m_deletionIterator=0;
		#ifdef ASSERT
		assert(m_bufferedData.isEmpty());
		#endif
	}else if(m_deletionIterator<(uint64_t)verticesToRemove->size()){
		uint64_t vertex=verticesToRemove->at(m_deletionIterator);
		m_deletionIterator++;
		int rankToFlush=vertexRank(vertex,size);
		m_bufferedData.addAt(rankToFlush,vertex);

		if(m_bufferedData.flush(rankToFlush,1,RAY_MPI_TAG_DELETE_VERTEX,m_outboxAllocator,m_outbox,rank,false)){
			m_pendingMessages++;
		}

		uint64_t rcVertex=complementVertex(vertex,wordSize,color);
		rankToFlush=vertexRank(rcVertex,size);
		m_bufferedData.addAt(rankToFlush,rcVertex);

		if(m_bufferedData.flush(rankToFlush,1,RAY_MPI_TAG_DELETE_VERTEX,m_outboxAllocator,m_outbox,rank,false)){
			m_pendingMessages++;
		}
	}else{
		if(!m_bufferedData.isEmpty()){
			m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_DELETE_VERTEX,m_outboxAllocator,m_outbox,rank);
		}

		if(m_pendingMessages==0){
			#ifdef ASSERT
			assert(m_pendingMessages==0);
			assert(m_bufferedData.isEmpty());
			#endif
			return true;
		}
	}
	return false;
}

void VerticesExtractor::prepareDeletions(){
	m_deletionsInitiated=false;
}

void VerticesExtractor::flushBuffers(int rank,StaticVector*m_outbox,RingAllocator*m_outboxAllocator){
	m_mustFlushBuffers=true;
	if(!m_buffersForIngoingEdgesToDelete.isEmpty()){// not empty
		m_pendingMessages+=m_buffersForIngoingEdgesToDelete.flushAll(RAY_MPI_TAG_DELETE_INGOING_EDGE,m_outboxAllocator,m_outbox,rank);
		#ifdef ASSERT
		assert(m_pendingMessages>0); // flushed something
		#endif
		return;
	}

	if(!m_buffersForOutgoingEdgesToDelete.isEmpty()){// not empty
		m_pendingMessages+=m_buffersForOutgoingEdgesToDelete.flushAll(RAY_MPI_TAG_DELETE_OUTGOING_EDGE,m_outboxAllocator,m_outbox,rank);
		#ifdef ASSERT
		assert(m_pendingMessages>0);// flushed something
		#endif
		return;
	}

	if(m_pendingMessages==0){
		#ifdef ASSERT
		assert(m_pendingMessages==0);
		assert(m_buffersForOutgoingEdgesToDelete.isEmpty());
		assert(m_buffersForIngoingEdgesToDelete.isEmpty());
		#endif

		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY,rank);
		m_outbox->push_back(aMessage);
		m_mustFlushBuffers=false;
	}
}

void VerticesExtractor::incrementPendingMessages(){
	m_pendingMessages++;
}

void VerticesExtractor::showBuffers(){
	#ifdef ASSERT
	assert(m_buffersForOutgoingEdgesToDelete.isEmpty());
	assert(m_buffersForIngoingEdgesToDelete.isEmpty());
	assert(m_bufferedData.isEmpty());
	assert(m_bufferedDataForOutgoingEdges.isEmpty());
	assert(m_bufferedDataForIngoingEdges.isEmpty());
	#endif
}

bool VerticesExtractor::isDistributionCompleted(){
	return m_distributionIsCompleted;
}

void VerticesExtractor::setDistributionAsCompleted(){
	m_distributionIsCompleted=true;
}
