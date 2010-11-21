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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#include<SequencesIndexer.h>
#include<string.h>
#include<stdlib.h>

#ifdef ASSERT
#include<assert.h>
#endif
#include<Parameters.h>
#include<DistributionData.h>
#include<Loader.h>
#include<common_functions.h>
#include<Message.h>

void SequencesIndexer::attachReads(vector<Read*>*m_myReads,
				RingAllocator*m_outboxAllocator,
				StaticVector*m_outbox,
				int*m_mode,
				int m_wordSize,
				BufferedData*m_bufferedData,
				int m_size,
				int m_rank,
				bool m_colorSpaceMode
			){
	// when done: call_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY to root
	// the tag: TAG_ATTACH_SEQUENCE

	if(m_theSequenceId==(int)m_myReads->size()){

		cout<<"Rank "<<m_rank<<" is indexing sequence reads "<<m_myReads->size()<<"/"<<m_myReads->size()<<" (completed)"<<endl;
		m_bufferedData->flushAll(4,TAG_ATTACH_SEQUENCE,m_outboxAllocator,m_outbox,m_rank);
		(*m_mode)=MODE_DO_NOTHING;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,m_rank);
		m_outbox->push_back(aMessage);
		return;
	}
	#ifdef ASSERT
	assert(m_theSequenceId<(int)m_myReads->size());
	#endif
	char*sequence=m_myReads->at(m_theSequenceId)->getSeq();
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
		VERTEX_TYPE vertex=wordId(vertexChar);
		int sendTo=vertexRank(vertex,m_size);
		m_bufferedData->addAt(sendTo,vertex);
		m_bufferedData->addAt(sendTo,m_rank);
		m_bufferedData->addAt(sendTo,m_theSequenceId);
		m_bufferedData->addAt(sendTo,(VERTEX_TYPE)'F');

		m_bufferedData->flush(sendTo,4,TAG_ATTACH_SEQUENCE,m_outboxAllocator,m_outbox,m_rank,false);
	}


	memcpy(vertexChar,sequence+strlen(sequence)-(m_wordSize),m_wordSize);
	vertexChar[m_wordSize]='\0';
	if(isValidDNA(vertexChar)){
		VERTEX_TYPE vertex=complementVertex(wordId(vertexChar),m_wordSize,(m_colorSpaceMode));
		int sendTo=vertexRank(vertex,m_size);
		m_bufferedData->addAt(sendTo,vertex);
		m_bufferedData->addAt(sendTo,m_rank);
		m_bufferedData->addAt(sendTo,m_theSequenceId);
		m_bufferedData->addAt(sendTo,(VERTEX_TYPE)'R');
		m_bufferedData->flush(sendTo,4,TAG_ATTACH_SEQUENCE,m_outboxAllocator,m_outbox,m_rank,false);
	}


	if(m_theSequenceId%1000000==0){
		cout<<"Rank "<<m_rank<<" is indexing sequence reads "<<m_theSequenceId+1<<"/"<<m_myReads->size()<<endl;
	}
	m_theSequenceId++;
}

