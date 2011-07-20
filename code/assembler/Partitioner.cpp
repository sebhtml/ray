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

#include <assembler/Partitioner.h>

void Partitioner::constructor(RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,
	int*slaveMode,int*masterMode){
	m_outboxAllocator=outboxAllocator;
	m_inbox=inbox;
	m_outbox=outbox;
	m_parameters=parameters;
	m_initiatedMaster=false;
	m_initiatedSlave=false;
	m_slaveMode=slaveMode;
	m_masterMode=masterMode;
	m_loader.constructor(m_parameters->getMemoryPrefix().c_str(),m_parameters->showMemoryAllocations());
}

void Partitioner::masterMethod(){
	/** tell every peer to count entries in files in parallel */
	if(!m_initiatedMaster){
		m_initiatedMaster=true;
		m_ranksDoneCounting=0;
		m_ranksDoneSending=0;
		for(int destination=0;destination<m_parameters->getSize();destination++){
			Message aMessage(NULL,0,destination,RAY_MPI_TAG_COUNT_FILE_ENTRIES,m_parameters->getRank());
			m_outbox->push_back(aMessage);
		}
	/** a peer rank finished counting the entries in its files */
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()== RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY){
		m_ranksDoneCounting++;
		/** all peers have finished */
		if(m_ranksDoneCounting==m_parameters->getSize()){
			for(int destination=0;destination<m_parameters->getSize();destination++){
				Message aMessage(NULL,0,destination,RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS,m_parameters->getRank());
				m_outbox->push_back(aMessage);
			}
		}
	/** a peer send the count for one file */
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()== RAY_MPI_TAG_FILE_ENTRY_COUNT){
		uint64_t*buffer=m_inbox->at(0)->getBuffer();
		int file=buffer[0];
		uint64_t count=buffer[1];
		m_masterCounts[file]=count;
		//cout<<"Rank "<<m_parameters->getRank()<<" received from "<<m_inbox->at(0)->getSource()<<" File "<<file<<" Entries "<<count<<endl;
		/** reply to the peer */
		Message aMessage(NULL,0,m_inbox->at(0)->getSource(),RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY,m_parameters->getRank());
		m_outbox->push_back(aMessage);
	/** a peer finished sending file counts */
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag()== RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY){
		m_ranksDoneSending++;
		/** all peers have finished */
		if(m_ranksDoneSending==m_parameters->getSize()){
			for(int i=0;i<m_parameters->getSize();i++){
				m_parameters->setNumberOfSequences(i,m_masterCounts[i]);
			}
			m_masterCounts.clear();
			(*m_masterMode)=RAY_MASTER_MODE_LOAD_SEQUENCES;
		}
	}
}

void Partitioner::slaveMethod(){
	/** initialize the slave */
	if(!m_initiatedSlave){
		m_initiatedSlave=true;
		m_currentFileToCount=0;
		m_currentlySendingCounts=false;
		m_sentCount=false;
	/** count sequences in a file */
	}else if(m_currentFileToCount<m_parameters->getNumberOfFiles()){
		int rankInCharge=m_currentFileToCount%m_parameters->getSize();
		if(rankInCharge==m_parameters->getRank()){
			/** count the entries in the file */
			string file=m_parameters->getFile(m_currentFileToCount);
			//cout<<"Rank "<<m_parameters->getRank()<<" Reading "<<file<<endl;
			int res=m_loader.load(file,false);
			if(res==EXIT_FAILURE){
				cout<<"Rank "<<m_parameters->getRank()<<" Error: "<<file<<" failed to load properly..."<<endl;
			}
			m_slaveCounts[m_currentFileToCount]=m_loader.size();

			m_loader.clear();

			//cout<<"Rank "<<m_parameters->getRank()<<" File "<<m_currentFileToCount<<" has "<<m_slaveCounts[m_currentFileToCount]<<endl;
		}
		m_currentFileToCount++;
		/* all files were processed, tell control peer that we are done */
		if(m_currentFileToCount==m_parameters->getNumberOfFiles()){
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY,m_parameters->getRank());
			m_outbox->push_back(aMessage);
		}
	/** control peer asks the slave to send counts */
	}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag() == RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS){
		m_currentFileToSend=0;
		m_currentlySendingCounts=true;
	/** sending counts */
	}else if(m_currentlySendingCounts){
		if(m_currentFileToSend<m_parameters->getNumberOfFiles()){
			int rankInCharge=m_currentFileToSend%m_parameters->getSize();
			/** skip the file, we are not in charge */
			if(rankInCharge!=m_parameters->getRank()){
				m_currentFileToSend++;
			/** send the count and wait for a reply to continue */
			}else if(!m_sentCount){
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
				message[0]=m_currentFileToSend;
				message[1]=m_slaveCounts[m_currentFileToSend];
				Message aMessage(message,2,MASTER_RANK,RAY_MPI_TAG_FILE_ENTRY_COUNT,m_parameters->getRank());
				m_outbox->push_back(aMessage);
				m_sentCount=true;
			/** we got a reply, let's continue */
			}else if(m_inbox->size()>0 && m_inbox->at(0)->getTag() == RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY){
				m_sentCount=false;
				m_currentFileToSend++;
			}
		/** all counts were processed, report this to the control peer */
		}else{
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY,m_parameters->getRank());
			m_outbox->push_back(aMessage);
			(*m_slaveMode)=RAY_SLAVE_MODE_DO_NOTHING;
			m_slaveCounts.clear();
		}
	}
}
