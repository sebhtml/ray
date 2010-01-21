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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include"common_functions.h"
#include"Machine.h"
#include<iostream>
#include<fstream>
#include"CoverageDistribution.h"
#include<string.h>
#include"SplayTreeIterator.h"
#include<mpi.h>
#include"Read.h"
#include"Loader.h"
#include"MyAllocator.h"
using namespace std;

Machine::Machine(int argc,char**argv){
	m_wordSize=21;
	m_last_value=0;
	m_mode_send_ingoing_edges=false;
	m_mode_send_edge_sequence_id_position=0;
	m_mode_send_vertices=false;
	m_mode_sendDistribution=false;
	m_mode_send_outgoing_edges=false;
	m_mode_send_vertices_sequence_id=0;
	m_mode_send_vertices_sequence_id_position=0;

	m_numberOfMachinesDoneSendingVertices=0;
	m_numberOfMachinesDoneSendingEdges=0;
	m_numberOfMachinesReadyToSendDistribution=0;
	m_numberOfMachinesDoneSendingCoverage=0;

	m_messageSentForVerticesDistribution=false;
	MASTER_RANK=0;
	m_sequence_ready_machines=0;

	m_TAG_WELCOME=0;
	m_TAG_SEND_SEQUENCE=1;
	m_TAG_SEQUENCES_READY=2;
	m_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS=3;
	m_TAG_VERTICES_DATA=4;
	m_TAG_VERTICES_DISTRIBUTED=5;
	m_TAG_VERTEX_PTR_REQUEST=6;
	m_TAG_OUT_EDGE_DATA_WITH_PTR=7;
	m_TAG_OUT_EDGES_DATA=8;
	m_TAG_SHOW_VERTICES=9;
	m_TAG_START_VERTICES_DISTRIBUTION=10;
	m_TAG_EDGES_DISTRIBUTED=11;
	m_TAG_IN_EDGES_DATA=12;
	m_TAG_IN_EDGE_DATA_WITH_PTR=13;
	m_TAG_START_EDGES_DISTRIBUTION=14;
	m_TAG_START_EDGES_DISTRIBUTION_ASK=15;
	m_TAG_START_EDGES_DISTRIBUTION_ANSWER=16;
	m_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION=17;
	m_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER=18;
	m_TAG_PREPARE_COVERAGE_DISTRIBUTION=19;
	m_TAG_COVERAGE_DATA=20;

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&m_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&m_size);
	MPI_Get_processor_name (m_name, &m_nameLen); 
	cout<<"Rank="<<m_rank<<" processor "<<m_name<<endl;
	m_alive=true;
	m_welcomeStep=true;
	m_loadSequenceStep=false;
	m_inputFile=argv[1];
	m_vertices_sent=0;
	m_totalLetters=0;
	m_distribution_file_id=m_distribution_sequence_id=m_distribution_currentSequenceId=0;
	if(argc!=2){
		if(isMaster()){
			cout<<"You must provide a input file."<<endl;
		}
		return ;
	}
	run();
	MPI_Finalize();
}

void Machine::run(){
	while(isAlive()){
		receiveMessages();
		checkRequests();
		processData();
	}
}

void Machine::checkRequests(){
	int minQueries=0;
	while((int)m_pendingMpiRequest.size()>minQueries){
		vector<MPI_Request> toRecheck;
		for(int i=0;i<(int)m_pendingMpiRequest.size();i++){
			MPI_Status status;
			int flag;
			MPI_Request request=m_pendingMpiRequest[i];
			MPI_Test(&request,&flag,&status);
			if(!flag){
				toRecheck.push_back(request);
			}
		}
		m_pendingMpiRequest=toRecheck;
	}
	if(m_messageMyAllocator.getNumberOfChunks()>1){
		m_messageMyAllocator.clear();
		m_messageMyAllocator.constructor();
	}
}

