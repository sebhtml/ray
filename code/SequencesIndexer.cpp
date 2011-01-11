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

#include<SequencesIndexer.h>
#include<string.h>
#include<stdlib.h>
#include<assert.h>
#include<Parameters.h>
#include<Loader.h>
#include<common_functions.h>
#include<Message.h>

void SequencesIndexer::attachReads(ArrayOfReads*m_myReads,
				RingAllocator*m_outboxAllocator,
				StaticVector*m_outbox,
				int*m_mode,
				int m_wordSize,
				int m_size,
				int m_rank,
				bool m_colorSpaceMode
			){
	if(m_pendingMessages!=0){
		return;
	}

	// when done: call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY to root
	// the tag: RAY_MPI_TAG_ATTACH_SEQUENCE

	if(m_theSequenceId==(int)m_myReads->size()){
		if(!m_bufferedData.isEmpty()){
			m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_ATTACH_SEQUENCE,m_outboxAllocator,m_outbox,m_rank);
			return;
		}

		printf("Rank %i is indexing sequence reads [%i/%i] (completed)\n",m_rank,(int)m_myReads->size(),(int)m_myReads->size());
		fflush(stdout);
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,m_rank);
		m_outbox->push_back(aMessage);
		m_bufferedData.clear();

		return;
	}
	#ifdef ASSERT
	assert(m_theSequenceId<(int)m_myReads->size());
	#endif
	string aSeq=m_myReads->at(m_theSequenceId)->getSeq();
	const char*sequence=aSeq.c_str();
	int theLength=strlen(sequence);
	if((int)theLength<m_wordSize){
		m_theSequenceId++;
		return;
	}
	#ifdef ASSERT
	assert(theLength>=m_wordSize);
	#endif
	char vertexChar[100];
	memcpy(vertexChar,sequence,m_wordSize);
	vertexChar[m_wordSize]='\0';
	if(isValidDNA(vertexChar)){
		uint64_t vertex=wordId(vertexChar);
		int sendTo=vertexRank(vertex,m_size);
		m_bufferedData.addAt(sendTo,vertex);
		m_bufferedData.addAt(sendTo,m_rank);

		#ifdef ASSERT
		assert(m_rank<m_size);
		assert(m_rank>=0);
		#endif

		m_bufferedData.addAt(sendTo,m_theSequenceId);
		m_bufferedData.addAt(sendTo,(uint64_t)'F');

		if(m_bufferedData.flush(sendTo,4,RAY_MPI_TAG_ATTACH_SEQUENCE,m_outboxAllocator,m_outbox,m_rank,false)){
			m_pendingMessages++;
		}
	}


	memcpy(vertexChar,sequence+strlen(sequence)-(m_wordSize),m_wordSize);
	vertexChar[m_wordSize]='\0';
	if(isValidDNA(vertexChar)){
		uint64_t vertex=complementVertex(wordId(vertexChar),m_wordSize,(m_colorSpaceMode));
		int sendTo=vertexRank(vertex,m_size);
		m_bufferedData.addAt(sendTo,vertex);
		m_bufferedData.addAt(sendTo,m_rank);
		m_bufferedData.addAt(sendTo,m_theSequenceId);
		m_bufferedData.addAt(sendTo,(uint64_t)'R');
		if(m_bufferedData.flush(sendTo,4,RAY_MPI_TAG_ATTACH_SEQUENCE,m_outboxAllocator,m_outbox,m_rank,false)){
			m_pendingMessages++;
		}
	}


	if(m_theSequenceId%1000000==0){
		printf("Rank %i is indexing sequence reads [%i/%i]\n",m_rank,m_theSequenceId+1,(int)m_myReads->size());
		fflush(stdout);
	}
	m_theSequenceId++;
}

void SequencesIndexer::constructor(int m_size){
	m_bufferedData.constructor(m_size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t));
	m_pendingMessages=0;
}

void SequencesIndexer::setReadiness(){
	m_pendingMessages--;
}
