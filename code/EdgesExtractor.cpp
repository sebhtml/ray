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

#include<EdgesExtractor.h>
#include<Message.h>
#include<string.h>
#include<common_functions.h>

void EdgesExtractor::setReadiness(){
	m_ready=true;
}

EdgesExtractor::EdgesExtractor(){
	setReadiness();
	m_mode_send_edge_sequence_id=0;
	m_reverseComplementEdge=false;
	m_mode_send_edge_sequence_id_position=0;
}

void EdgesExtractor::constructor(int size){
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
}

void EdgesExtractor::processOutgoingEdges(){

	if((m_mode_send_edge_sequence_id)%100000==0 and (m_mode_send_edge_sequence_id_position)==0){
		string strand="";
		if(m_reverseComplementEdge){
			strand="(reverse complement) ";
		}
		printf("Rank %i is adding outgoing edges %s%i/%i\n",getRank,strand.c_str(),(int)(m_mode_send_edge_sequence_id)+1,(int)m_myReads->size());
		fflush(stdout);
	}

	if((m_mode_send_edge_sequence_id)>(int)m_myReads->size()-1){
		if(m_reverseComplementEdge==false){
			(m_mode_send_edge_sequence_id_position)=0;
			m_reverseComplementEdge=true;
			m_bufferedData.flushAll(TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,getRank);
			printf("Rank %i is adding outgoing edges %i/%i (completed)\n",getRank,(int)m_myReads->size(),(int)m_myReads->size());
			fflush(stdout);
			(m_mode_send_edge_sequence_id)=0;
		}else{
			m_bufferedData.flushAll(TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,getRank);
			(*m_mode_send_outgoing_edges)=false;
			(*m_mode)=MODE_PROCESS_INGOING_EDGES;
			(*m_mode_send_ingoing_edges)=true;
			(m_mode_send_edge_sequence_id_position)=0;
			printf("Rank %i is adding outgoing edges (reverse complement) %i/%i (completed)\n",getRank,(int)m_myReads->size(),(int)m_myReads->size());
			fflush(stdout);
			(m_mode_send_edge_sequence_id)=0;
			m_reverseComplementEdge=false;
		}
	}else{
		char*readSequence=(*m_myReads)[(m_mode_send_edge_sequence_id)]->getSeq();
		int len=strlen(readSequence);
		char memory[100];
		int lll=len-m_wordSize-1;
		if((m_mode_send_edge_sequence_id_position)>lll){
			(m_mode_send_edge_sequence_id)++;
			(m_mode_send_edge_sequence_id_position)=0;
			return;
		}

		int p=(m_mode_send_edge_sequence_id_position);
		memcpy(memory,readSequence+p,m_wordSize+1);
		memory[m_wordSize+1]='\0';
		if(isValidDNA(memory)){
			char prefix[100];
			char suffix[100];
			strcpy(prefix,memory);
			prefix[m_wordSize]='\0';
			strcpy(suffix,memory+1);
			VERTEX_TYPE a_1=wordId(prefix);
			VERTEX_TYPE a_2=wordId(suffix);
			int rankToFlush=0;
			if(m_reverseComplementEdge){
				VERTEX_TYPE b_1=complementVertex(a_2,m_wordSize,m_colorSpaceMode);
				VERTEX_TYPE b_2=complementVertex(a_1,m_wordSize,m_colorSpaceMode);
				int rankB=vertexRank(b_1,getSize);
				rankToFlush=rankB;
				m_bufferedData.addAt(rankB,b_1);
				m_bufferedData.addAt(rankB,b_2);
			}else{
				int rankA=vertexRank(a_1,getSize);
				rankToFlush=rankA;
				m_bufferedData.addAt(rankA,a_1);
				m_bufferedData.addAt(rankA,a_2);
			}
			
			if(m_bufferedData.flush(rankToFlush,2,TAG_OUT_EDGES_DATA,m_outboxAllocator,m_outbox,getRank,false)){
				m_ready=false;
			}
		}
		
		(m_mode_send_edge_sequence_id_position)++;
	}
}

void EdgesExtractor::processIngoingEdges(){
	#ifdef SHOW_PROGRESS
	if(m_mode_send_edge_sequence_id%100000==0 and m_mode_send_edge_sequence_id_position==0){
		string strand="";
		if(m_reverseComplementEdge)
			strand="(reverse complement) ";
		cout<<"Rank "<<getRank<<" is adding ingoing edges "<<strand<<""<<m_mode_send_edge_sequence_id+1<<"/"<<m_myReads->size()<<endl;
	}
	#endif

	if(m_mode_send_edge_sequence_id>(int)m_myReads->size()-1){
		if(m_reverseComplementEdge==false){
			m_reverseComplementEdge=true;
			m_mode_send_edge_sequence_id_position=0;
			m_bufferedData.flushAll(TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,getRank);
		
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank<<" is adding ingoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads->size()<<" (completed)"<<endl;
			#endif
			m_mode_send_edge_sequence_id=0;
		}else{
			m_bufferedData.flushAll(TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,getRank);
			Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_EDGES_DISTRIBUTED,getRank);
			m_outbox->push_back(aMessage);
			(*m_mode_send_ingoing_edges)=false;
			(*m_mode)=MODE_DO_NOTHING;

			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank<<" is adding ingoing edges (reverse complement) "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads->size()<<" (completed)"<<endl;
			#endif
			m_bufferedData.clear();
		}
	}else{


		char*readSequence=(*m_myReads)[m_mode_send_edge_sequence_id]->getSeq();
		int len=strlen(readSequence);
		char memory[100];
		int lll=len-m_wordSize-1;
		
		if(m_mode_send_edge_sequence_id_position>lll){
			m_mode_send_edge_sequence_id++;
			m_mode_send_edge_sequence_id_position=0;
			return;
		}

			
		memcpy(memory,readSequence+m_mode_send_edge_sequence_id_position,m_wordSize+1);
		memory[m_wordSize+1]='\0';
		if(isValidDNA(memory)){
			char prefix[100];
			char suffix[100];
			strcpy(prefix,memory);
			prefix[m_wordSize]='\0';
			strcpy(suffix,memory+1);
			VERTEX_TYPE a_1=wordId(prefix);
			VERTEX_TYPE a_2=wordId(suffix);
			int rankToFlush=0;
			if(m_reverseComplementEdge){
				VERTEX_TYPE b_1=complementVertex(a_2,m_wordSize,m_colorSpaceMode);
				VERTEX_TYPE b_2=complementVertex(a_1,m_wordSize,m_colorSpaceMode);
				int rankB=vertexRank(b_2,getSize);
				rankToFlush=rankB;
				m_bufferedData.addAt(rankB,b_1);
				m_bufferedData.addAt(rankB,b_2);
			}else{
				int rankA=vertexRank(a_2,getSize);
				rankToFlush=rankA;
				m_bufferedData.addAt(rankA,a_1);
				m_bufferedData.addAt(rankA,a_2);
			}

			// flush data

			if(m_bufferedData.flush(rankToFlush,2,TAG_IN_EDGES_DATA,m_outboxAllocator,m_outbox,getRank,false)){
				m_ready=false;
			}
		}

		m_mode_send_edge_sequence_id_position++;


		if(m_mode_send_edge_sequence_id_position>lll){
			m_mode_send_edge_sequence_id++;
			m_mode_send_edge_sequence_id_position=0;
		}
	}
}



