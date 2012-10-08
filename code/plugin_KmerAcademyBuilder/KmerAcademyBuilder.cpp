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
#include <stdlib.h>
#include <stdio.h>
#include <plugin_KmerAcademyBuilder/KmerAcademyBuilder.h>
#include <assert.h>
#include <communication/Message.h>
#include <time.h>
#include <structures/StaticVector.h>
#include <application_core/common_functions.h>
#include <fstream>


__CreatePlugin(KmerAcademyBuilder);

 /**/
 /**/
__CreateSlaveModeAdapter(KmerAcademyBuilder,RAY_SLAVE_MODE_ADD_VERTICES); /**/
 /**/
 /**/

using namespace std;

void KmerAcademyBuilder::call_RAY_SLAVE_MODE_ADD_VERTICES(){
	
	if(!m_initialised){
		m_initialised=true;
		(m_mode_send_vertices_sequence_id)=0;
	}

	MACRO_COLLECT_PROFILING_INFORMATION();

	if(m_finished)
		return;

	if(!m_checkedCheckpoint){
		m_checkedCheckpoint=true;
		if(m_parameters->hasCheckpoint("GenomeGraph")){
			cout<<"Rank "<<m_parameters->getRank()<<": checkpoint GenomeGraph exists, not counting k-mers."<<endl;
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED,m_parameters->getRank());
			m_outbox->push_back(aMessage);
			m_finished=true;
			return;
		}
	}

	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif
	if(m_inbox->size()>0&&m_inbox->at(0)->getTag()==RAY_MPI_TAG_VERTICES_DATA_REPLY){
		m_pendingMessages--;
	}

	MACRO_COLLECT_PROFILING_INFORMATION();

/*
 * Busy wait if there are pending messages.
 */
	if(m_pendingMessages>0){
		return;
	}

	if(m_mode_send_vertices_sequence_id%10000==0 &&m_mode_send_vertices_sequence_id_position==0
	&&m_mode_send_vertices_sequence_id<(int)m_myReads->size()){
		string reverse="";
		if(m_reverseComplementVertex==true){
			reverse="(reverse complement) ";
		}
		printf("Rank %i is counting k-mers in sequence reads %s[%i/%i]\n",m_parameters->getRank(),
			reverse.c_str(),(int)m_mode_send_vertices_sequence_id+1,(int)m_myReads->size());
		fflush(stdout);

		m_derivative.addX(m_mode_send_vertices_sequence_id);
		m_derivative.printStatus(SLAVE_MODES[RAY_SLAVE_MODE_ADD_VERTICES],RAY_SLAVE_MODE_ADD_VERTICES);
		m_derivative.printEstimatedTime(m_myReads->size());
	}

	if(m_mode_send_vertices_sequence_id>(int)m_myReads->size()-1){
		// flush data
		flushAll(m_outboxAllocator,m_outbox,m_parameters->getRank());
		if(m_pendingMessages==0){
			#ifdef ASSERT
			assert(m_bufferedData.isEmpty());
			#endif

			Message aMessage(NULL,0, MASTER_RANK,RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED,m_parameters->getRank());
			m_outbox->push_back(aMessage);
			m_finished=true;
			printf("Rank %i is counting k-mers in sequence reads [%i/%i] (completed)\n",
				m_parameters->getRank(),(int)m_mode_send_vertices_sequence_id,(int)m_myReads->size());
			fflush(stdout);
			m_bufferedData.showStatistics(m_parameters->getRank());

			
			m_derivative.writeFile(&cout);

		}
		MACRO_COLLECT_PROFILING_INFORMATION();
	}else{
		if(m_mode_send_vertices_sequence_id_position==0){
			(*m_myReads)[(m_mode_send_vertices_sequence_id)]->getSeq(m_readSequence,m_parameters->getColorSpaceMode(),false);

//#define DEBUG_GCC_4_7_2

			#ifdef DEBUG_GCC_4_7_2
			cout<<"[DEBUG_GCC_4_7_2] read is "<<m_readSequence<<endl;
			#endif
		}

		int len=strlen(m_readSequence);

		if(len<m_parameters->getWordSize()){
			(m_mode_send_vertices_sequence_id)++;
			(m_mode_send_vertices_sequence_id_position)=0;
			return;
		}

		char memory[MAXKMERLENGTH+1];
		int maximumPosition=len-m_parameters->getWordSize()+1;
		
		#ifdef ASSERT
		assert(m_readSequence!=NULL);
		#endif

		int position=(m_mode_send_vertices_sequence_id_position);

		memcpy(memory,m_readSequence+position,m_parameters->getWordSize());
		memory[m_parameters->getWordSize()]='\0';

		MACRO_COLLECT_PROFILING_INFORMATION();

		if(isValidDNA(memory)){
			Kmer kmerToSend=wordId(memory);

/*
 * Sets this if you only want to send the lower 
 * k-mer.
 *
 * TODO: the code is suppose to work without __SEND_LOWER, but it does not.
 */
			#define __SEND_LOWER

			#ifdef __SEND_LOWER
			Kmer reverseKmer=kmerToSend.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

			if(reverseKmer<kmerToSend)
				kmerToSend=reverseKmer;
			#endif
	
			#undef __SEND_LOWER

			MACRO_COLLECT_PROFILING_INFORMATION();


/*
 * We only send one of the two kmer at this point.
 * The two kmers are the forwardKmer and the reverseKmer.
 * The reason is that the ForwardKmer will add +1 to
 * its coverage and the ReverseKmer will also add +1 to
 * its coverage. But in the implementation,
 * ForwardKmer and ReverseKmer are stored together.
 * To avoid doubling the coverage of any k-mer, we sent
 * only one of them.
 */

			Rank rankToFlush=kmerToSend.hash_function_1()%m_parameters->getSize();
			
			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_bufferedData.addAt(rankToFlush,kmerToSend.getU64(i));
			}

			if(m_bufferedData.flush(rankToFlush,KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_VERTICES_DATA,
				m_outboxAllocator,m_outbox,
				m_parameters->getRank(),false)){

				m_pendingMessages++;
			}

			MACRO_COLLECT_PROFILING_INFORMATION();
		}

		(m_mode_send_vertices_sequence_id_position++);
		if((m_mode_send_vertices_sequence_id_position)==maximumPosition){
			(m_mode_send_vertices_sequence_id)++;
			(m_mode_send_vertices_sequence_id_position)=0;
		}
			
		MACRO_COLLECT_PROFILING_INFORMATION();
	}

	MACRO_COLLECT_PROFILING_INFORMATION();
}

