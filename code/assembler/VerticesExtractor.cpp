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
#include <string.h>
#include <stdlib.h>
#include <assembler/VerticesExtractor.h>
#include <assert.h>
#include <communication/Message.h>
#include <time.h>
#include <structures/StaticVector.h>
#include <core/common_functions.h>
#include <memory/malloc_types.h>

void VerticesExtractor::process(int*m_mode_send_vertices_sequence_id,
				ArrayOfReads*m_myReads,
				bool*m_reverseComplementVertex,
				int rank,
				StaticVector*m_outbox,
				bool*m_mode_send_vertices,
				int wordSize,
				int size,
				RingAllocator*m_outboxAllocator,
				int*m_mode
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
	
	if(m_finished){
		return;
	}

	if(*m_mode_send_vertices_sequence_id%100000==0 &&m_mode_send_vertices_sequence_id_position==0
	&&*m_mode_send_vertices_sequence_id<(int)m_myReads->size()){
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
			m_finished=true;
			printf("Rank %i is computing vertices & edges [%i/%i] (completed)\n",rank,(int)*m_mode_send_vertices_sequence_id,(int)m_myReads->size());
			fflush(stdout);
		}
	}else{
		if(m_mode_send_vertices_sequence_id_position==0){
			(*m_myReads)[(*m_mode_send_vertices_sequence_id)]->getSeq(m_readSequence,m_parameters->getColorSpaceMode(),false);
		
			//cout<<"DEBUG Read="<<*m_mode_send_vertices_sequence_id<<" color="<<m_parameters->getColorSpaceMode()<<" Seq= "<<m_readSequence<<endl;
		}
		int len=strlen(m_readSequence);

		if(len<wordSize){
			m_hasPreviousVertex=false;
			(*m_mode_send_vertices_sequence_id)++;
			(m_mode_send_vertices_sequence_id_position)=0;
			return;
		}

		char memory[1000];
		int lll=len-wordSize+1;
		
		#ifdef ASSERT
		assert(m_readSequence!=NULL);
		#endif

		int p=(m_mode_send_vertices_sequence_id_position);
		memcpy(memory,m_readSequence+p,wordSize);
		memory[wordSize]='\0';
		if(isValidDNA(memory)){
			Kmer a=wordId(memory);

			int rankToFlush=0;

			rankToFlush=m_parameters->_vertexRank(&a);
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_bufferedData.addAt(rankToFlush,a.getU64(i));
			}

			if(m_bufferedData.flush(rankToFlush,KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,false)){
				m_pendingMessages++;
			}

			if(m_hasPreviousVertex){
				// outgoing edge
				int outgoingRank=m_parameters->_vertexRank(&m_previousVertex);
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,m_previousVertex.getU64(i));
				}
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,a.getU64(i));
				}

				if(m_bufferedDataForOutgoingEdges.needsFlushing(outgoingRank,2*KMER_U64_ARRAY_SIZE)){
					if(m_bufferedData.flush(outgoingRank,KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,true)){
						m_pendingMessages++;
					}
				}

				if(m_bufferedDataForOutgoingEdges.flush(outgoingRank,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,rank,false)){
					m_pendingMessages++;
				}

				// ingoing edge
				int ingoingRank=m_parameters->_vertexRank(&a);
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,m_previousVertex.getU64(i));
				}
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,a.getU64(i));
				}

				if(m_bufferedDataForIngoingEdges.needsFlushing(ingoingRank,2*KMER_U64_ARRAY_SIZE)){
					if(m_bufferedData.flush(ingoingRank,KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,true)){
						m_pendingMessages++;
					}
				}

				if(m_bufferedDataForIngoingEdges.flush(ingoingRank,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,rank,false)){
					m_pendingMessages++;
				}
			}

			// reverse complement
			Kmer b=complementVertex(&a,wordSize,m_parameters->getColorSpaceMode());

			rankToFlush=m_parameters->_vertexRank(&b);
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_bufferedData.addAt(rankToFlush,b.getU64(i));
			}

			if(m_bufferedData.flush(rankToFlush,KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,false)){
				m_pendingMessages++;
			}

			if(m_hasPreviousVertex){
				// outgoing edge
				int outgoingRank=m_parameters->_vertexRank(&b);
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,b.getU64(i));
				}
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,m_previousVertexRC.getU64(i));
				}

				if(m_bufferedDataForOutgoingEdges.needsFlushing(outgoingRank,2*KMER_U64_ARRAY_SIZE)){
					if(m_bufferedData.flush(outgoingRank,1*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,true)){
						m_pendingMessages++;
					}
				}

				if(m_bufferedDataForOutgoingEdges.flush(outgoingRank,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,rank,false)){
					m_pendingMessages++;
				}

				// ingoing edge
				int ingoingRank=m_parameters->_vertexRank(&m_previousVertexRC);
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,b.getU64(i));
				}
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,m_previousVertexRC.getU64(i));
				}

				if(m_bufferedDataForIngoingEdges.needsFlushing(ingoingRank,2*KMER_U64_ARRAY_SIZE)){
					if(m_bufferedData.flush(ingoingRank,1*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,true)){
						m_pendingMessages++;
					}
				}

				if(m_bufferedDataForIngoingEdges.flush(ingoingRank,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,rank,false)){
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
	m_parameters=parameters;
	m_finished=false;
	m_distributionIsCompleted=false;
	m_outbox=NULL;
	m_mode_send_vertices_sequence_id_position=0;
	m_hasPreviousVertex=false;
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),RAY_MALLOC_TYPE_VERTEX_EXTRACTOR_BUFFERS,m_parameters->showMemoryAllocations());
	m_bufferedDataForOutgoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),RAY_MALLOC_TYPE_OUTGOING_EDGES_EXTRACTOR_BUFFERS,m_parameters->showMemoryAllocations());
	m_bufferedDataForIngoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),RAY_MALLOC_TYPE_INGOING_EDGES_EXTRACTOR_BUFFERS,m_parameters->showMemoryAllocations());
	
	m_buffersForOutgoingEdgesToDelete.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),RAY_MALLOC_TYPE_DEL_OUT,m_parameters->showMemoryAllocations());
	m_buffersForIngoingEdgesToDelete.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),RAY_MALLOC_TYPE_DEL_IN,m_parameters->showMemoryAllocations());

	m_pendingMessages=0;
	m_size=size;
	m_ranksDoneWithReduction=0;
	m_ranksReadyForReduction=0;

	m_reductionPeriod=parameters->getReducerValue();
	
	m_triggered=false;
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

