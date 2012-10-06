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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <plugin_VerticesExtractor/VerticesExtractor.h>
#include <assert.h>
#include <communication/Message.h>
#include <time.h>
#include <structures/StaticVector.h>
#include <application_core/common_functions.h>

__CreatePlugin(VerticesExtractor);

 /**/
 /**/
__CreateSlaveModeAdapter(VerticesExtractor,RAY_SLAVE_MODE_ADD_EDGES); /**/
 /**/
 /**/


void VerticesExtractor::call_RAY_SLAVE_MODE_ADD_EDGES(){

	MACRO_COLLECT_PROFILING_INFORMATION();

	if(this->m_outbox==NULL){
		m_rank=m_parameters->getRank();
		this->m_mode=m_mode;
		this->m_outbox=m_outbox;
		this->m_outboxAllocator=m_outboxAllocator;
	}

	if(m_finished){
		return;
	}

	if(!m_checkedCheckpoint){
		m_checkedCheckpoint=true;
		if(m_parameters->hasCheckpoint("GenomeGraph")){
			cout<<"Rank "<<m_parameters->getRank()<<": checkpoint GenomeGraph exists, not extracting vertices."<<endl;
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_VERTICES_DISTRIBUTED,m_parameters->getRank());
			m_outbox->push_back(aMessage);
			m_finished=true;
			return;
		}
	}

	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif
	if(m_pendingMessages!=0){
		return;
	}

	if(m_mode_send_vertices_sequence_id%10000==0 &&m_mode_send_vertices_sequence_id_position==0
		&&m_mode_send_vertices_sequence_id<(int)m_myReads->size()){

		string reverse="";
		if(m_reverseComplementVertex==true){
			reverse="(reverse complement) ";
		}
		printf("Rank %i is adding edges %s[%i/%i]\n",m_parameters->getRank(),reverse.c_str(),(int)m_mode_send_vertices_sequence_id+1,(int)m_myReads->size());
		fflush(stdout);

		m_derivative.addX(m_mode_send_vertices_sequence_id);
		m_derivative.printStatus(SLAVE_MODES[RAY_SLAVE_MODE_ADD_EDGES],RAY_SLAVE_MODE_ADD_EDGES);
		m_derivative.printEstimatedTime(m_myReads->size());
	}

	if(m_mode_send_vertices_sequence_id==(int)m_myReads->size()){

		MACRO_COLLECT_PROFILING_INFORMATION();

		// flush data
		flushAll(m_outboxAllocator,m_outbox,m_parameters->getRank());

		if(m_pendingMessages==0){
			#ifdef ASSERT
			assert(m_bufferedDataForIngoingEdges.isEmpty());
			assert(m_bufferedDataForOutgoingEdges.isEmpty());
			#endif

			Message aMessage(NULL,0, MASTER_RANK, RAY_MPI_TAG_VERTICES_DISTRIBUTED,m_parameters->getRank());
			m_outbox->push_back(aMessage);
			m_finished=true;
			printf("Rank %i is adding edges [%i/%i] (completed)\n",m_parameters->getRank(),(int)m_mode_send_vertices_sequence_id,(int)m_myReads->size());
			fflush(stdout);
			m_bufferedDataForIngoingEdges.showStatistics(m_parameters->getRank());
			m_bufferedDataForOutgoingEdges.showStatistics(m_parameters->getRank());

			m_derivative.writeFile(&cout);
		}
	}else{

		MACRO_COLLECT_PROFILING_INFORMATION();

/*
 * Decode the DNA sequence 
 * and store it in a local buffer.
 */
		if(m_mode_send_vertices_sequence_id_position==0){
			(*m_myReads)[(m_mode_send_vertices_sequence_id)]->getSeq(m_readSequence,m_parameters->getColorSpaceMode(),false);
		
			//cout<<"DEBUG Read="<<*m_mode_send_vertices_sequence_id<<" color="<<m_parameters->getColorSpaceMode()<<" Seq= "<<m_readSequence<<endl;
		}

		int len=strlen(m_readSequence);

		if(len<m_parameters->getWordSize()){
			m_hasPreviousVertex=false;
			(m_mode_send_vertices_sequence_id)++;
			(m_mode_send_vertices_sequence_id_position)=0;
			return;
		}

		MACRO_COLLECT_PROFILING_INFORMATION();

		char memory[1000];

		int maximumPosition=len-m_parameters->getWordSize()+1;
		
		#ifdef ASSERT
		assert(m_readSequence!=NULL);
		#endif

		int p=(m_mode_send_vertices_sequence_id_position);
		memcpy(memory,m_readSequence+p,m_parameters->getWordSize());
		memory[m_parameters->getWordSize()]='\0';

		MACRO_COLLECT_PROFILING_INFORMATION();

		if(isValidDNA(memory)){

			MACRO_COLLECT_PROFILING_INFORMATION();

			Kmer currentForwardKmer=wordId(memory);

			/* TODO: possibly don't flush k-mer that are not lower. not sure it that would work though. -Seb */

/*
 *                   previousForwardKmer   ->   currentForwardKmer
 *                   previousReverseKmer   <-   currentReverseKmer
 */


/*
 * Push the kmer
 */


			MACRO_COLLECT_PROFILING_INFORMATION();

			if(m_hasPreviousVertex){

				MACRO_COLLECT_PROFILING_INFORMATION();

				// outgoing edge
				// PreviousVertex(*) -> CurrentVertex
				Rank outgoingRank=m_parameters->_vertexRank(&m_previousVertex);
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,m_previousVertex.getU64(i));
				}
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,currentForwardKmer.getU64(i));
				}


				if(m_bufferedDataForOutgoingEdges.flush(outgoingRank,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,m_parameters->getRank(),false)){
					m_pendingMessages++;
				}

				// ingoing edge
				// PreviousVertex -> CurrentVertex(*)
				Rank ingoingRank=m_parameters->_vertexRank(&currentForwardKmer);
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,m_previousVertex.getU64(i));
				}
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,currentForwardKmer.getU64(i));
				}


				if(m_bufferedDataForIngoingEdges.flush(ingoingRank,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,m_parameters->getRank(),false)){
					m_pendingMessages++;
				}

				MACRO_COLLECT_PROFILING_INFORMATION();
			}

			// reverse complement
			//
			Kmer currentReverseKmer=currentForwardKmer.
				complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());


			if(m_hasPreviousVertex){
				MACRO_COLLECT_PROFILING_INFORMATION();

				// outgoing edge
				// 
				Rank outgoingRank=m_parameters->_vertexRank(&currentReverseKmer);

				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,currentReverseKmer.getU64(i));
				}
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForOutgoingEdges.addAt(outgoingRank,m_previousVertexRC.getU64(i));
				}

				MACRO_COLLECT_PROFILING_INFORMATION();


				if(m_bufferedDataForOutgoingEdges.flush(outgoingRank,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,m_parameters->getRank(),false)){

					m_pendingMessages++;
				}

				MACRO_COLLECT_PROFILING_INFORMATION();

				// ingoing edge
				Rank ingoingRank=m_parameters->_vertexRank(&m_previousVertexRC);

				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,currentReverseKmer.getU64(i));
				}
				for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
					m_bufferedDataForIngoingEdges.addAt(ingoingRank,m_previousVertexRC.getU64(i));
				}

				MACRO_COLLECT_PROFILING_INFORMATION();


				if(m_bufferedDataForIngoingEdges.flush(ingoingRank,2*KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,m_parameters->getRank(),false)){
					m_pendingMessages++;
				}
				MACRO_COLLECT_PROFILING_INFORMATION();
			}

			// there is a previous vertex.
			m_hasPreviousVertex=true;
			m_previousVertex=currentForwardKmer;
			m_previousVertexRC=currentReverseKmer;
		}else{
			m_hasPreviousVertex=false;
		}

		MACRO_COLLECT_PROFILING_INFORMATION();

		(m_mode_send_vertices_sequence_id_position++);

		if((m_mode_send_vertices_sequence_id_position)==maximumPosition){
			m_hasPreviousVertex=false;
			(m_mode_send_vertices_sequence_id)++;
			(m_mode_send_vertices_sequence_id_position)=0;
		}
	}
	MACRO_COLLECT_PROFILING_INFORMATION();
}

