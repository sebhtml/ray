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

void IndexerWorker::constructor(int sequenceId,Parameters*parameters,RingAllocator*outboxAllocator,
	VirtualCommunicator*vc,uint64_t workerId,ArrayOfReads*a,MyAllocator*allocator,
	ofstream*f,
	map<int,map<int,int> >*forwardStatistics,
	map<int,map<int,int> >*reverseStatistics){

	m_forwardStatistics=forwardStatistics;
	m_reverseStatistics=reverseStatistics;
	m_readMarkerFile=f;
	m_reads=a;
	m_allocator=allocator;
	m_sequenceId=sequenceId;
	m_parameters=parameters;
	m_outboxAllocator=outboxAllocator;
	m_virtualCommunicator=vc;
	m_workerId=workerId;
	m_done=false;
	m_forwardIndexed=false;
	m_reverseIndexed=false;
	m_position=0;
	m_coverageRequested=false;
	m_vertexIsDone=false;
	m_vertexInitiated=false;
	m_fetchedCoverageValues=false;
	m_coverages.constructor();
	m_vertices.constructor();
}

bool IndexerWorker::isDone(){
	return m_done;
}

void IndexerWorker::work(){
	Read*read=m_reads->at(m_workerId);

	#ifdef ASSERT
	assert(read!=NULL);
	#endif

	if(m_done){
		return;
	}else if(!m_fetchedCoverageValues){
		if(m_position>read->length() -m_parameters->getWordSize()){
			m_fetchedCoverageValues=true;
		}else if(!m_coverageRequested){
			Kmer vertex=read->getVertex(m_position,m_parameters->getWordSize(),'F',m_parameters->getColorSpaceMode());
			m_vertices.push_back(vertex,m_allocator);
			int sendTo=m_parameters->_vertexRank(&vertex);
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
			int bufferPosition=0;
			vertex.pack(message,&bufferPosition);
			Message aMessage(message,bufferPosition,sendTo,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
			m_coverageRequested=true;
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			vector<uint64_t> response;
			m_virtualCommunicator->getMessageResponseElements(m_workerId,&response);
			int coverage=response[0];
			m_coverages.push_back(coverage,m_allocator);
			m_position++;
			m_coverageRequested=false;
		}
	}else if(!m_forwardIndexed){
		if(!m_vertexIsDone){
			int selectedPosition=-1;
			// find a vertex that is not an error and that is not repeated
			for(int i=0;i<(int)m_coverages.size()/2;i++){
				int coverage=(m_coverages).at(i);
				if(coverage>=m_parameters->getMinimumCoverage()/2&&coverage<m_parameters->getPeakCoverage()*2){
					selectedPosition=i;
					break;
				}
			}

			// find a vertex that is not an error 
			if(selectedPosition==-1){
				for(int i=0;i<(int)m_coverages.size();i++){
					int coverage=(m_coverages).at(i);
					if(coverage>=m_parameters->getMinimumCoverage()/2){
						selectedPosition=i;
						break;
					}
				}
	
			}
			// index it
			if(selectedPosition!=-1){
				Kmer vertex=(m_vertices).at(selectedPosition);
				int sendTo=m_parameters->_vertexRank(&vertex);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(5*sizeof(uint64_t));
				int j=0;
				vertex.pack(message,&j);
				message[j++]=m_parameters->getRank();
				message[j++]=m_sequenceId;
				message[j++]=selectedPosition;
				message[j++]='F';
				Message aMessage(message,j,sendTo,RAY_MPI_TAG_ATTACH_SEQUENCE,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
				m_vertexIsDone=true;
				m_reads->at(m_workerId)->setForwardOffset(selectedPosition);
			}else{
				m_forwardIndexed=true;
			}
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			vector<uint64_t> response;
			m_virtualCommunicator->getMessageResponseElements(m_workerId,&response);
			m_forwardIndexed=true;
			m_reverseIndexed=false;
			m_vertexIsDone=false;
		}
	}else if(!m_reverseIndexed){
		if(!m_vertexIsDone){
			int selectedPosition=-1;
			// find a vertex that is not an error and that is not repeated
			for(int i=(int)(m_coverages).size()-1;i>=(int)m_coverages.size()/2;i--){
				int coverage=(m_coverages).at(i);
				if(coverage>=m_parameters->getMinimumCoverage()/2&&coverage<m_parameters->getPeakCoverage()*2){
					selectedPosition=i;
					break;
				}
			}

			// find a vertex that is not an error 
			if(selectedPosition==-1){
				for(int i=(int)(m_coverages).size()-1;i>=0;i--){
					int coverage=(m_coverages).at(i);
					if(coverage>=m_parameters->getMinimumCoverage()/2){
						selectedPosition=i;
						break;
					}
				}
	
			}

			// index it
			if(selectedPosition!=-1){
				Kmer tmp=m_vertices.at(selectedPosition);
				Kmer vertex=m_parameters->_complementVertex(&tmp);
				int sendTo=m_parameters->_vertexRank(&vertex);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(5*sizeof(uint64_t));
				int positionOnStrand=read->length()-m_parameters->getWordSize()-selectedPosition;
				int j=0;
				vertex.pack(message,&j);
				message[j++]=m_parameters->getRank();
				message[j++]=m_sequenceId;
				message[j++]=positionOnStrand;
				message[j++]='R';
				Message aMessage(message,j,sendTo,RAY_MPI_TAG_ATTACH_SEQUENCE,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
				m_vertexIsDone=true;
				m_reads->at(m_workerId)->setReverseOffset(positionOnStrand);
			}else{
				m_reverseIndexed=true;
			}
		}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
			vector<uint64_t> response;
			m_virtualCommunicator->getMessageResponseElements(m_workerId,&response);
			m_reverseIndexed=true;
		}

	}else{
		if(m_parameters->hasOption("-write-read-markers")){
			#ifdef ASSERT
			assert(m_readMarkerFile != NULL);
			#endif

			// append read marker information to a file.
			(*m_readMarkerFile)<<m_sequenceId<<" Count: "<<m_coverages.size();

			#ifdef ASSERT
			assert(m_workerId < m_reads->size());
			#endif

			(*m_readMarkerFile)<<" Selections:";
			(*m_readMarkerFile)<<" "<<m_reads->at(m_workerId)->getForwardOffset();
			(*m_readMarkerFile)<<" "<<m_reads->at(m_workerId)->getReverseOffset();

			(*m_readMarkerFile)<<" Values:";
			for(int i=0;i<(int)m_coverages.size();i++){
				(*m_readMarkerFile)<<" "<<i<<" "<<m_coverages.at(i);
			}
			(*m_readMarkerFile)<<endl;

		}

		if(m_parameters->hasOption("-write-marker-summary")){
			#ifdef ASSERT
			assert(m_workerId < m_reads->size());
			#endif

			int forwardOffset = m_reads->at(m_workerId)->getForwardOffset();

			if(forwardOffset < m_coverages.size() && m_coverages.size() > 0){
				#ifdef ASSERT
				assert(m_coverages.size() > 0);
				#endif

				int forwardCoverage = m_coverages.at(forwardOffset);
				(*m_forwardStatistics)[forwardOffset][forwardCoverage] ++ ;
			}else{
				// invalid selection, probably because there are nothing to poke around
				(*m_forwardStatistics)[-1][-1] ++ ;
			}

			int reverseOffset = m_reads->at(m_workerId)->getReverseOffset();

			if(reverseOffset < m_coverages.size() && m_coverages.size() > 0){
				#ifdef ASSERT
				assert(m_coverages.size() > 0);
				#endif

				int reverseCoverage = m_coverages.at(m_coverages.size()- 1 - reverseOffset);
				(*m_reverseStatistics)[reverseOffset][reverseCoverage] ++ ;
			}else{
				// invalid selection, probably because there are nothing to poke around
				(*m_reverseStatistics)[-1][-1] ++ ;
			}
		}

		m_vertices.destructor(m_allocator);
		m_coverages.destructor(m_allocator);
		m_done=true;
	}
}

uint64_t IndexerWorker::getWorkerIdentifier(){
	return m_workerId;
}


