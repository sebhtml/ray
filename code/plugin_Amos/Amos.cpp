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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <plugin_Amos/Amos.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <communication/mpi_tags.h>
#include <communication/Message.h>
#include <application_core/constants.h>
#include <core/master_modes.h>
#include <core/slave_modes.h>
#include <application_core/Parameters.h>

__CreatePlugin(Amos);

 /**/
__CreateMasterModeAdapter(Amos,RAY_MASTER_MODE_AMOS); /**/
 /**/
__CreateSlaveModeAdapter(Amos,RAY_SLAVE_MODE_AMOS); /**/
 /**/
 /**/


void Amos::constructor(Parameters*parameters,RingAllocator*outboxAllocator,StaticVector*outbox,
	FusionData*fusionData,ExtensionData*extensionData,int*masterMode,int*slaveMode,Scaffolder*scaffolder,
	StaticVector*inbox,VirtualCommunicator*virtualCommunicator){
	m_virtualCommunicator=virtualCommunicator;
	m_inbox=inbox;
	m_slave_mode=slaveMode;
	m_master_mode=masterMode;
	m_scaffolder=scaffolder;
	m_parameters=parameters;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_fusionData=fusionData;
	m_ed=extensionData;
	m_workerId=0;
}

void Amos::call_RAY_MASTER_MODE_AMOS(){
	if(!m_ed->m_EXTENSION_currentRankIsStarted){
		MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
		message[0]=m_ed->m_EXTENSION_currentPosition;
		Message aMessage(message,1,m_ed->m_EXTENSION_rank,RAY_MPI_TAG_WRITE_AMOS,m_parameters->getRank());
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

void Amos::call_RAY_SLAVE_MODE_AMOS(){
	if(!m_ed->m_EXTENSION_initiated){
		cout<<"Rank "<<m_parameters->getRank()<<" is appending positions to "<<m_parameters->getAmosFile()<<endl;
		m_amosFile=fopen(m_parameters->getAmosFile().c_str(),"a+");
		m_contigId=0;
		m_mode_send_vertices_sequence_id_position=0;
		m_ed->m_EXTENSION_initiated=true;
		m_ed->m_EXTENSION_reads_requested=false;
		m_sequence_id=0;
	}

	m_virtualCommunicator->forceFlush();
	m_virtualCommunicator->processInbox(&m_activeWorkers);
	m_activeWorkers.clear();


	/*
	* use m_allPaths and m_identifiers
	*
	* iterators: m_SEEDING_i: for the current contig
	*            m_mode_send_vertices_sequence_id_position: for the current position in the current contig.
	*/

	if(m_contigId==(int)m_ed->m_EXTENSION_contigs.size()){// all contigs are processed
		MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
		message[0]=m_ed->m_EXTENSION_currentPosition;
		Message aMessage(message,1,MASTER_RANK,RAY_MPI_TAG_WRITE_AMOS_REPLY,m_parameters->getRank());
		m_outbox->push_back(aMessage);
		fclose(m_amosFile);
		*m_slave_mode=RAY_SLAVE_MODE_DO_NOTHING;
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
				string seq=convertToString(&(m_ed->m_EXTENSION_contigs[m_contigId]),m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
				char*qlt=(char*)__Malloc(seq.length()+1,"RAY_MASTER_MODE_AMOS",m_parameters->showMemoryAllocations());
				strcpy(qlt,seq.c_str());
				for(int i=0;i<(int)strlen(qlt);i++){
					qlt[i]='D';
				}
				m_sequence_id++;
	
				#if defined(RAY_64_BITS)
					fprintf(m_amosFile,"{CTG\niid:%u\neid:contig-%lu\ncom:\nSoftware: Ray, MPI rank: %i\n.\nseq:\n%s\n.\nqlt:\n%s\n.\n",
					m_ed->m_EXTENSION_currentPosition+1,
					m_ed->m_EXTENSION_identifiers[m_contigId],
					m_parameters->getRank(),
					seq.c_str(),
					qlt
					);
				#elif defined(RAY_32_BITS)
					fprintf(m_amosFile,"{CTG\niid:%u\neid:contig-%llu\ncom:\nSoftware: Ray, MPI rank: %i\n.\nseq:\n%s\n.\nqlt:\n%s\n.\n",
					m_ed->m_EXTENSION_currentPosition+1,
					m_ed->m_EXTENSION_identifiers[m_contigId],
					m_parameters->getRank(),
					seq.c_str(),
					qlt
					);
				#endif

				m_ed->m_EXTENSION_currentPosition++;
				__Free(qlt,"RAY_MALLOC_TYPE_AMOS",m_parameters->showMemoryAllocations());
			}

			if(m_mode_send_vertices_sequence_id_position%10000==0){
				cout<<"Rank "<<m_parameters->getRank()<<" Exporting AMOS ["<<m_contigId<<"/"<<m_ed->m_EXTENSION_contigs.size()<<"] [";
				cout<<m_mode_send_vertices_sequence_id_position<<"/"<<m_ed->m_EXTENSION_contigs[m_contigId].size()<<"]"<<endl;
			}

			m_ed->m_EXTENSION_reads_requested=true;
			m_ed->m_EXTENSION_reads_received=false;
			Kmer vertex=m_ed->m_EXTENSION_contigs[m_contigId][m_mode_send_vertices_sequence_id_position];

			m_readFetcher.constructor(&vertex,m_outboxAllocator,m_inbox,m_outbox,m_parameters,m_virtualCommunicator,m_workerId,
 RAY_MPI_TAG_REQUEST_VERTEX_READS);

			// iterator on reads
			m_fusionData->m_FUSION_path_id=0;
			m_ed->m_EXTENSION_readLength_requested=false;
		}else if(m_ed->m_EXTENSION_reads_requested && !m_ed->m_EXTENSION_reads_received){
			if(!m_readFetcher.isDone()){
				m_readFetcher.work();
			}else{
				m_ed->m_EXTENSION_reads_received=true;
				m_ed->m_EXTENSION_receivedReads=*(m_readFetcher.getResult());
			}
		}else if(m_ed->m_EXTENSION_reads_received){
			if(m_fusionData->m_FUSION_path_id<(int)m_ed->m_EXTENSION_receivedReads.size()){
				Rank readRank=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getRank();
				Strand strand=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getStrand();
				int idOnRank=m_ed->m_EXTENSION_receivedReads[m_fusionData->m_FUSION_path_id].getReadIndex();
				if(!m_ed->m_EXTENSION_readLength_requested){
					m_ed->m_EXTENSION_readLength_requested=true;
					m_ed->m_EXTENSION_readLength_received=false;
					MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
					message[0]=idOnRank;
					Message aMessage(message,1,readRank,RAY_MPI_TAG_ASK_READ_LENGTH,m_parameters->getRank());
					m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
				}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
					vector<MessageUnit> result;
					m_virtualCommunicator->getMessageResponseElements(m_workerId,&result);
					int readLength=result[0];
					int forwardOffset=result[1];
					int reverseOffset=result[2];
					ReadHandle globalIdentifier=m_parameters->getGlobalIdFromRankAndLocalId(readRank,idOnRank)+1;
					int start=forwardOffset;
					int theEnd=readLength-1;
					int offset=m_mode_send_vertices_sequence_id_position;
					if(strand=='R'){
						start=0;
						theEnd=theEnd-reverseOffset;
						int t=start;
						start=theEnd;
						theEnd=t;
						offset++;
					}
	
					#if defined(RAY_64_BITS)
						fprintf(m_amosFile,"{TLE\nsrc:%li\noff:%i\nclr:%i,%i\n}\n",globalIdentifier,offset,
						start,theEnd);
					#elif defined(RAY_32_BITS)
						fprintf(m_amosFile,"{TLE\nsrc:%lli\noff:%i\nclr:%i,%i\n}\n",globalIdentifier,offset,
						start,theEnd);
					#endif

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

void Amos::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"Amos");
	core->setPluginDescription(plugin,"Writes the assembly in the Amos format");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_SLAVE_MODE_AMOS=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_AMOS, __GetAdapter(Amos,RAY_SLAVE_MODE_AMOS));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_AMOS,"RAY_SLAVE_MODE_AMOS");

	RAY_MASTER_MODE_AMOS=core->allocateMasterModeHandle(plugin);
	core->setMasterModeObjectHandler(plugin,RAY_MASTER_MODE_AMOS, __GetAdapter(Amos,RAY_MASTER_MODE_AMOS));
	core->setMasterModeSymbol(plugin,RAY_MASTER_MODE_AMOS,"RAY_MASTER_MODE_AMOS");
}

void Amos::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_AMOS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_AMOS");
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");

	RAY_MASTER_MODE_AMOS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_AMOS");
	RAY_MASTER_MODE_SCAFFOLDER=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_SCAFFOLDER");

	RAY_MPI_TAG_ASK_READ_LENGTH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_READ_LENGTH");
	RAY_MPI_TAG_WRITE_AMOS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_AMOS");
	RAY_MPI_TAG_WRITE_AMOS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_AMOS_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_READS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_READS");

	__BindPlugin(Amos);
}