void VerticesExtractor::constructor(int size,Parameters*parameters,GridTable*graph,
	StaticVector*outbox,RingAllocator*outboxAllocator,ArrayOfReads*reads){

	m_myReads=reads;
	m_checkedCheckpoint=false;
	m_subgraph=graph;
	m_parameters=parameters;
	m_finished=false;
	m_distributionIsCompleted=false;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_mode_send_vertices_sequence_id_position=0;
	m_hasPreviousVertex=false;

	m_bufferedDataForOutgoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),"RAY_MALLOC_TYPE_OUTGOING_EDGES_EXTRACTOR_BUFFERS",m_parameters->showMemoryAllocations(),2*KMER_U64_ARRAY_SIZE);
	m_bufferedDataForIngoingEdges.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),"RAY_MALLOC_TYPE_INGOING_EDGES_EXTRACTOR_BUFFERS",m_parameters->showMemoryAllocations(),2*KMER_U64_ARRAY_SIZE);

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

/**
 * wait for the reply for current messages
 */
	if(m_pendingMessages)
		return;

	if(!m_bufferedDataForOutgoingEdges.isEmpty()){
		m_pendingMessages+=m_bufferedDataForOutgoingEdges.flushAll(RAY_MPI_TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,m_parameters->getRank());
		return;
	}
	if(!m_bufferedDataForIngoingEdges.isEmpty()){
		m_pendingMessages+=m_bufferedDataForIngoingEdges.flushAll(RAY_MPI_TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,m_parameters->getRank());
		return;
	}
}

