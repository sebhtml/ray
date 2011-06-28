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

#include <assembler/ReadFetcher.h>
#include <assert.h>
#include <iostream>
using namespace std;

void ReadFetcher::constructor(Kmer*vertex,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,
VirtualCommunicator*vc,uint64_t workerId){
	m_workerId=workerId;
	m_virtualCommunicator=vc;
	m_parameters=parameters;
	m_outboxAllocator=outboxAllocator;
	m_outbox=outbox;
	m_inbox=inbox;
	m_vertex=*vertex;
	m_readsRequested=false;
	m_reads.clear();
	m_done=false;
	m_pointer=NULL;
}

bool ReadFetcher::isDone(){
	return m_done;
}

void ReadFetcher::work(){
	if(m_done){
		return;
	}
	if(!m_readsRequested){
		uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int bufferPosition=0;
		m_vertex.pack(message2,&bufferPosition);
		message2[bufferPosition++]=(uint64_t)m_pointer;
		int destination=m_parameters->_vertexRank(&m_vertex);
		Message aMessage(message2,bufferPosition,destination,RAY_MPI_TAG_REQUEST_VERTEX_READS,m_parameters->getRank());
		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_readsRequested=true;
	}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<uint64_t> buffer=m_virtualCommunicator->getMessageResponseElements(m_workerId);
		#ifdef ASSERT
		assert((int)buffer.size()==5);
		#endif
		m_pointer=(void*)buffer[0];
		int rank=buffer[1];
		if(rank!=INVALID_RANK){
			#ifdef ASSERT
			if(!(rank>=0&&rank<m_parameters->getSize())){
				cout<<"Error rank="<<rank<<endl;
				cout<<"Buffer: ";
				for(int i=0;i<5;i++){
					cout<<buffer[i]<<" ";
				}
				cout<<endl;
			}
			assert(rank>=0&&rank<m_parameters->getSize());
			#endif

			ReadAnnotation readAnnotation;
			readAnnotation.constructor(rank,buffer[2],buffer[3],buffer[4],false);
			m_reads.push_back(readAnnotation);
		}
		if(m_pointer==NULL){
			m_done=true;
		}else{

			uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			int bufferPosition=0;
			m_vertex.pack(message2,&bufferPosition);
			message2[bufferPosition++]=(uint64_t)m_pointer;
			int destination=m_parameters->_vertexRank(&m_vertex);
			Message aMessage(message2,bufferPosition,destination,RAY_MPI_TAG_REQUEST_VERTEX_READS,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		}
	}
}

vector<ReadAnnotation>*ReadFetcher::getResult(){
	return &m_reads;
}


