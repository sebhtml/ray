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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include<VerticesExtractor.h>
#include<assert.h>
#include<Message.h>
#include<common_functions.h>
#include<DistributionData.h>

void VerticesExtractor::process(int*m_mode_send_vertices_sequence_id,
				vector<Read*>*m_myReads,
				bool*m_reverseComplementVertex,
				int*m_mode_send_vertices_sequence_id_position,
				int rank,
				vector<Message>*m_outbox,
				bool*m_mode_send_vertices,
				int m_wordSize,
				DistributionData*m_disData,
				int size,
				MyAllocator*m_outboxAllocator,
				bool m_colorSpaceMode
				){
	#ifdef SHOW_PROGRESS
	if(*m_mode_send_vertices_sequence_id%100000==0 and *m_mode_send_vertices_sequence_id_position==0){
		string reverse="";
		if(*m_reverseComplementVertex==true)
			reverse="(reverse complement) ";
		cout<<"Rank "<<rank<<" is extracting vertices "<<reverse<<"from sequences "<<*m_mode_send_vertices_sequence_id<<"/"<<m_myReads->size()<<endl;
	}
	#endif

	if(*m_mode_send_vertices_sequence_id>(int)m_myReads->size()-1){
		if(*m_reverseComplementVertex==false){
			// flush data
			flushVertices(1,m_disData,m_outboxAllocator,m_outbox,rank,size);

			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<rank<<" is extracting vertices from sequences "<<*m_mode_send_vertices_sequence_id<<"/"<<m_myReads->size()<<" (DONE)"<<endl;
			#endif
			(*m_mode_send_vertices_sequence_id)=0;
			*m_mode_send_vertices_sequence_id_position=0;
			*m_reverseComplementVertex=true;
		}else{
			// flush data

			flushVertices(1,m_disData,m_outboxAllocator,m_outbox,rank,size);
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_VERTICES_DISTRIBUTED,rank);
			m_outbox->push_back(aMessage);
			*m_mode_send_vertices=false;
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<rank<<" is extracting vertices (reverse complement) from sequences "<<*m_mode_send_vertices_sequence_id<<"/"<<m_myReads->size()<<" (DONE)"<<endl;
			#endif
		}
	}else{
		char*readSequence=(*m_myReads)[(*m_mode_send_vertices_sequence_id)]->getSeq();
		int len=strlen(readSequence);
		char memory[100];
		int lll=len-m_wordSize;
		for(int p=(*m_mode_send_vertices_sequence_id_position);p<=(*m_mode_send_vertices_sequence_id_position);p++){
			#ifdef DEBUG
			assert(readSequence!=NULL);
			assert(m_wordSize<=32);
			#endif
			memcpy(memory,readSequence+p,m_wordSize);
			memory[m_wordSize]='\0';
			if(isValidDNA(memory)){
				VERTEX_TYPE a=wordId(memory);
				if(*m_reverseComplementVertex==false){
					m_disData->m_messagesStock.addAt(vertexRank(a,size),a);
				}else{
					VERTEX_TYPE b=complementVertex(a,m_wordSize,m_colorSpaceMode);
					m_disData->m_messagesStock.addAt(vertexRank(b,size),b);
				}
			}
		}
		(*m_mode_send_vertices_sequence_id_position)++;
		flushVertices(MAX_UINT64_T_PER_MESSAGE,m_disData,m_outboxAllocator,m_outbox,rank,size);

		if(*m_mode_send_vertices_sequence_id_position>lll){
			(*m_mode_send_vertices_sequence_id)++;
			(*m_mode_send_vertices_sequence_id_position)=0;
		}
	}
}

void VerticesExtractor::flushVertices(int threshold,
				DistributionData*m_disData,
				MyAllocator*m_outboxAllocator,
				vector<Message>*m_outbox,
				int rank,int size
){

	// send messages
	for(int rankId=0;rankId<size;rankId++){
		int destination=rankId;
		int length=m_disData->m_messagesStock.size(rankId);

		// accumulate data.
		if(length<threshold)
			continue;

		VERTEX_TYPE *data=(VERTEX_TYPE*)m_outboxAllocator->allocate(sizeof(VERTEX_TYPE)*length);
		for(int j=0;j<(int)length;j++){
			data[j]=m_disData->m_messagesStock.getAt(rankId,j);
		}
		m_disData->m_messagesStock.reset(rankId);

		Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_VERTICES_DATA,rank);
		m_outbox->push_back(aMessage);
	}
}

