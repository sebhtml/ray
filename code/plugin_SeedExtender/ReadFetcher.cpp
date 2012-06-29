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

#include <plugin_SeedExtender/ReadFetcher.h>
#include <core/OperatingSystem.h>

//#define GUILLIMIN_BUG

#include <assert.h>
#include <iostream>
using namespace std;

void ReadFetcher::constructor(Kmer*vertex,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,
VirtualCommunicator*vc,WorkerHandle workerId,
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS
){
	this->RAY_MPI_TAG_REQUEST_VERTEX_READS=RAY_MPI_TAG_REQUEST_VERTEX_READS;
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
		MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int bufferPosition=0;
		m_vertex.pack(message2,&bufferPosition);

		MessageUnit integerValue=pack_pointer((void**)&m_pointer);

		// fancy trick to transmit a void* over the network
		message2[bufferPosition++]=integerValue;

		int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_READS);

		// do some padding
		while(bufferPosition<period){
			message2[bufferPosition++]=0;
		}

		#ifdef ASSERT
		int start=KMER_U64_ARRAY_SIZE+1;
	
		assert(period>=5);

		// Padding must be 0.
		while(start<period){
			if(message2[start]!=0){
				cout<<"Error, padding must be 0."<<endl;
			}
			assert(message2[start]==0);
			start++;
		}

		#endif
		
		int destination=m_parameters->_vertexRank(&m_vertex);

		#ifdef GUILLIMIN_BUG
		if(m_parameters->getRank()==destination){
			cout<<endl;
			cout<<"worker: "<<m_workerId<<endl;
			cout<<"Sending9 RAY_MPI_TAG_REQUEST_VERTEX_READS ptr="<<m_pointer<<" ";
			cout<<" integerValue= "<<integerValue<<" to "<<destination<<endl;
			for(int i=0;i<period;i++)
				cout<<" "<<i<<" -> "<<message2[i];
			cout<<endl;
		}
		#endif

		Message aMessage(message2,period,destination,RAY_MPI_TAG_REQUEST_VERTEX_READS,
			m_parameters->getRank());

		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		m_readsRequested=true;

	}else if(m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<MessageUnit> buffer;
		m_virtualCommunicator->getMessageResponseElements(m_workerId,&buffer);

		#ifdef ASSERT
		int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_READS);
		assert((int)buffer.size()==period);
		#endif

		//. ------
		
		#ifdef GUILLIMIN_BUG
		int destination=m_parameters->_vertexRank(&m_vertex);
		if(m_parameters->getRank()==destination){
			cout<<endl;
			cout<<"worker: "<<m_workerId<<endl;
			cout<<"Receiving RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY ptr="<<m_pointer<<" from "<<endl;
			for(int i=0;i<period;i++)
				cout<<" "<<i<<" -> "<<buffer[i];
			cout<<endl;
		}
		#endif

		// fancy trick to transmit a void* over the network
		unpack_pointer((void**)&m_pointer,buffer[0]);

		int rank=buffer[1];

		if(rank!=INVALID_RANK){

			#ifdef ASSERT
			if(!(rank>=0&&rank<m_parameters->getSize())){
				cout<<"Error rank="<<rank<<endl;
				cout<<"Buffer: ";
				for(int i=0;i<period;i++){
					cout<<buffer[i]<<" ";
				}
				cout<<endl;
			}
			assert(rank>=0&&rank<m_parameters->getSize());
			#endif

			int readIndex=buffer[2];
			int position=buffer[3];
			char strand=(char)buffer[4];

			#ifdef ASSERT
			int destination=m_parameters->_vertexRank(&m_vertex);

			assert(readIndex>=0);
			assert(position>=0);

			if(!(strand=='F'||strand=='R')){
				cout<<"Error, invalid strand from "<<destination<<" strand= "<<strand<<" or ";
				cout<<buffer[4]<<endl;
			}
			assert(strand=='F'||strand=='R');
			#endif

			ReadAnnotation readAnnotation;
			readAnnotation.constructor(rank,readIndex,position,strand,false);
			m_reads.push_back(readAnnotation);
		}

		if(m_pointer==NULL){
			m_done=true;
		}else{

			MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
			int bufferPosition=0;
			m_vertex.pack(message2,&bufferPosition);

			MessageUnit integerValue=pack_pointer((void**)&m_pointer);
			message2[bufferPosition++]=integerValue;

			int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_READS);

			// do some padding
			while(bufferPosition<period){
				message2[bufferPosition++]=0;
			}


			int destination=m_parameters->_vertexRank(&m_vertex);
			Message aMessage(message2,period,destination,RAY_MPI_TAG_REQUEST_VERTEX_READS,m_parameters->getRank());

			#ifdef GUILLIMIN_BUG
			if(m_parameters->getRank()==destination){
				cout<<endl;
				cout<<"worker: "<<m_workerId<<endl;
				cout<<"Sending11 RAY_MPI_TAG_REQUEST_VERTEX_READS ptr="<<m_pointer<<" ";
				cout<<" integerValue= "<<integerValue<<" to "<<destination<<endl;
				
				for(int i=0;i<period;i++)
					cout<<" "<<i<<" -> "<<message2[i];
				cout<<endl;
			}
			#endif

			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);
		}
	}
}

vector<ReadAnnotation>*ReadFetcher::getResult(){
	return &m_reads;
}


