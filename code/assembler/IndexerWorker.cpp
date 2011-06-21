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

#include <assembler/IndexerWorker.h>
#include <string.h>

void IndexerWorker::constructor(int sequenceId,char*sequence,Parameters*parameters,RingAllocator*outboxAllocator,
	VirtualCommunicator*vc,uint64_t workerId,ArrayOfReads*a){
	m_reads=a;
	m_sequenceId=sequenceId;
	strcpy(m_sequence,sequence);
	m_parameters=parameters;
	m_outboxAllocator=outboxAllocator;
	m_virtualCommunicator=vc;
	m_workerId=workerId;
	m_done=false;
	m_forwardIndexed=false;
	m_reverseIndexed=false;
	m_position=0;
	m_coverageRequested=false;
	m_theLength=strlen(sequence);
	m_vertexIsDone=false;
	m_vertexInitiated=false;
	m_fetchedCoverageValues=false;
}

bool IndexerWorker::isDone(){
	return m_done;
}

void IndexerWorker::work(){
	if(m_done){
		return;
	}else if(!m_fetchedCoverageValues){
		if(m_position>m_theLength-m_parameters->getWordSize()){
			m_fetchedCoverageValues=true;
		}else if(!m_coverageRequested){
			Kmer vertex=kmerAtPosition(m_sequence,m_position,m_parameters->getWordSize(),'F',m_parameters->getColorSpaceMode());
			m_vertices.push_back(vertex);
			int sendTo=m_parameters->_vertexRank(&vertex);
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
			int bufferPosition=0;
			vertex.pack(message,&bufferPosition);
			Message aMessage(message,bufferPosition,MPI_UNSIGNED_LONG_LONG,sendTo,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
			m_coverageRequested=true;
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			int coverage=m_virtualCommunicator->getMessageResponseElements(m_workerId)[0];
			m_coverages.push_back(coverage);
			m_position++;
			m_coverageRequested=false;
		}
	}else if(!m_forwardIndexed){
		if(!m_vertexIsDone){
			int selectedPosition=-1;
			// find a vertex that is not an error and that is not repeated
			for(int i=0;i<(int)m_coverages.size()/2;i++){
				int coverage=m_coverages[i];
				if(coverage>=m_parameters->getMinimumCoverage()/2&&coverage<m_parameters->getPeakCoverage()*2){
					selectedPosition=i;
					break;
				}
			}

			// find a vertex that is not an error 
			if(selectedPosition==-1){
				for(int i=0;i<(int)m_coverages.size();i++){
					int coverage=m_coverages[i];
					if(coverage>=m_parameters->getMinimumCoverage()/2){
						selectedPosition=i;
						break;
					}
				}
	
			}
			// index it
			if(selectedPosition!=-1){
				Kmer vertex=m_vertices[selectedPosition];
				int sendTo=m_parameters->_vertexRank(&vertex);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(5*sizeof(uint64_t));
				int j=0;
				vertex.pack(message,&j);
				message[j++]=m_parameters->getRank();
				message[j++]=m_sequenceId;
				message[j++]=selectedPosition;
				message[j++]='F';
				Message aMessage(message,j,MPI_UNSIGNED_LONG_LONG,sendTo,RAY_MPI_TAG_ATTACH_SEQUENCE,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
				m_vertexIsDone=true;
				m_reads->at(m_workerId)->setForwardOffset(selectedPosition);
			}else{
				m_forwardIndexed=true;
			}
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			m_virtualCommunicator->getMessageResponseElements(m_workerId);
			m_forwardIndexed=true;
			m_reverseIndexed=false;
			m_vertexIsDone=false;
		}
	}else if(!m_reverseIndexed){
		if(!m_vertexIsDone){
			int selectedPosition=-1;
			// find a vertex that is not an error and that is not repeated
			for(int i=(int)m_coverages.size()-1;i>=(int)m_coverages.size()/2;i--){
				int coverage=m_coverages[i];
				if(coverage>=m_parameters->getMinimumCoverage()/2&&coverage<m_parameters->getPeakCoverage()*2){
					selectedPosition=i;
					break;
				}
			}

			// find a vertex that is not an error 
			if(selectedPosition==-1){
				for(int i=(int)m_coverages.size()-1;i>=0;i--){
					int coverage=m_coverages[i];
					if(coverage>=m_parameters->getMinimumCoverage()/2){
						selectedPosition=i;
						break;
					}
				}
	
			}

			// index it
			if(selectedPosition!=-1){
				Kmer vertex=m_parameters->_complementVertex(&(m_vertices[selectedPosition]));
				int sendTo=m_parameters->_vertexRank(&vertex);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(5*sizeof(uint64_t));
				int positionOnStrand=m_theLength-m_parameters->getWordSize()-selectedPosition;
				int j=0;
				vertex.pack(message,&j);
				message[j++]=m_parameters->getRank();
				message[j++]=m_sequenceId;
				message[j++]=positionOnStrand;
				message[j++]='R';
				Message aMessage(message,j,MPI_UNSIGNED_LONG_LONG,sendTo,RAY_MPI_TAG_ATTACH_SEQUENCE,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
				m_vertexIsDone=true;
				m_reads->at(m_workerId)->setReverseOffset(positionOnStrand);
			}else{
				m_reverseIndexed=true;
			}
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			m_virtualCommunicator->getMessageResponseElements(m_workerId);
			m_reverseIndexed=true;
		}

	}else{
		m_done=true;
	}
}

uint64_t IndexerWorker::getWorkerId(){
	return m_workerId;
}


