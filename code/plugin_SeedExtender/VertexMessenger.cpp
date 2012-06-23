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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#include <plugin_SeedExtender/VertexMessenger.h>
#include <communication/Message.h>
#include <core/OperatingSystem.h>

#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <iostream>
using namespace std;

/**
* This method will fetch reads for the k-mer
*/
void VertexMessenger::work(){
	if(m_isDone){
		return;
	}else if(!m_requestedBasicInfo){
		m_requestedBasicInfo=true;
		m_receivedBasicInfo=false;
		MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(3*sizeof(MessageUnit));
		int j=0;
		m_vertex.pack(message,&j);
		message[j++]=m_waveId;
		message[j++]=m_wavePosition;
		Message aMessage(message,j,m_destination,RAY_MPI_TAG_VERTEX_INFO,m_parameters->getRank());
		m_outbox->push_back(aMessage);

	}else if(!m_receivedBasicInfo &&m_inbox->size()==1&&m_inbox->at(0)->getTag()==RAY_MPI_TAG_VERTEX_INFO_REPLY){
		m_receivedBasicInfo=true;
		MessageUnit*buffer=(MessageUnit*)m_inbox->at(0)->getBuffer();
		m_coverageValue=buffer[0];
		m_edges=buffer[1];
		m_numberOfAnnotations=buffer[2];
		m_pointer=(void*)buffer[3];

		int numberOfReadsInMessage=buffer[4];
		int i=0;
		while(i<numberOfReadsInMessage){
			int theRank=buffer[5+4*i];
			int index=buffer[5+4*i+1];
			int strandPosition=buffer[5+4*i+2];
			char strand=buffer[5+4*i+3];

			#ifdef ASSERT
			assert(theRank>=0);
			if(theRank>=m_parameters->getSize()){
				cout<<"Rank="<<theRank<<" Size="<<m_parameters->getSize()<<endl;
			}
			assert(theRank<m_parameters->getSize());
			assert(strand=='F'||strand=='R');
			#endif

			ReadAnnotation e;
			e.constructor(theRank,index,strandPosition,strand,false);
			m_annotations.push_back(e);
			i++;
		}

		if(m_pointer==NULL){
			m_isDone=true;
		}else{
			m_requestedReads=false;
			m_mateIterator=m_matesToMeet->begin();
		}
	}else if(m_receivedBasicInfo){
		if(!m_getReads){
			m_numberOfAnnotations=0;
			m_annotations.clear();
			m_isDone=true;

		}else if(m_coverageValue>= 3* m_peakCoverage){
			getReadsForRepeatedVertex();
		}else{
			getReadsForUniqueVertex();
		}
	}
}

void VertexMessenger::getReadsForRepeatedVertex(){
	if(!m_requestedReads){
		MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int j=0;
		m_vertex.pack(message,&j);
		message[j++]=(MessageUnit)m_pointer;
		int maximumMates=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit)/4;
		
		int processed=0;
		while(processed<maximumMates&&m_mateIterator!=m_matesToMeet->end()){
			ReadHandle mate=*m_mateIterator;
			message[j+1+processed]=mate;
			processed++;
			m_mateIterator++;
		}
		message[j]=processed;
/*
 * <--vertex--><--pointer--><--numberOfMates--><--mates -->
 */
		Message aMessage(message,j+1+processed,
			m_destination,RAY_MPI_TAG_VERTEX_READS_FROM_LIST,m_parameters->getRank());
		m_outbox->push_back(aMessage);
		m_requestedReads=true;
		m_receivedReads=false;
	}else if(!m_receivedReads&&m_inbox->size()==1&&m_inbox->at(0)->getTag()==RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY){
		m_receivedReads=true;
		MessageUnit*buffer=(MessageUnit*)m_inbox->at(0)->getBuffer();
		int numberOfReadsInMessage=buffer[0];
		int i=0;
		while(i<numberOfReadsInMessage){
			int theRank=buffer[1+4*i];
			int index=buffer[1+4*i+1];
			int strandPosition=buffer[1+4*i+2];
			char strand=buffer[1+4*i+3];

			#ifdef ASSERT
			assert(theRank>=0);
			if(theRank>=m_parameters->getSize()){
				cout<<"Rank="<<theRank<<" Size="<<m_parameters->getSize()<<endl;
			}
			assert(theRank<m_parameters->getSize());
			assert(strand=='F'||strand=='R');
			#endif

			ReadAnnotation e;
			e.constructor(theRank,index,strandPosition,strand,false);
			m_annotations.push_back(e);
			i++;
		}
		if(m_mateIterator==m_matesToMeet->end()){
			m_isDone=true;
		}else{
			m_requestedReads=false;
		}
	}
}

