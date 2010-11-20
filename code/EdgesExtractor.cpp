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


#include<EdgesExtractor.h>
#include<Message.h>
#include<string.h>
#include<common_functions.h>

EdgesExtractor::EdgesExtractor(){
	m_mode_send_edge_sequence_id=0;
	m_reverseComplementEdge=false;
	m_mode_send_edge_sequence_id_position=0;
}

void EdgesExtractor::processOutgoingEdges(){

	#ifdef SHOW_PROGRESS
	if((m_mode_send_edge_sequence_id)%100000==0 and (m_mode_send_edge_sequence_id_position)==0){
		string strand="";
		if(m_reverseComplementEdge)
			strand="(reverse complement)";
		cout<<"Rank "<<getRank<<" is extracting outgoing edges "<<strand<<" "<<(m_mode_send_edge_sequence_id)+1<<"/"<<m_myReads->size()<<endl;
	}
	#endif

	if((m_mode_send_edge_sequence_id)>(int)m_myReads->size()-1){
		if(m_reverseComplementEdge==false){
			(m_mode_send_edge_sequence_id_position)=0;
			m_reverseComplementEdge=true;
			flushOutgoingEdges(1);
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank<<" is extracting outgoing edges "<<m_myReads->size()<<"/"<<m_myReads->size()<<" (completed)"<<endl;
			#endif
			(m_mode_send_edge_sequence_id)=0;
		}else{
			flushOutgoingEdges(1);
			(*m_mode_send_outgoing_edges)=false;
			(*m_mode)=MODE_PROCESS_INGOING_EDGES;
			(*m_mode_send_ingoing_edges)=true;
			(m_mode_send_edge_sequence_id_position)=0;
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank<<" is extracting outgoing edges (reverse complement) "<<m_myReads->size()<<"/"<<m_myReads->size()<<" (completed)"<<endl;
			#endif
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
			if(m_reverseComplementEdge){
				VERTEX_TYPE b_1=complementVertex(a_2,m_wordSize,m_colorSpaceMode);
				VERTEX_TYPE b_2=complementVertex(a_1,m_wordSize,m_colorSpaceMode);
				int rankB=vertexRank(b_1,getSize);
				m_disData->m_messagesStockOut.addAt(rankB,b_1);
				m_disData->m_messagesStockOut.addAt(rankB,b_2);
			}else{
				int rankA=vertexRank(a_1,getSize);
				m_disData->m_messagesStockOut.addAt(rankA,a_1);
				m_disData->m_messagesStockOut.addAt(rankA,a_2);
			}
			
			flushOutgoingEdges(MAX_UINT64_T_PER_MESSAGE);
		}
		
		(m_mode_send_edge_sequence_id_position)++;


	}
}

void EdgesExtractor::flushOutgoingEdges(int threshold){
	for(int rankId=0;rankId<(int)getSize;rankId++){
		int destination=rankId;
		int length=m_disData->m_messagesStockOut.size(rankId);
		if(length<threshold)
			continue;
		#ifdef SHOW_PROGRESS
		#endif
		VERTEX_TYPE*data=(VERTEX_TYPE*)(*m_outboxAllocator).allocate(sizeof(VERTEX_TYPE)*(length));
		for(int j=0;j<(int)length;j++){
			data[j]=m_disData->m_messagesStockOut.getAt(rankId,j);
		}
		m_disData->m_messagesStockOut.reset(rankId);
		Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_OUT_EDGES_DATA,getRank);
		(*m_outbox).push_back(aMessage);
	}
}

void EdgesExtractor::processIngoingEdges(){
	#ifdef SHOW_PROGRESS
	if(m_mode_send_edge_sequence_id%100000==0 and m_mode_send_edge_sequence_id_position==0){
		string strand="";
		if(m_reverseComplementEdge)
			strand="(reverse complement)";
		cout<<"Rank "<<getRank<<" is extracting ingoing edges "<<strand<<" "<<m_mode_send_edge_sequence_id+1<<"/"<<m_myReads->size()<<endl;
	}
	#endif

	if(m_mode_send_edge_sequence_id>(int)m_myReads->size()-1){
		if(m_reverseComplementEdge==false){
			m_reverseComplementEdge=true;
			m_mode_send_edge_sequence_id_position=0;
			flushIngoingEdges(1);
			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank<<" is extracting ingoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads->size()<<" (completed)"<<endl;
			#endif
			m_mode_send_edge_sequence_id=0;
		}else{
			flushIngoingEdges(1);
			Message aMessage(NULL,0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_EDGES_DISTRIBUTED,getRank);
			m_outbox->push_back(aMessage);
			(*m_mode_send_ingoing_edges)=false;
			(*m_mode)=MODE_DO_NOTHING;

			#ifdef SHOW_PROGRESS
			cout<<"Rank "<<getRank<<" is extracting ingoing edges (reverse complement) "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads->size()<<" (completed)"<<endl;
			#endif
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
			if(m_reverseComplementEdge){
				VERTEX_TYPE b_1=complementVertex(a_2,m_wordSize,m_colorSpaceMode);
				VERTEX_TYPE b_2=complementVertex(a_1,m_wordSize,m_colorSpaceMode);
				int rankB=vertexRank(b_2,getSize);
				m_disData->m_messagesStockIn.addAt(rankB,b_1);
				m_disData->m_messagesStockIn.addAt(rankB,b_2);
			}else{
				int rankA=vertexRank(a_2,getSize);
				m_disData->m_messagesStockIn.addAt(rankA,a_1);
				m_disData->m_messagesStockIn.addAt(rankA,a_2);
			}

			// flush data
			flushIngoingEdges(MAX_UINT64_T_PER_MESSAGE);
		}

		m_mode_send_edge_sequence_id_position++;


		if(m_mode_send_edge_sequence_id_position>lll){
			m_mode_send_edge_sequence_id++;
			m_mode_send_edge_sequence_id_position=0;
		}
	}
}


void EdgesExtractor::flushIngoingEdges(int threshold){
	// send messages
	for(int rankId=0;rankId<getSize;rankId++){
		int destination=rankId;
		int length=m_disData->m_messagesStockIn.size(rankId);
		if(length<threshold)
			continue;
		VERTEX_TYPE*data=(VERTEX_TYPE*)m_outboxAllocator->allocate(sizeof(VERTEX_TYPE)*(length));
		for(int j=0;j<(int)length;j++){
			data[j]=m_disData->m_messagesStockIn.getAt(rankId,j);
		}
		m_disData->m_messagesStockIn.reset(rankId);

		Message aMessage(data,length,MPI_UNSIGNED_LONG_LONG,destination,TAG_IN_EDGES_DATA,getRank);
		m_outbox->push_back(aMessage);
	}
}



