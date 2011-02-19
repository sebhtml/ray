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

#include <IndexerWorker.h>

void IndexerWorker::constructor(int sequenceId,char*sequence,Parameters*parameters,RingAllocator*outboxAllocator,
	VirtualCommunicator*vc,uint64_t workerId){
	m_sequenceId=sequenceId;
	m_sequence=sequence;
	m_parameters=parameters;
	m_outboxAllocator=outboxAllocator;
	m_virtualCommunicator=vc;
	m_workerId=workerId;
	m_done=false;
	m_forwardIndexed=false;
	m_reverseIndexed=false;
	m_position=0;
	m_theLength=strlen(sequence);
	m_vertexIsDone=false;
	m_gotIndexed=false;
	m_vertexInitiated=false;
	m_currentStrand='F';
}

bool IndexerWorker::isDone(){
	return m_done;
}

void IndexerWorker::work(){
	if(m_done){
		return;
	}else if(!m_forwardIndexed){
		if(m_position>m_theLength-m_parameters->getWordSize()){
			m_done=true;// no solid k-mer found
		}else{
			uint64_t vertex=kmerAtPosition(m_sequence.c_str(),m_position,m_parameters->getWordSize(),m_currentStrand,m_parameters->getColorSpaceMode());
			if(!m_vertexIsDone){
				indexIfGood(vertex);
			}else{// is done
				m_vertexInitiated=false;
				m_vertexIsDone=false;
				if(m_gotIndexed){
					m_forwardIndexed=true;
					m_position=0;
					m_currentStrand='R';
				}else{
					m_position++;
				}
			}
		}
	}else if(!m_reverseIndexed){
		if(m_position>m_theLength-m_parameters->getWordSize()){
			m_done=true;// no solid k-mer found
		}else{
			uint64_t vertex=kmerAtPosition(m_sequence.c_str(),m_position,m_parameters->getWordSize(),m_currentStrand,m_parameters->getColorSpaceMode());
			if(!m_vertexIsDone){
				indexIfGood(vertex);
			}else{// is done
				m_vertexInitiated=false;
				m_vertexIsDone=false;
				if(m_gotIndexed){
					m_reverseIndexed=true;
				}else{
					m_position++;
				}
			}
		}
	}else{
		m_done=true;
	}
}

uint64_t IndexerWorker::getWorkerId(){
	return m_workerId;
}

// RAY_MPI_TAG_ATTACH_SEQUENCE takes 5: vertex, rank, seqId, strandPosition, strand
//      returns nothing
// RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE takes 1: vertex
//      return the coverage
//
//      on completion m_vertexIsDone is set to true
//      for initialisation, set m_vertexInitiated to false
void IndexerWorker::indexIfGood(uint64_t vertex){
	if(m_vertexIsDone){
		return;
	}else if(!m_vertexInitiated){
		m_gotIndexed=false;
		m_coverageRequested=false;
		m_checkedCoverage=false;
		m_vertexInitiated=true;
	}else if(!m_checkedCoverage){
		if(!m_coverageRequested){
			int sendTo=vertexRank(vertex,m_parameters->getSize(),m_parameters->getWordSize());
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
			message[0]=vertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,sendTo,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
			m_coverageRequested=true;
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			int coverage=m_virtualCommunicator->getResponseElements(m_workerId)[0];
			//cout<<"Coverage is "<<coverage<<endl;
			if(coverage<m_parameters->getMinimumCoverage()){
				m_vertexIsDone=true;
				m_gotIndexed=false;
			}else{
				m_checkedCoverage=true;
				m_indexedTheVertex=false;
			}
		}
	}else if(!m_indexedTheVertex){
		int sendTo=vertexRank(vertex,m_parameters->getSize(),m_parameters->getWordSize());
		uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(5*sizeof(uint64_t));
		message[0]=vertex;
		message[1]=m_parameters->getRank();
		message[2]=m_sequenceId;
		message[3]=m_position;
		message[4]=m_currentStrand;
		Message aMessage(message,5,MPI_UNSIGNED_LONG_LONG,sendTo,RAY_MPI_TAG_ATTACH_SEQUENCE,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_indexedTheVertex=true;
	}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
		m_virtualCommunicator->getResponseElements(m_workerId);
		m_gotIndexed=true;
		m_vertexIsDone=true;
	}
}