void VertexMessenger::getReadsForUniqueVertex(){
	if(!m_requestedReads){
		MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(2*sizeof(MessageUnit));
		int j=0;
		m_vertex.pack(message,&j);
		message[j++]=(MessageUnit)m_pointer;
		Message aMessage(message,j,m_destination,RAY_MPI_TAG_VERTEX_READS,m_parameters->getRank());
		m_outbox->push_back(aMessage);
		m_requestedReads=true;
		m_receivedReads=false;

	}else if(!m_receivedReads&&m_inbox->size()==1&&m_inbox->at(0)->getTag()==RAY_MPI_TAG_VERTEX_READS_REPLY){
		m_receivedReads=true;
		MessageUnit*buffer=(MessageUnit*)m_inbox->at(0)->getBuffer();
		int numberOfReadsInMessage=buffer[0];
		int i=0;
		while(i<numberOfReadsInMessage){
			int theRank=buffer[1+4*i];
			int index=buffer[1+4*i+1];
			int strandPosition=buffer[1+4*i+2];
			char strand=buffer[1+4*i+3];

			#ifdef ASSERT
			assert(theRank>=0);
			if(theRank>=m_parameters->getSize()){
				cout<<"Rank="<<theRank<<" Size="<<m_parameters->getSize()<<endl;
			}
			assert(theRank<m_parameters->getSize());
			assert(strand=='F'||strand=='R');
			#endif

			ReadAnnotation e;
			e.constructor(theRank,index,strandPosition,strand,false);
			m_annotations.push_back(e);
			i++;
		}
		m_pointer=(void*)buffer[1+numberOfReadsInMessage*4];
		if((int)m_annotations.size()==m_numberOfAnnotations){
			m_isDone=true;
		}else{
			m_requestedReads=false;
		}
	}
}

bool VertexMessenger::isDone(){
	return m_isDone;
}

vector<ReadAnnotation> VertexMessenger::getReadAnnotations(){
	return m_annotations;
}

uint8_t VertexMessenger::getEdges(){
	return m_edges;
}

CoverageDepth VertexMessenger::getCoverageValue(){
	return m_coverageValue;
}

void VertexMessenger::constructor(Kmer vertex,PathHandle wave,int pos,
	set<ReadHandle>*matesToMeet,StaticVector*inbox,StaticVector*outbox,
	RingAllocator*outboxAllocator,Parameters*parameters,bool getReads,CoverageDepth peakCoverage,

	MessageTag RAY_MPI_TAG_VERTEX_INFO,
	MessageTag RAY_MPI_TAG_VERTEX_INFO_REPLY,
	MessageTag RAY_MPI_TAG_VERTEX_READS,
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST,
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY,
	MessageTag RAY_MPI_TAG_VERTEX_READS_REPLY
){

	this->RAY_MPI_TAG_VERTEX_INFO=RAY_MPI_TAG_VERTEX_INFO;
	this->RAY_MPI_TAG_VERTEX_INFO_REPLY=RAY_MPI_TAG_VERTEX_INFO_REPLY;
	this->RAY_MPI_TAG_VERTEX_READS=RAY_MPI_TAG_VERTEX_READS;
	this->RAY_MPI_TAG_VERTEX_READS_FROM_LIST=RAY_MPI_TAG_VERTEX_READS_FROM_LIST;
	this->RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY=RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY;
	this->RAY_MPI_TAG_VERTEX_READS_REPLY=RAY_MPI_TAG_VERTEX_READS_REPLY;

	m_peakCoverage=peakCoverage;

	m_getReads=getReads;
	m_inbox=inbox;
	m_outbox=outbox;
	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;
	m_vertex=vertex;
	m_waveId=wave;
	m_wavePosition=pos;
	m_matesToMeet=matesToMeet;
	m_annotations.clear();
	m_isDone=false;
	m_requestedBasicInfo=false;
	m_destination=m_parameters->_vertexRank(&m_vertex);
}
