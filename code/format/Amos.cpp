/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#include <format/Amos.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <communication/mpi_tags.h>
#include <memory/malloc_types.h>
#include <communication/Message.h>
#include <core/constants.h>
#include <core/master_modes.h>
#include <core/slave_modes.h>
#include <core/Parameters.h>

void Amos::constructor(Parameters*parameters,RingAllocator*outboxAllocator,StaticVector*outbox,
	FusionData*fusionData,ExtensionData*extensionData,int*masterMode,int*slaveMode,Scaffolder*scaffolder){
	m_slave_mode=slaveMode;
	m_master_mode=masterMode;
	m_scaffolder=scaffolder;
	m_parameters=parameters;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_fusionData=fusionData;
	m_ed=extensionData;
}

void Amos::masterMode(){
	if(!m_ed->m_EXTENSION_currentRankIsStarted){
		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
		message[0]=m_ed->m_EXTENSION_currentPosition;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_ed->m_EXTENSION_rank,RAY_MPI_TAG_WRITE_AMOS,m_parameters->getRank());
		m_outbox->push_back(aMessage);
		m_ed->m_EXTENSION_rank++;
		m_ed->m_EXTENSION_currentRankIsDone=false;
		m_ed->m_EXTENSION_currentRankIsStarted=true;
	}else if(m_ed->m_EXTENSION_currentRankIsDone){
		if(m_ed->m_EXTENSION_rank<m_parameters->getSize()){
			m_ed->m_EXTENSION_currentRankIsStarted=false;
		}else{
			*m_master_mode=RAY_MASTER_MODE_SCAFFOLDER;
			m_scaffolder->m_numberOfRanksFinished=0;
		}
	}
}

void Amos::slaveMode(){
	if(!m_ed->m_EXTENSION_initiated){
		cout<<"Rank "<<m_parameters->getRank()<<" is appending positions to "<<m_parameters->getAmosFile()<<endl;
		m_amosFile=fopen(m_parameters->getAmosFile().c_str(),"a+");
		m_contigId=0;
		m_mode_send_vertices_sequence_id_position=0;
		m_ed->m_EXTENSION_initiated=true;
		m_ed->m_EXTENSION_reads_requested=false;
		m_sequence_id=0;
	}
	/*
	* use m_allPaths and m_identifiers
	*
	* iterators: m_SEEDING_i: for the current contig
	*            m_mode_send_vertices_sequence_id_position: for the current position in the current contig.
	*/

	if(true || m_contigId==(int)m_ed->m_EXTENSION_contigs.size()){// all contigs are processed
		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
		message[0]=m_ed->m_EXTENSION_currentPosition;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_WRITE_AMOS_REPLY,m_parameters->getRank());
		m_outbox->push_back(aMessage);
		fclose(m_amosFile);
		*m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
		//cout<<"Rank "<<m_rank<<" appended "<<m_sequence_id<<" elements"<<endl;
		//TODO: AMOS FILE OUTPUT IS disabled for now.
	// iterate over the next one
	}else if(m_fusionData->m_FUSION_eliminated.count(m_ed->m_EXTENSION_identifiers[m_contigId])>0){
		m_contigId++;
		m_mode_send_vertices_sequence_id_position=0;
		m_ed->m_EXTENSION_reads_requested=false;
	}else if(m_mode_send_vertices_sequence_id_position==(int)m_ed->m_EXTENSION_contigs[m_contigId].size()){
		m_contigId++;
		m_mode_send_vertices_sequence_id_position=0;
		m_ed->m_EXTENSION_reads_requested=false;
		
		fprintf(m_amosFile,"}\n");
	}else{
		if(!m_ed->m_EXTENSION_reads_requested){
			if(m_mode_send_vertices_sequence_id_position==0){
				string seq=convertToString(&(m_ed->m_EXTENSION_contigs[m_contigId]),m_parameters->getWordSize());
				char*qlt=(char*)__Malloc(seq.length()+1,RAY_MASTER_MODE_AMOS);
				strcpy(qlt,seq.c_str());
				for(int i=0;i<(int)strlen(qlt);i++){
					qlt[i]='D';
				}
				m_sequence_id++;
				fprintf(m_amosFile,"{CTG\niid:%u\neid:contig-%lu\ncom:\nSoftware: Ray, MPI rank: %i\n.\nseq:\n%s\n.\nqlt:\n%s\n.\n",
					m_ed->m_EXTENSION_currentPosition+1,
					m_ed->m_EXTENSION_identifiers[m_contigId],
					m_parameters->getRank(),
					seq.c_str(),
					qlt
					);

				m_ed->m_EXTENSION_currentPosition++;
				__Free(qlt,RAY_MALLOC_TYPE_AMOS);
			}

			m_ed->m_EXTENSION_reads_requested=true;
			m_ed->m_EXTENSION_reads_received=false;
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
			Kmer vertex=m_ed->m_EXTENSION_contigs[m_contigId][m_mode_send_vertices_sequence_id_position];
			int pos=0;
			vertex.pack(message,&pos);
			//Message aMessage(message,pos,MPI_UNSIGNED_LONG_LONG,vertexRank(vertex,getSize(),m_wordSize),RAY_MPI_TAG_REQUEST_READS,getRank());
			//m_outbox.push_back(aMessage);
			//TODO: code is broken



			// iterator on reads
			m_fusionData->m_FUSION_path_id=0;
			m_ed->m_EXTENSION_readLength_requested=false;
		}else if(m_ed->m_EXTENSION_reads_received){
			if(m_fusionData->m_FUSION_path_id<(int)m_ed->m_EXTENSION_receivedReads.size()){
				int readRank=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getRank();
				char strand=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getStrand();
				int idOnRank=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getReadIndex();
				if(!m_ed->m_EXTENSION_readLength_requested){
					m_ed->m_EXTENSION_readLength_requested=true;
					m_ed->m_EXTENSION_readLength_received=false;
					uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
					message[0]=idOnRank;
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,readRank,RAY_MPI_TAG_ASK_READ_LENGTH,m_parameters->getRank());
					m_outbox->push_back(aMessage);
				}else if(m_ed->m_EXTENSION_readLength_received){
					int readLength=m_ed->m_EXTENSION_receivedLength;
					int globalIdentifier=m_parameters->getGlobalIdFromRankAndLocalId(readRank,idOnRank);
					int start=0;
					int theEnd=readLength-1;
					int offset=m_mode_send_vertices_sequence_id_position;
					if(strand=='R'){
						int t=start;
						start=theEnd;
						theEnd=t;
						offset++;
					}
					fprintf(m_amosFile,"{TLE\nsrc:%i\noff:%i\nclr:%i,%i\n}\n",globalIdentifier+1,offset,
						start,theEnd);
		
					// increment to get the next read.
					m_fusionData->m_FUSION_path_id++;
					m_ed->m_EXTENSION_readLength_requested=false;
				}
			}else{
				// continue.
				m_mode_send_vertices_sequence_id_position++;
				m_ed->m_EXTENSION_reads_requested=false;
			}
		}

	}

}
