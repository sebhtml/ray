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

#include<string.h>
#include<stdlib.h>
#include<VerticesExtractor.h>
#include<assert.h>
#include<Message.h>
#include<time.h>
#include<StaticVector.h>
#include<common_functions.h>
#include<DistributionData.h>

void VerticesExtractor::process(int*m_mode_send_vertices_sequence_id,
				vector<Read*>*m_myReads,
				bool*m_reverseComplementVertex,
				int*m_mode_send_vertices_sequence_id_position,
				int rank,
				StaticVector*m_outbox,
				bool*m_mode_send_vertices,
				int m_wordSize,
				DistributionData*m_disData,
				int size,
				RingAllocator*m_outboxAllocator,
				bool m_colorSpaceMode,int*m_mode
				){
	if(!m_ready){
		return;
	}
	#ifdef SHOW_PROGRESS
	if(*m_mode_send_vertices_sequence_id%100000==0 and *m_mode_send_vertices_sequence_id_position==0){
		string reverse="";
		if(*m_reverseComplementVertex==true)
			reverse="(reverse complement) ";
		cout<<"Rank "<<rank<<" is computing vertices "<<reverse<<""<<*m_mode_send_vertices_sequence_id+1<<"/"<<m_myReads->size()<<endl;
	}
	#endif

	if(*m_mode_send_vertices_sequence_id>(int)m_myReads->size()-1){
		if(*m_reverseComplementVertex==false){
			// flush data

			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<rank<<" is computing vertices "<<*m_mode_send_vertices_sequence_id<<"/"<<m_myReads->size()<<" (completed)"<<endl;
			#endif
			(*m_mode_send_vertices_sequence_id)=0;
			*m_mode_send_vertices_sequence_id_position=0;
			*m_reverseComplementVertex=true;
		}else{
			// flush data
			m_disData->m_messagesStock.flushAll(1,TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank);
			Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_VERTICES_DISTRIBUTED,rank);
			m_outbox->push_back(aMessage);
			*m_mode_send_vertices=false;
			(*m_mode)=MODE_DO_NOTHING;
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<rank<<" is computing vertices (reverse complement) "<<*m_mode_send_vertices_sequence_id<<"/"<<m_myReads->size()<<" (completed)"<<endl;
			#endif

		}
	}else{
		char*readSequence=(*m_myReads)[(*m_mode_send_vertices_sequence_id)]->getSeq();
		int len=strlen(readSequence);
		char memory[100];
		int lll=len-m_wordSize;
		int p=(*m_mode_send_vertices_sequence_id_position);
		#ifdef DEBUG
		assert(readSequence!=NULL);
		assert(m_wordSize<=32);
		#endif
		memcpy(memory,readSequence+p,m_wordSize);
		memory[m_wordSize]='\0';
		if(isValidDNA(memory)){
			VERTEX_TYPE a=wordId(memory);
			int rankToFlush=0;
			if(*m_reverseComplementVertex==false){
				rankToFlush=vertexRank(a,size);
				m_disData->m_messagesStock.addAt(rankToFlush,a);
			}else{
				VERTEX_TYPE b=complementVertex(a,m_wordSize,m_colorSpaceMode);
				rankToFlush=vertexRank(b,size);
				m_disData->m_messagesStock.addAt(rankToFlush,b);
			}

			if(m_disData->m_messagesStock.flush(rankToFlush,1,TAG_VERTICES_DATA,m_outboxAllocator,m_outbox,rank,false)){
				m_ready=false;
			}

		}
		(*m_mode_send_vertices_sequence_id_position)++;

		if(*m_mode_send_vertices_sequence_id_position>lll){
			(*m_mode_send_vertices_sequence_id)++;
			(*m_mode_send_vertices_sequence_id_position)=0;
		}
	}
}


VerticesExtractor::VerticesExtractor(){
	setReadiness();
}

void VerticesExtractor::setReadiness(){
	m_ready=true;
}