void KmerAcademyBuilder::setProfiler(Profiler*profiler){
	m_profiler = profiler;
}

void KmerAcademyBuilder::constructor(int size,Parameters*parameters,GridTable*graph,
	ArrayOfReads*myReads,StaticVector*inbox,StaticVector*outbox,
SlaveMode*mode,RingAllocator*outboxAllocator){


	m_mode=mode;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_inbox=inbox;

	m_myReads=myReads;
	m_checkedCheckpoint=false;
	m_subgraph=graph;
	m_parameters=parameters;
	m_finished=false;
	m_distributionIsCompleted=false;

	m_reverseComplementVertex=false;// TODO: remove this

	m_mode_send_vertices_sequence_id=0;
	m_mode_send_vertices_sequence_id_position=0;
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),"RAY_MALLOC_TYPE_KMER_ACADEMY_BUFFER",m_parameters->showMemoryAllocations(),KMER_U64_ARRAY_SIZE);
	
	m_pendingMessages=0;
	m_size=size;

	m_initialised=false;

	m_rank=m_parameters->getRank();
}

void KmerAcademyBuilder::setReadiness(){
	#ifdef ASSERT
	assert(m_pendingMessages>0);
	#endif
	m_pendingMessages--;
}

void KmerAcademyBuilder::flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox,int rank){
	if(!m_bufferedData.isEmpty()){
		m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_VERTICES_DATA,
			m_outboxAllocator,m_outbox,rank);
		return;
	}
}

bool KmerAcademyBuilder::finished(){
	return m_finished;
}

void KmerAcademyBuilder::assertBuffersAreEmpty(){
	assert(m_bufferedData.isEmpty());
	assert(m_mode_send_vertices_sequence_id_position==0);
}

void KmerAcademyBuilder::incrementPendingMessages(){
	m_pendingMessages++;
}

void KmerAcademyBuilder::showBuffers(){
	#ifdef ASSERT
	assert(m_bufferedData.isEmpty());
	#endif
}

bool KmerAcademyBuilder::isDistributionCompleted(){
	return m_distributionIsCompleted;
}

void KmerAcademyBuilder::setDistributionAsCompleted(){
	m_distributionIsCompleted=true;
}

void KmerAcademyBuilder::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();

	m_plugin=plugin;

	core->setPluginName(plugin,"KmerAcademyBuilder");
	core->setPluginDescription(plugin,"Filter out almost all sequencing errors with a Bloom filter and an academy");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_SLAVE_MODE_ADD_VERTICES=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_ADD_VERTICES, __GetAdapter(KmerAcademyBuilder,RAY_SLAVE_MODE_ADD_VERTICES));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_ADD_VERTICES,"RAY_SLAVE_MODE_ADD_VERTICES");


}

void KmerAcademyBuilder::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_ADD_VERTICES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ADD_VERTICES");

	RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED");
	RAY_MPI_TAG_VERTICES_DATA_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTICES_DATA_REPLY");
	RAY_MPI_TAG_VERTICES_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTICES_DATA");

	__BindPlugin(KmerAcademyBuilder);

}