void VerticesExtractor::updateThreshold(GridTable*a){
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
bool VerticesExtractor::deleteVertices(vector<Kmer>*verticesToRemove,GridTable*subgraph,Parameters*parameters,RingAllocator*m_outboxAllocator,
	StaticVector*m_outbox,map<Kmer,vector<Kmer> >*ingoingEdges,map<Kmer,vector<Kmer> >*outgoingEdges
){
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif

	if(m_pendingMessages!=0){
		return false;
	}

	#ifdef ASSERT
	assert(m_bufferedDataForIngoingEdges.isEmpty());
	assert(m_bufferedDataForOutgoingEdges.isEmpty());
	#endif

	int rank=parameters->getRank();

	bool color=parameters->getColorSpaceMode();
	int wordSize=parameters->getWordSize();

	if(!m_deletionsInitiated){
		m_deletionsInitiated=true;
		m_deletionIterator=0;

		#ifdef ASSERT
		assert(m_bufferedData.isEmpty());
		assert(m_buffersForIngoingEdgesToDelete.isEmpty());
		assert(m_buffersForOutgoingEdgesToDelete.isEmpty());
		#endif
	}else if(m_deletionIterator<(uint64_t)verticesToRemove->size()){
		Kmer vertex=verticesToRemove->at(m_deletionIterator);
		m_deletionIterator++;
		int rankToFlush=m_parameters->_vertexRank(&vertex);
		for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
			m_bufferedData.addAt(rankToFlush,vertex.getU64(i));
		}

		if(m_bufferedData.flush(rankToFlush,1*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_DELETE_VERTEX,m_outboxAllocator,m_outbox,rank,false)){
			m_pendingMessages++;
		}

		Kmer rcVertex=complementVertex(&vertex,wordSize,color);
		rankToFlush=m_parameters->_vertexRank(&rcVertex);
		for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
			m_bufferedData.addAt(rankToFlush,rcVertex.getU64(i));
		}

		if(m_bufferedData.flush(rankToFlush,1*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_DELETE_VERTEX,m_outboxAllocator,m_outbox,rank,false)){
			m_pendingMessages++;
		}

		// using ingoing edges, tell parents to delete the associated outgoing edge
		// a maximum of 4 messages will be released

		vector<Kmer>ingoingEdgesForDirect=(*ingoingEdges)[vertex];

		#ifdef ASSERT
		assert(ingoingEdgesForDirect.size()>=0);
		assert(ingoingEdgesForDirect.size()<=4);
		#endif

		for(int j=0;j<(int)ingoingEdgesForDirect.size();j++){
			Kmer prefix=ingoingEdgesForDirect[j];
			Kmer suffix=vertex;
			int rankToFlush=m_parameters->_vertexRank(&prefix);
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForOutgoingEdgesToDelete.addAt(rankToFlush,prefix.getU64(i));
			}
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForOutgoingEdgesToDelete.addAt(rankToFlush,suffix.getU64(i));
			}

			if(m_buffersForOutgoingEdgesToDelete.flush(rankToFlush,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_DELETE_OUTGOING_EDGE,m_outboxAllocator,m_outbox,rank,false)){
				incrementPendingMessages();
			}

			// flush RC too
			// XXX: I am not sure that this procedure works too for color space, must verify.
			prefix=rcVertex;
			suffix=complementVertex(&(ingoingEdgesForDirect[j]),wordSize,color);
			rankToFlush=m_parameters->_vertexRank(&suffix);

			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForIngoingEdgesToDelete.addAt(rankToFlush,prefix.getU64(i));
			}

			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForIngoingEdgesToDelete.addAt(rankToFlush,suffix.getU64(i));
			}

			if(m_buffersForIngoingEdgesToDelete.flush(rankToFlush,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_DELETE_INGOING_EDGE,m_outboxAllocator,m_outbox,rank,false)){
				incrementPendingMessages();
			}

		}

		vector<Kmer>outgoingEdgesForDirect=(*outgoingEdges)[vertex];

		#ifdef ASSERT
		assert(outgoingEdgesForDirect.size()>=0);
		assert(outgoingEdgesForDirect.size()<=4);
		#endif

		// using outgoing edges, tell children to delete the associated ingoing edge
		// a maximum of 4 messages will be released
		for(int j=0;j<(int)outgoingEdgesForDirect.size();j++){
			Kmer prefix=vertex;
			Kmer suffix=outgoingEdgesForDirect[j];
			int rankToFlush=m_parameters->_vertexRank(&suffix);
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForIngoingEdgesToDelete.addAt(rankToFlush,prefix.getU64(i));
			}
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForIngoingEdgesToDelete.addAt(rankToFlush,suffix.getU64(i));
			}

			if(m_buffersForIngoingEdgesToDelete.flush(rankToFlush,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_DELETE_INGOING_EDGE,m_outboxAllocator,m_outbox,rank,false)){
				incrementPendingMessages();
			}

			// flush RC too
			// XXX: I am not sure that this procedure works too for color space, must verify.
			prefix=complementVertex(&(outgoingEdgesForDirect[j]),wordSize,color);
			suffix=rcVertex;
			rankToFlush=m_parameters->_vertexRank(&prefix);

			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForOutgoingEdgesToDelete.addAt(rankToFlush,prefix.getU64(i));
			}
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForOutgoingEdgesToDelete.addAt(rankToFlush,suffix.getU64(i));
			}

			if(m_buffersForOutgoingEdgesToDelete.flush(rankToFlush,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_DELETE_OUTGOING_EDGE,m_outboxAllocator,m_outbox,rank,false)){
				incrementPendingMessages();
			}
		}

		// a a maximum of 17 messages will be released in total.
	}else{
		if(!m_bufferedData.isEmpty()){
			m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_DELETE_VERTEX,m_outboxAllocator,m_outbox,rank);
			return false;
		}

		if(!m_buffersForIngoingEdgesToDelete.isEmpty()){
			m_pendingMessages+=m_buffersForIngoingEdgesToDelete.flushAll(RAY_MPI_TAG_DELETE_INGOING_EDGE,m_outboxAllocator,m_outbox,rank);
			return false;
		}
	
		if(!m_buffersForOutgoingEdgesToDelete.isEmpty()){
			m_pendingMessages+=m_buffersForOutgoingEdgesToDelete.flushAll(RAY_MPI_TAG_DELETE_OUTGOING_EDGE,m_outboxAllocator,m_outbox,rank);
			return false;
		}

		if(m_pendingMessages==0){

			#ifdef ASSERT
			assert(m_pendingMessages==0);
			assert(m_bufferedData.isEmpty());
			assert(m_buffersForOutgoingEdgesToDelete.isEmpty());
			assert(m_buffersForIngoingEdgesToDelete.isEmpty());
			#endif

			return true;
		}
	}
	return false;
}

void VerticesExtractor::prepareDeletions(){
	m_deletionsInitiated=false;
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
