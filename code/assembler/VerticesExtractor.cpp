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
			#endif

			Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, RAY_MPI_TAG_VERTICES_DISTRIBUTED,rank);
			m_outbox->push_back(aMessage);
			m_finished=true;
			printf("Rank %i is computing vertices & edges [%i/%i] (completed)\n",rank,(int)*m_mode_send_vertices_sequence_id,(int)m_myReads->size());
			fflush(stdout);
			m_bufferedData.showStatistics(m_parameters->getRank());
			m_bufferedDataForIngoingEdges.showStatistics(m_parameters->getRank());
			m_bufferedDataForOutgoingEdges.showStatistics(m_parameters->getRank());
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
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),RAY_MALLOC_TYPE_VERTEX_EXTRACTOR_BUFFERS,m_parameters->showMemoryAllocations(),KMER_U64_ARRAY_SIZE);
	m_bufferedDataForOutgoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),RAY_MALLOC_TYPE_OUTGOING_EDGES_EXTRACTOR_BUFFERS,m_parameters->showMemoryAllocations(),2*KMER_U64_ARRAY_SIZE);
	m_bufferedDataForIngoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),RAY_MALLOC_TYPE_INGOING_EDGES_EXTRACTOR_BUFFERS,m_parameters->showMemoryAllocations(),2*KMER_U64_ARRAY_SIZE);

	m_pendingMessages=0;
	m_size=size;
}

void VerticesExtractor::setReadiness(){
	#ifdef ASSERT
	assert(m_pendingMessages>0);
	#endif
	m_pendingMessages--;
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

bool VerticesExtractor::finished(){
	return m_finished;
}

void VerticesExtractor::assertBuffersAreEmpty(){
	assert(m_bufferedData.isEmpty());
	assert(m_bufferedDataForOutgoingEdges.isEmpty());
	assert(m_bufferedDataForIngoingEdges.isEmpty());
	assert(m_mode_send_vertices_sequence_id_position==0);
}

void VerticesExtractor::incrementPendingMessages(){
	m_pendingMessages++;
}

void VerticesExtractor::showBuffers(){
	#ifdef ASSERT
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