void Machine::receiveMessages(){
	int numberOfMessages=0;
	int flag;
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	while(flag){
		numberOfMessages++;
		int tag=status.MPI_TAG;
		int source=status.MPI_SOURCE;
		// receive vertices and store them
		if(tag==m_TAG_VERTICES_DATA){
			int length;
			MPI_Get_count(&status,MPI_UNSIGNED_LONG_LONG,&length);
			uint64_t incoming[1000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_UNSIGNED_LONG_LONG,source,tag,MPI_COMM_WORLD,&status2);
		
			for(int i=0;i<length;i++){
				uint64_t l=incoming[i];
				if(m_last_value!=m_subgraph.size() and m_subgraph.size()%100000==0){
					m_last_value=m_subgraph.size();
					cout<<"Rank "<<getRank()<<" has "<<m_subgraph.size()<<" vertices "<<endl;
				}
				SplayNode<uint64_t,Vertex>*tmp=m_subgraph.insert(l);
			
				if(m_subgraph.inserted()){
					tmp->getValue()->constructor(); 
				}
				tmp->getValue()->setCoverage(tmp->getValue()->getCoverage()+1);
			}
		// receive coverage data, and merge it
		}else if(tag==m_TAG_COVERAGE_DATA){
			int length;
			MPI_Get_count(&status,MPI_UNSIGNED_LONG_LONG,&length);
			uint64_t*incoming=(uint64_t*)m_messageMyAllocator.allocate(length*sizeof(uint64_t));
			MPI_Status status2;
			MPI_Recv(incoming,length,MPI_UNSIGNED_LONG_LONG,source,tag,MPI_COMM_WORLD,&status2);
			for(int i=0;i<length;i+=2){
				int coverage=incoming[i+0];
				uint64_t count=incoming[i+1];
				m_coverageDistribution[coverage]+=count;
			}
			m_numberOfMachinesDoneSendingCoverage++;
		// receive an outgoing edge in respect to prefix, along with the pointer for suffix
		}else if(tag==m_TAG_OUT_EDGE_DATA_WITH_PTR){
			int length;
			MPI_Get_count(&status,MPI_UNSIGNED_LONG_LONG,&length);
			uint64_t incoming[1000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_UNSIGNED_LONG_LONG,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			void*ptr=(void*)incoming[2];
			uint64_t prefix=incoming[0];
			
			int rank=vertexRank(incoming[1]);
			
			Vertex*vertex=m_subgraph.find(prefix)->getValue();
			vertex->addOutgoingEdge(rank,ptr,&m_persistentAllocator);
		
		// receive an ingoing edge in respect to prefix, along with the pointer for suffix
		}else if(tag==m_TAG_IN_EDGE_DATA_WITH_PTR){
			int length;
			MPI_Get_count(&status,MPI_UNSIGNED_LONG_LONG,&length);
			uint64_t incoming[1000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_UNSIGNED_LONG_LONG,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			uint64_t prefix=incoming[0];
			uint64_t suffix=incoming[1];
			void*ptr=(void*)incoming[2];
			int rank=vertexRank(prefix);

			Vertex*vertex=m_subgraph.find(suffix)->getValue();
			vertex->addIngoingEdge(rank,ptr,&m_persistentAllocator);
	
		// receive a out edge, send it back with the pointer
		}else if(tag==m_TAG_OUT_EDGES_DATA){
			int length;
			MPI_Get_count(&status,MPI_UNSIGNED_LONG_LONG,&length);
			uint64_t incoming[1000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_UNSIGNED_LONG_LONG,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			

			for(int i=0;i<(int)length;i+=2){
				int currentLength=3;
				uint64_t*sendBuffer=(uint64_t*)m_messageMyAllocator.allocate(currentLength*sizeof(uint64_t));
				sendBuffer[0]=incoming[i+0];
				sendBuffer[1]=incoming[i+1];
				sendBuffer[2]=(uint64_t)m_subgraph.find(incoming[i+1]);
				int destination=vertexRank(incoming[i+0]);

				MPI_Request request;
				MPI_Isend(sendBuffer, currentLength, MPI_UNSIGNED_LONG_LONG,destination, m_TAG_OUT_EDGE_DATA_WITH_PTR, MPI_COMM_WORLD, &request);
				m_pendingMpiRequest.push_back(request);
			}

		// receive an ingoing edge, send it back with the pointer
		}else if(tag==m_TAG_IN_EDGES_DATA){
			int length;
			MPI_Get_count(&status,MPI_UNSIGNED_LONG_LONG,&length);
			uint64_t incoming[1000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_UNSIGNED_LONG_LONG,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);

			for(int i=0;i<(int)length;i+=2){
				int currentLength=3;
				uint64_t*sendBuffer=(uint64_t*)m_messageMyAllocator.allocate(currentLength*sizeof(uint64_t));
				sendBuffer[0]=incoming[i+0];
				sendBuffer[1]=incoming[i+1];
				sendBuffer[2]=(uint64_t)m_subgraph.find(incoming[i+0]);
				
				int destination=vertexRank(incoming[i+1]);

				MPI_Request request;
				MPI_Isend(sendBuffer, currentLength, MPI_UNSIGNED_LONG_LONG,destination, m_TAG_IN_EDGE_DATA_WITH_PTR, MPI_COMM_WORLD, &request);
				m_pendingMpiRequest.push_back(request);
			}
		}else if(tag==m_TAG_WELCOME){
			receiveWelcomeMessage(&status);
		}else if(tag==m_TAG_SEND_SEQUENCE){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			incoming[length]='\0';
			Read*myRead=(Read*)m_persistentAllocator.allocate(sizeof(Read));
			myRead->copy(NULL,incoming,&m_persistentAllocator);
			m_myReads.push_back(myRead);
			if(m_myReads.size()%10000==0){
				cout<<"Rank "<<getRank()<<" has "<<m_myReads.size()<<" sequences"<<endl;
			}
		}else if(tag==m_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			cout<<"Rank "<<getRank()<<" has "<<m_myReads.size()<<" sequences"<<endl;
			
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, status.MPI_SOURCE, m_TAG_SEQUENCES_READY, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}else if(tag==m_TAG_SHOW_VERTICES){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			cout<<"Rank "<<getRank()<<" has "<<m_subgraph.size()<<" vertices (DONE)"<<endl;
		}else if(tag==m_TAG_SEQUENCES_READY){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			m_sequence_ready_machines++;
		}else if(tag==m_TAG_START_EDGES_DISTRIBUTION_ANSWER){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status2);
			m_numberOfMachinesReadyForEdgesDistribution++;
		}else if(tag==m_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status2);
			m_numberOfMachinesReadyToSendDistribution++;
		}else if(tag==m_TAG_PREPARE_COVERAGE_DISTRIBUTION){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status2);
			m_mode_sendDistribution=true;
		}else if(tag==m_TAG_START_EDGES_DISTRIBUTION_ASK){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, source, m_TAG_START_EDGES_DISTRIBUTION_ANSWER, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}else if(tag==m_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,source,tag,MPI_COMM_WORLD,&status2);
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, source, m_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}else if(tag==m_TAG_START_EDGES_DISTRIBUTION){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			m_mode_send_outgoing_edges=true;
			m_mode_send_edge_sequence_id=0;
		}else if(tag==m_TAG_START_VERTICES_DISTRIBUTION){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			m_mode_send_vertices=true;
			m_mode_send_vertices_sequence_id=0;
		}else if(tag==m_TAG_VERTICES_DISTRIBUTED){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			m_numberOfMachinesDoneSendingVertices++;
		}else if(tag==m_TAG_EDGES_DISTRIBUTED){
			int length;
			MPI_Get_count(&status,MPI_BYTE,&length);
			char incoming[2000];
			MPI_Status status2;
			MPI_Recv(&incoming,length,MPI_BYTE,status.MPI_SOURCE,status.MPI_TAG,MPI_COMM_WORLD,&status2);
			m_numberOfMachinesDoneSendingEdges++;
		}else{
			cout<<"Unknown tag"<<endl;

		}

		MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	}
}