bool VerticesExtractor::finished(){
	return m_finished;
}

void VerticesExtractor::assertBuffersAreEmpty(){
	assert(m_bufferedDataForOutgoingEdges.isEmpty());
	assert(m_bufferedDataForIngoingEdges.isEmpty());
	assert(m_mode_send_vertices_sequence_id_position==0);
}

void VerticesExtractor::incrementPendingMessages(){
	m_pendingMessages++;
}

void VerticesExtractor::showBuffers(){
	#ifdef ASSERT
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

void VerticesExtractor::setProfiler(Profiler*profiler){
	m_profiler = profiler;
}

void VerticesExtractor::registerPlugin(ComputeCore*core){
	m_plugin=core->allocatePluginHandle();

	PluginHandle plugin=m_plugin;

	core->setPluginName(plugin,"VerticesExtractor");
	core->setPluginDescription(plugin,"Builds the distributed de Bruijn graph");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_SLAVE_MODE_ADD_EDGES=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_ADD_EDGES, __GetAdapter(VerticesExtractor,RAY_SLAVE_MODE_ADD_EDGES));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_ADD_EDGES,"RAY_SLAVE_MODE_ADD_EDGES");

	RAY_MPI_TAG_WRITE_KMERS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_WRITE_KMERS,"RAY_MPI_TAG_WRITE_KMERS");

	RAY_MPI_TAG_WRITE_KMERS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_WRITE_KMERS_REPLY,"RAY_MPI_TAG_WRITE_KMERS_REPLY");

	RAY_MPI_TAG_BUILD_GRAPH=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_BUILD_GRAPH,"RAY_MPI_TAG_BUILD_GRAPH");
	RAY_MPI_TAG_VERTEX_INFO_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTEX_INFO_REPLY,"RAY_MPI_TAG_VERTEX_INFO_REPLY");

}

void VerticesExtractor::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_ADD_EDGES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ADD_EDGES");
	RAY_SLAVE_MODE_WRITE_KMERS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_WRITE_KMERS");

	RAY_MPI_TAG_IN_EDGES_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_IN_EDGES_DATA");
	RAY_MPI_TAG_OUT_EDGES_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_OUT_EDGES_DATA");
	RAY_MPI_TAG_VERTICES_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTICES_DATA");
	RAY_MPI_TAG_VERTICES_DISTRIBUTED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTICES_DISTRIBUTED");

	RAY_MPI_TAG_WRITE_KMERS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_KMERS");
	RAY_MPI_TAG_WRITE_KMERS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_KMERS_REPLY");

	RAY_MPI_TAG_BUILD_GRAPH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_BUILD_GRAPH");
	RAY_MPI_TAG_VERTEX_INFO_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_INFO_REPLY");


	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_BUILD_GRAPH,          RAY_SLAVE_MODE_ADD_EDGES );
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_WRITE_KMERS,   RAY_SLAVE_MODE_WRITE_KMERS);

	__BindPlugin(VerticesExtractor);
}