void Machine::receiveWelcomeMessage(MPI_Status*status){
	char incoming[255];
	int length;
	MPI_Get_count(status,MPI_BYTE,&length);
	MPI_Status status2;
	MPI_Recv(&incoming,length,MPI_BYTE,status->MPI_SOURCE,status->MPI_TAG,MPI_COMM_WORLD,&status2);
	incoming[length]='\0';
}

int Machine::getRank(){
	return m_rank;
}

void Machine::loadSequences(){
	vector<string> allFiles=m_parameters.getAllFiles();
	if(m_distribution_reads.size()>0 and m_distribution_sequence_id>(int)m_distribution_reads.size()-1){
		m_distribution_file_id++;
		m_distribution_sequence_id=0;
		m_distribution_reads.clear();
	}
	if(m_distribution_file_id>(int)allFiles.size()-1){
		m_loadSequenceStep=true;
		for(int i=0;i<getSize();i++){
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, i, m_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}
		m_distribution_reads.clear();
		m_distributionAllocator.clear();
		return;
	}
	if(m_distribution_reads.size()==0){
		Loader loader;
		m_distribution_reads.clear();
		m_distributionAllocator.clear();
		m_distributionAllocator.constructor();
		m_distributionAllocator.clear();
		m_distributionAllocator.constructor();
		loader.load(allFiles[m_distribution_file_id],&m_distribution_reads,&m_distributionAllocator,&m_distributionAllocator);
		cout<<m_distribution_reads.size()<<" sequences to distribute"<<endl;
	}
	for(int i=0;i<1*getSize();i++){
		if(m_distribution_sequence_id>(int)m_distribution_reads.size()-1)
			break;

		int destination=m_distribution_currentSequenceId%getSize();

		if(destination<0 or destination>getSize()-1){
			cout<<destination<<" is bad"<<endl;
		}
		char*sequence=m_distribution_reads[m_distribution_sequence_id]->getSeq();
		if(false and destination==MASTER_RANK){

		}else{
			MPI_Request request;
			MPI_Isend(sequence, strlen(sequence), MPI_BYTE, destination, m_TAG_SEND_SEQUENCE, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}
		m_distribution_currentSequenceId++;
		m_distribution_sequence_id++;
	}
}

void Machine::processData(){
	if(m_pendingMpiRequest.size()>0)
		return;

	if(!m_parameters.isInitiated()&&isMaster()){
		m_parameters.load(m_inputFile);
	}else if(m_welcomeStep==true && m_loadSequenceStep==false&&isMaster()){
		loadSequences();
	}else if(m_loadSequenceStep==true && m_mode_send_vertices==false&&isMaster() and m_sequence_ready_machines==getSize()&&m_messageSentForVerticesDistribution==false){
		cout<<"Rank "<<getRank()<<" tells others to start vertices distribution"<<endl;
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			MPI_Request request;
			MPI_Isend(sequence, strlen(sequence), MPI_BYTE,i, m_TAG_START_VERTICES_DISTRIBUTION, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}
		m_messageSentForVerticesDistribution=true;
	}else if(m_numberOfMachinesDoneSendingVertices==getSize()){
		m_numberOfMachinesReadyForEdgesDistribution=0;
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			MPI_Request request;
			MPI_Isend(sequence, strlen(sequence), MPI_BYTE,i, m_TAG_START_EDGES_DISTRIBUTION_ASK, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}
		m_numberOfMachinesDoneSendingVertices=-1;
	}else if(m_numberOfMachinesReadyForEdgesDistribution==getSize()){
		cout<<"Rank "<<getRank()<<" tells others to start edges distribution"<<endl;
		m_numberOfMachinesReadyForEdgesDistribution=-1;
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			MPI_Request request;
			MPI_Isend(sequence, strlen(sequence), MPI_BYTE,i, m_TAG_START_EDGES_DISTRIBUTION, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}
	}else if(m_numberOfMachinesDoneSendingCoverage==getSize()){
		m_numberOfMachinesDoneSendingCoverage=-1;
		CoverageDistribution distribution(m_coverageDistribution,m_parameters.getDirectory());
		m_minimumCoverage=distribution.getMinimumCoverage();
		m_peakCoverage=distribution.getPeakCoverage();
		m_seedCoverage=(m_minimumCoverage+m_peakCoverage)/2;
		cout<<"MinimumCoverage = "<<m_minimumCoverage<<endl;
		cout<<"PeakCoverage = "<<m_peakCoverage<<endl;
	}

	if(m_mode_send_vertices==true){
		if(m_mode_send_vertices_sequence_id%100000==0 and m_mode_send_vertices_sequence_id_position==0){
			cout<<"Rank "<<getRank()<<" is extracting vertices from sequences "<<m_mode_send_vertices_sequence_id<<"/"<<m_myReads.size()<<endl;
		}

		if(m_mode_send_vertices_sequence_id>(int)m_myReads.size()-1){
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, MASTER_RANK, m_TAG_VERTICES_DISTRIBUTED, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
			m_mode_send_vertices=false;
			cout<<"Rank "<<getRank()<<" is extracting vertices from sequences "<<m_mode_send_vertices_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
		}else{
			char*readSequence=m_myReads[m_mode_send_vertices_sequence_id]->getSeq();
			int len=strlen(readSequence);
			char memory[100];
			int lll=len-m_wordSize;
	
			map<int,vector<uint64_t> > messagesStock;
			for(int p=m_mode_send_vertices_sequence_id_position;p<=m_mode_send_vertices_sequence_id_position;p++){
				memcpy(memory,readSequence+p,m_wordSize);
				memory[m_wordSize]='\0';
				if(isValidDNA(memory)){
					VertexMer a=wordId(memory);
					VertexMer b=complementVertex(a,m_wordSize);
					messagesStock[vertexRank(a)].push_back(a);
					messagesStock[vertexRank(b)].push_back(b);
				}
			}
			m_mode_send_vertices_sequence_id_position++;
			// send messages
			for(map<int,vector<uint64_t> >::iterator i=messagesStock.begin();i!=messagesStock.end();i++){
				int destination=i->first;
				int length=i->second.size();
				uint64_t *data=(uint64_t*)m_messageMyAllocator.allocate(sizeof(uint64_t)*length);
				for(int j=0;j<(int)i->second.size();j++)
					data[j]=i->second[j];
				MPI_Request request;
			
				MPI_Isend(data, length, MPI_UNSIGNED_LONG_LONG,destination, m_TAG_VERTICES_DATA, MPI_COMM_WORLD, &request);
				m_pendingMpiRequest.push_back(request);
				m_vertices_sent+=length;
			}

			if(m_mode_send_vertices_sequence_id_position>lll){
				m_mode_send_vertices_sequence_id++;
				m_mode_send_vertices_sequence_id_position=0;
			}
		}


	}else if(m_numberOfMachinesDoneSendingVertices==getSize()){
		m_numberOfMachinesDoneSendingVertices=0;
		for(int i=0;i<getSize();i++){
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, i, m_TAG_SHOW_VERTICES, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}
		
	}else if(m_numberOfMachinesDoneSendingEdges==getSize()){
		m_numberOfMachinesDoneSendingEdges=-1;
		cout<<"Rank "<<getRank()<<" says that edges are distributed."<<endl;
		for(int i=0;i<getSize();i++){
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, i, m_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}
	}else if(m_numberOfMachinesReadyToSendDistribution==getSize()){
		m_numberOfMachinesReadyToSendDistribution=-1;
		for(int i=0;i<getSize();i++){
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, i, m_TAG_PREPARE_COVERAGE_DISTRIBUTION, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
		}
	}

	if(m_mode_sendDistribution){
		m_mode_sendDistribution=false;
		map<int,uint64_t> distributionOfCoverage;

		SplayTreeIterator<uint64_t,Vertex> iterator(&m_subgraph);
		while(iterator.hasNext()){
			int coverage=iterator.next()->getValue()->getCoverage();
			distributionOfCoverage[coverage]++;
		}
		int length=2*distributionOfCoverage.size();
		uint64_t*data=(uint64_t*)m_messageMyAllocator.allocate(sizeof(uint64_t)*length);
		int j=0;
		for(map<int,uint64_t>::iterator i=distributionOfCoverage.begin();i!=distributionOfCoverage.end();i++){
			int coverage=i->first;
			uint64_t count=i->second;
			data[j+0]=coverage;
			data[j+1]=count;
			j+=2;
		}
		
		MPI_Request request;
		MPI_Isend(data,length, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, m_TAG_COVERAGE_DATA, MPI_COMM_WORLD, &request);
		m_pendingMpiRequest.push_back(request);
	}else if(m_mode_send_outgoing_edges==true){ 

		if(m_mode_send_edge_sequence_id%10000==0 and m_mode_send_edge_sequence_id_position==0){
			cout<<"Rank "<<getRank()<<" is extracting outgoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<endl;
		}

		if(m_mode_send_edge_sequence_id>(int)m_myReads.size()-1){
			m_mode_send_outgoing_edges=false;
			m_mode_send_ingoing_edges=true;
			m_mode_send_edge_sequence_id=0;
			m_mode_send_edge_sequence_id_position=0;
			cout<<"Rank "<<getRank()<<" is extracting outgoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
		}else{


			char*readSequence=m_myReads[m_mode_send_edge_sequence_id]->getSeq();
			int len=strlen(readSequence);
			char memory[100];
			int lll=len-m_wordSize-1;
			map<int,vector<uint64_t> > messagesStockOut;
			for(int p=m_mode_send_edge_sequence_id_position;p<=m_mode_send_edge_sequence_id_position;p++){
				memcpy(memory,readSequence+p,m_wordSize+1);
				memory[m_wordSize+1]='\0';
				if(isValidDNA(memory)){
					VertexMer a=wordId(memory);
					VertexMer b=complementVertex(a,m_wordSize+1);
					uint64_t a_1=getKPrefix(a,m_wordSize);
					uint64_t a_2=getKSuffix(a,m_wordSize);
					uint64_t b_1=getKPrefix(b,m_wordSize);
					uint64_t b_2=getKSuffix(b,m_wordSize);
					
					int rankA=vertexRank(a_2);
					int rankB=vertexRank(b_2);
					messagesStockOut[rankA].push_back(a_1);
					messagesStockOut[rankA].push_back(a_2);
					messagesStockOut[rankB].push_back(b_1);
					messagesStockOut[rankB].push_back(b_2);
				}
			}

			m_mode_send_edge_sequence_id_position++;

			for(map<int,vector<uint64_t> >::iterator i=messagesStockOut.begin();i!=messagesStockOut.end();i++){
				int destination=i->first;
				int length=i->second.size();
				uint64_t*data=(uint64_t*)m_messageMyAllocator.allocate(sizeof(uint64_t)*(length));
				for(int j=0;j<(int)i->second.size();j++){
					data[j]=i->second[j];
				}
	
				MPI_Request request;
				MPI_Isend(data, length, MPI_UNSIGNED_LONG_LONG,destination, m_TAG_OUT_EDGES_DATA, MPI_COMM_WORLD, &request);
				m_pendingMpiRequest.push_back(request);
			}


			if(m_mode_send_edge_sequence_id_position>lll){
				m_mode_send_edge_sequence_id++;
				m_mode_send_edge_sequence_id_position=0;
			}
		}
	}else if(m_mode_send_ingoing_edges==true){ 

		if(m_mode_send_edge_sequence_id%10000==0 and m_mode_send_edge_sequence_id_position==0){
			cout<<"Rank "<<getRank()<<" is extracting ingoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<endl;
		}

		if(m_mode_send_edge_sequence_id>(int)m_myReads.size()-1){
			char*message=m_name;
			MPI_Request request;
			MPI_Isend(message, strlen(message), MPI_BYTE, MASTER_RANK, m_TAG_EDGES_DISTRIBUTED, MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
			m_mode_send_ingoing_edges=false;
			cout<<"Rank "<<getRank()<<" is extracting ingoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
		}else{


			char*readSequence=m_myReads[m_mode_send_edge_sequence_id]->getSeq();
			int len=strlen(readSequence);
			char memory[100];
			int lll=len-m_wordSize-1;
			map<int,vector<uint64_t> > messagesStockIn;
			for(int p=m_mode_send_edge_sequence_id_position;p<=m_mode_send_edge_sequence_id_position;p++){
				memcpy(memory,readSequence+p,m_wordSize+1);
				memory[m_wordSize+1]='\0';
				if(isValidDNA(memory)){
					VertexMer a=wordId(memory);
					VertexMer b=complementVertex(a,m_wordSize+1);
					uint64_t a_1=getKPrefix(a,m_wordSize);
					uint64_t a_2=getKSuffix(a,m_wordSize);
					uint64_t b_1=getKPrefix(b,m_wordSize);
					uint64_t b_2=getKSuffix(b,m_wordSize);
					
					int rankA=vertexRank(a_1);
					int rankB=vertexRank(b_1);
					messagesStockIn[rankA].push_back(a_1);
					messagesStockIn[rankA].push_back(a_2);
					messagesStockIn[rankB].push_back(b_1);
					messagesStockIn[rankB].push_back(b_2);
				}
			}

			m_mode_send_edge_sequence_id_position++;

			// send messages
			for(map<int,vector<uint64_t> >::iterator i=messagesStockIn.begin();i!=messagesStockIn.end();i++){
				int destination=i->first;
				int length=i->second.size();
				uint64_t*data=(uint64_t*)m_messageMyAllocator.allocate(sizeof(uint64_t)*(length));
				for(int j=0;j<(int)i->second.size();j++){
					data[j]=i->second[j];
				}
	
				MPI_Request request;
				MPI_Isend(data, length, MPI_UNSIGNED_LONG_LONG,destination, m_TAG_IN_EDGES_DATA, MPI_COMM_WORLD, &request);
				m_pendingMpiRequest.push_back(request);
			}

			if(m_mode_send_edge_sequence_id_position>lll){
				m_mode_send_edge_sequence_id++;
				m_mode_send_edge_sequence_id_position=0;
			}
		}
	}
}

bool Machine::isMaster(){
	return getRank()==MASTER_RANK;
}



int Machine::getSize(){
	return m_size;
}



bool Machine::isAlive(){
	return m_alive;
}

void Machine::printStatus(){
	cout<<"********"<<endl;
	cout<<"Rank: "<<getRank()<<endl;
	cout<<"Reads: "<<m_myReads.size()<<endl;
	m_persistentAllocator.print();

	cout<<"For distribution: "<<endl;
	m_distributionAllocator.print();
	cout<<"Reads: "<<m_distribution_reads.size()<<endl;
}

int Machine::vertexRank(uint64_t a){
	return hash_uint64_t(a)%getSize();
}
