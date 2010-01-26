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

#include"Machine.h"
#include"Message.h"
#include"common_functions.h"
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
	m_sentMessages=0;
	m_ticks=0;
	m_numberOfStops=0;
	m_receivedMessages=0;
	m_canUseBarrier=false;
	m_wordSize=21;
	m_reverseComplementVertex=false;
	m_last_value=0;
	m_mode_send_ingoing_edges=false;
	m_mode_send_edge_sequence_id_position=0;
	m_mode_send_vertices=false;
	m_mode_sendDistribution=false;
	m_mode_send_outgoing_edges=false;
	m_mode_send_vertices_sequence_id=0;
	m_mode_send_vertices_sequence_id_position=0;
	m_reverseComplementEdge=false;

	m_numberOfMachinesDoneSendingVertices=0;
	m_numberOfMachinesDoneSendingEdges=0;
	m_numberOfMachinesReadyToSendDistribution=0;
	m_numberOfMachinesDoneSendingCoverage=0;
	m_machineRank=0;
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
	m_TAG_COVERAGE_END=21;
	m_TAG_BARRIER_STOP=22;
	m_TAG_BARRIER_START=23;
	

	m_inBarrier=false;

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&m_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&m_size);
	MPI_Get_processor_name (m_name, &m_nameLen); 
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
	cout<<"Rank "<<getRank()<<" is "<<m_name<<endl;
	while(isAlive()){
		receiveMessages(); 

		checkRequests();
		processMessages();

		if(m_ticks%1000){
			MPI_Barrier(MPI_COMM_WORLD);
		}
		if(m_inBarrier==true){
			//cout<<"Rank "<<getRank()<<" is paused."<<endl;
			continue;
		}

		processData();
		sendMessages();

		m_ticks++;
	}
}

void Machine::sendMessages(){
	for(int i=0;i<(int)m_outbox.size();i++){
		Message*aMessage=&(m_outbox[i]);
		int tag=aMessage->getTag();
		if(tag==m_TAG_BARRIER_START){
			//cout<<"Tag= m_TAG_BARRIER_START"<<endl;
		}
/*
		if(aMessage->getDestination()==getRank()){
			int sizeOfElements=8;
			if(aMessage->getTag()==m_TAG_SEND_SEQUENCE){
				sizeOfElements=1;
			}
			void*newBuffer=m_inboxAllocator.allocate(sizeOfElements*aMessage->getCount());
			memcpy(newBuffer,aMessage->getBuffer(),sizeOfElements*aMessage->getCount());
			aMessage->setBuffer(newBuffer);
			m_inbox.push_back(*aMessage);
			m_receivedMessages++;
		}else{
*/
			MPI_Request request;
			MPI_Isend(aMessage->getBuffer(), aMessage->getCount(), aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(), MPI_COMM_WORLD, &request);
			m_pendingMpiRequest.push_back(request);
			m_requestIterations.push_back(0);
		//}
		m_sentMessages++;
	}
	m_outbox.clear();
	if(m_outboxAllocator.getNumberOfChunks()>1){
		m_outboxAllocator.clear();
		m_outboxAllocator.constructor();
	}
}

void Machine::checkRequests(){
	MPI_Request theRequests[1024];
	MPI_Status theStatus[1024];
	
	for(int i=0;i<(int)m_pendingMpiRequest.size();i++){
		theRequests[i]=m_pendingMpiRequest[i];
	}
	MPI_Waitall(m_pendingMpiRequest.size(),theRequests,theStatus);
	m_pendingMpiRequest.clear();
}

void Machine::waitall(int c,MPI_Request*a,MPI_Status*b){
	set<int> done;
	while(1){
		if(done.size()==c)
			break;
		for(int i=0;i<c;i++){
			if(done.count(i)>0)
				continue;
			int flag;
			MPI_Test(a+i,&flag,b+i);
			if(flag)
				done.insert(i);
		}
	}
}

void Machine::receiveMessages(){
	int numberOfMessages=0;
	int flag;
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	while(flag){
		if(numberOfMessages>10)
			break;
		MPI_Datatype datatype=MPI_UNSIGNED_LONG_LONG;
		int sizeOfType=8;
		int tag=status.MPI_TAG;
		if(tag==m_TAG_SEND_SEQUENCE){
			datatype=MPI_BYTE;
			sizeOfType=1;
		}

		int source=status.MPI_SOURCE;
		int length;
		MPI_Get_count(&status,datatype,&length);
		void*incoming=(void*)m_inboxAllocator.allocate(length*sizeOfType);
		MPI_Status status2;
		MPI_Recv(incoming,length,datatype,source,tag,MPI_COMM_WORLD,&status2);
		Message aMessage(incoming,length,datatype,source,tag,source);
		m_inbox.push_back(aMessage);
		m_receivedMessages++;
		numberOfMessages++;
		MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	}

	/*
	if(numberOfMessages>10){
		cout<<"Rank "<<getRank()<<" received "<<numberOfMessages<<" messages."<<endl;
	}
	*/
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
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, i, m_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS,getRank());
			m_outbox.push_back(aMessage);
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
	for(int i=0;i<1;i++){
		if(m_distribution_sequence_id>(int)m_distribution_reads.size()-1)
			break;

		int destination=m_distribution_currentSequenceId%getSize();

		if(destination<0 or destination>getSize()-1){
			cout<<destination<<" is bad"<<endl;
		}
		char*sequence=m_distribution_reads[m_distribution_sequence_id]->getSeq();
		if(false and destination==MASTER_RANK){

		}else{
			Message aMessage(sequence, strlen(sequence), MPI_BYTE, destination, m_TAG_SEND_SEQUENCE,getRank());
			m_outbox.push_back(aMessage);
		}
		m_distribution_currentSequenceId++;
		m_distribution_sequence_id++;
	}
}

void Machine::processMessage(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	int tag=message->getTag();
	int source=message->getSource();


	if(tag==m_TAG_VERTICES_DATA){
		int length=count;
		uint64_t*incoming=(uint64_t*)buffer;
		//cout<<"Length="<<length<<endl;
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
	}else if(tag==m_TAG_BARRIER_STOP){
		m_numberOfStops++;
		if(m_numberOfStops==getSize()){
			//cout<<"m_numberOfStops="<<m_numberOfStops<<endl;
			m_inBarrier=false;
			m_ticks++;
			m_numberOfStops=0;
			for(int i=0;i<getSize();i++){
				void*sendBuffer=m_name;
				Message aMessage(sendBuffer,0,MPI_UNSIGNED_LONG_LONG,i,m_TAG_BARRIER_START,getRank());
				//cout<<"Rank "<<getRank()<<" tells "<<i<<" to start."<<endl;
				m_outbox.push_back(aMessage);
			}
		}
	}else if(tag==m_TAG_BARRIER_START){
		m_inBarrier=false;
		//cout<<"Rank "<<getRank()<<" exiting barrier."<<endl;
	}else if(tag==m_TAG_COVERAGE_DATA){
		int length=count;
		uint64_t*incoming=(uint64_t*)buffer;

		for(int i=0;i<length;i+=2){
			int coverage=incoming[i+0];
			uint64_t count=incoming[i+1];
			//cout<<coverage<<" "<<count<<endl;
			m_coverageDistribution[coverage]+=count;
		}
	// receive an outgoing edge in respect to prefix, along with the pointer for suffix
	}else if(tag==m_TAG_OUT_EDGE_DATA_WITH_PTR){
		uint64_t*incoming=(uint64_t*)buffer;
		void*ptr=(void*)incoming[2];
		uint64_t prefix=incoming[0];
		uint64_t suffix=incoming[1];
		int rank=vertexRank(suffix);
		
		SplayNode<uint64_t,Vertex>*node=m_subgraph.find(prefix);
		if(node==NULL){
			cout<<"NULL "<<prefix<<" Rank="<<getRank()<<endl;
		}else{
			Vertex*vertex=node->getValue();
			vertex->addOutgoingEdge(rank,ptr,&m_persistentAllocator);
		}
	// receive an ingoing edge in respect to prefix, along with the pointer for suffix
	}else if(tag==m_TAG_IN_EDGE_DATA_WITH_PTR){
		uint64_t*incoming=(uint64_t*)buffer;
		uint64_t prefix=incoming[0];
		uint64_t suffix=incoming[1];
		void*ptr=(void*)incoming[2];
		int rank=vertexRank(prefix);


		SplayNode<uint64_t,Vertex>*node=m_subgraph.find(suffix);
		if(node==NULL){
			cout<<"NULL "<<prefix<<endl;
		}else{
			Vertex*vertex=node->getValue();
			vertex->addIngoingEdge(rank,ptr,&m_persistentAllocator);
		}
	// receive a out edge, send it back with the pointer
	}else if(tag==m_TAG_OUT_EDGES_DATA){
		int length=count;
		uint64_t*incoming=(uint64_t*)buffer;
		

		for(int i=0;i<(int)length;i+=2){
			int currentLength=3;
			uint64_t*sendBuffer=(uint64_t*)m_outboxAllocator.allocate(currentLength*sizeof(uint64_t));
			sendBuffer[0]=incoming[i+0];
			sendBuffer[1]=incoming[i+1];
			sendBuffer[2]=(uint64_t)m_subgraph.find(incoming[i+1]);
			int destination=vertexRank(incoming[i+0]);


			Message aMessage(sendBuffer,currentLength,MPI_UNSIGNED_LONG_LONG,destination,m_TAG_OUT_EDGE_DATA_WITH_PTR,getRank());
			m_outbox.push_back(aMessage);
		}

	// receive an ingoing edge, send it back with the pointer
	}else if(tag==m_TAG_IN_EDGES_DATA){
		int length=count;
		uint64_t*incoming=(uint64_t*)buffer;

		for(int i=0;i<(int)length;i+=2){
			int currentLength=3;
			uint64_t*sendBuffer=(uint64_t*)m_outboxAllocator.allocate(currentLength*sizeof(uint64_t));
			sendBuffer[0]=incoming[i+0];
			sendBuffer[1]=incoming[i+1];
			sendBuffer[2]=(uint64_t)m_subgraph.find(incoming[i+0]);
			
			int destination=vertexRank(incoming[i+1]);

			Message aMessage(sendBuffer,currentLength,MPI_UNSIGNED_LONG_LONG,destination,m_TAG_IN_EDGE_DATA_WITH_PTR,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(tag==m_TAG_WELCOME){

	}else if(tag==m_TAG_SEND_SEQUENCE){
		int length=count;
		char incoming[2000];
		for(int i=0;i<(int)length;i++)
			incoming[i]=((char*)buffer)[i];

		incoming[length]='\0';
		Read*myRead=(Read*)m_persistentAllocator.allocate(sizeof(Read));
		myRead->copy(NULL,incoming,&m_persistentAllocator);
		m_myReads.push_back(myRead);
		if(m_myReads.size()%10000==0){
			cout<<"Rank "<<getRank()<<" has "<<m_myReads.size()<<" sequences"<<endl;
		}
	}else if(tag==m_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS){
		cout<<"Rank "<<getRank()<<" has "<<m_myReads.size()<<" sequences"<<endl;
		char*message=m_name;
		Message aMessage(message,0,MPI_UNSIGNED_LONG_LONG,source,m_TAG_SEQUENCES_READY,getRank());
		//m_canUseBarrier=true;
		m_outbox.push_back(aMessage);
	}else if(tag==m_TAG_SHOW_VERTICES){
		cout<<"Rank "<<getRank()<<" has "<<m_subgraph.size()<<" vertices (DONE)"<<endl;
	}else if(tag==m_TAG_SEQUENCES_READY){
		m_sequence_ready_machines++;
	}else if(tag==m_TAG_START_EDGES_DISTRIBUTION_ANSWER){
		m_numberOfMachinesReadyForEdgesDistribution++;
	}else if(tag==m_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER){
		m_numberOfMachinesReadyToSendDistribution++;
	}else if(tag==m_TAG_PREPARE_COVERAGE_DISTRIBUTION){
		//cout<<"Rank "<<getRank()<<" prepares its distribution."<<endl;
		m_mode_send_coverage_iterator=0;
		m_mode_sendDistribution=true;
	}else if(tag==m_TAG_START_EDGES_DISTRIBUTION_ASK){
		char*message=m_name;
		Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, source, m_TAG_START_EDGES_DISTRIBUTION_ANSWER,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==m_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION){
		char*message=m_name;
		Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, source, m_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==m_TAG_START_EDGES_DISTRIBUTION){
		m_mode_send_outgoing_edges=true;
		m_mode_send_edge_sequence_id=0;
	}else if(tag==m_TAG_START_VERTICES_DISTRIBUTION){
		m_mode_send_vertices=true;
		m_mode_send_vertices_sequence_id=0;
	}else if(tag==m_TAG_VERTICES_DISTRIBUTED){
		m_numberOfMachinesDoneSendingVertices++;
	}else if(tag==m_TAG_COVERAGE_END){
		m_numberOfMachinesDoneSendingCoverage++;
		//cout<<"m_numberOfMachinesDoneSendingCoverage="<<m_numberOfMachinesDoneSendingCoverage<<" "<<source<<" is done"<<endl;
	}else if(tag==m_TAG_EDGES_DISTRIBUTED){
		m_numberOfMachinesDoneSendingEdges++;
	}else{
		cout<<"Unknown tag"<<endl;
	}
}

void Machine::processMessages(){
	for(int i=0;i<(int)m_inbox.size();i++){
		processMessage(&(m_inbox[i]));
	}
	m_inbox.clear();
	if(m_inboxAllocator.getNumberOfChunks()>1){
		m_inboxAllocator.clear();
		m_inboxAllocator.constructor();
	}
}

void Machine::processData(){

	if(m_ticks%100==0 and m_canUseBarrier){
		char*sequence=m_name;
		Message aMessage(sequence, 0, MPI_UNSIGNED_LONG_LONG,MASTER_RANK, m_TAG_BARRIER_STOP,getRank());
		m_outbox.push_back(aMessage);
		m_inBarrier=true;
	}


	if(!m_parameters.isInitiated()&&isMaster()){
		m_parameters.load(m_inputFile);
	}else if(m_welcomeStep==true && m_loadSequenceStep==false&&isMaster()){
		loadSequences();
	}else if(m_loadSequenceStep==true && m_mode_send_vertices==false&&isMaster() and m_sequence_ready_machines==getSize()&&m_messageSentForVerticesDistribution==false){
		cout<<"Rank "<<getRank()<<" tells others to start vertices distribution"<<endl;
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			Message aMessage(sequence, 0, MPI_UNSIGNED_LONG_LONG,i, m_TAG_START_VERTICES_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_messageSentForVerticesDistribution=true;
	}else if(m_numberOfMachinesDoneSendingVertices==getSize()){
		m_numberOfMachinesReadyForEdgesDistribution=0;
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			Message aMessage(sequence, 0, MPI_UNSIGNED_LONG_LONG,i, m_TAG_START_EDGES_DISTRIBUTION_ASK,getRank());
			m_outbox.push_back(aMessage);
		}
		m_numberOfMachinesDoneSendingVertices=-1;
	}else if(m_numberOfMachinesReadyForEdgesDistribution==getSize()){
		cout<<"Rank "<<getRank()<<" tells others to start edges distribution"<<endl;
		m_numberOfMachinesReadyForEdgesDistribution=-1;
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			Message aMessage(sequence, 0, MPI_UNSIGNED_LONG_LONG,i, m_TAG_START_EDGES_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_numberOfMachinesDoneSendingCoverage==getSize()){
		m_numberOfMachinesDoneSendingCoverage=-1;
		CoverageDistribution distribution(m_coverageDistribution,m_parameters.getDirectory());
		m_minimumCoverage=distribution.getMinimumCoverage();
		m_peakCoverage=distribution.getPeakCoverage();
		m_seedCoverage=(m_minimumCoverage+m_peakCoverage)/2;
		cout<<"MinimumCoverage = "<<m_minimumCoverage<<endl;
		cout<<"PeakCoverage = "<<m_peakCoverage<<endl;
		cout<<"SeedCoverage = "<<m_seedCoverage<<endl;
	}

	if(m_mode_send_vertices==true){
		if(m_mode_send_vertices_sequence_id%10000==0 and m_mode_send_vertices_sequence_id_position==0){
			string reverse="";
			if(m_reverseComplementVertex==true)
				reverse="(reverse complement) ";
			cout<<"Rank "<<getRank()<<" is extracting vertices "<<reverse<<"from sequences "<<m_mode_send_vertices_sequence_id<<"/"<<m_myReads.size()<<endl;
		}

		if(m_mode_send_vertices_sequence_id>(int)m_myReads.size()-1){
			if(m_reverseComplementVertex==false){
				cout<<"Rank "<<getRank()<<" is extracting vertices from sequences "<<m_mode_send_vertices_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
				m_mode_send_vertices_sequence_id=0;
				m_mode_send_vertices_sequence_id_position=0;
				m_reverseComplementVertex=true;
			}else{
				char*message=m_name;
				Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, m_TAG_VERTICES_DISTRIBUTED,getRank());
				m_outbox.push_back(aMessage);
				m_mode_send_vertices=false;
				cout<<"Rank "<<getRank()<<" is extracting vertices (reverse complement) from sequences "<<m_mode_send_vertices_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
			}
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
					if(m_reverseComplementVertex==false){
						messagesStock[vertexRank(a)].push_back(a);
					}else{
						VertexMer b=complementVertex(a,m_wordSize);
						messagesStock[vertexRank(b)].push_back(b);
					}
				}
			}
			m_mode_send_vertices_sequence_id_position++;
			// send messages
			for(map<int,vector<uint64_t> >::iterator i=messagesStock.begin();i!=messagesStock.end();i++){
				int destination=i->first;
				int length=i->second.size();
				uint64_t *data=(uint64_t*)m_outboxAllocator.allocate(sizeof(uint64_t)*length);
				for(int j=0;j<(int)i->second.size();j++){
					data[j]=i->second[j];
				}
				
				
				Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, m_TAG_VERTICES_DATA,getRank());
				m_outbox.push_back(aMessage);
				m_vertices_sent+=length;
			}

			if(m_mode_send_vertices_sequence_id_position>lll){
				m_mode_send_vertices_sequence_id++;
				m_mode_send_vertices_sequence_id_position=0;
			}
		}


	}else if(m_numberOfMachinesDoneSendingVertices==getSize()){
		cout<<"Now showing everyone."<<endl;
		m_numberOfMachinesDoneSendingVertices=0;
		for(int i=0;i<getSize();i++){
			char*message=m_name;
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, i, m_TAG_SHOW_VERTICES,getRank());
			m_outbox.push_back(aMessage);
		}
		
	}else if(m_numberOfMachinesDoneSendingEdges==getSize()){
		m_numberOfMachinesDoneSendingEdges=-1;
		cout<<"Rank "<<getRank()<<" says that edges are distributed."<<endl;
		for(int i=0;i<getSize();i++){
			char*message=m_name;
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, i, m_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_numberOfMachinesReadyToSendDistribution==getSize()){
		//m_numberOfMachinesReadyToSendDistribution=-1;

		if(m_machineRank<=m_numberOfMachinesDoneSendingCoverage){
			//cout<<"Rank "<<getRank()<<" tells "<<m_machineRank<<" to distribute its distribution."<<endl;
			char*message=m_name;
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, m_machineRank, m_TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
			m_machineRank++;
		}

		if(m_machineRank==getSize()){
			m_numberOfMachinesReadyToSendDistribution=-1;
		}
	}

	if(m_mode_sendDistribution){
		if(m_distributionOfCoverage.size()==0){
			SplayTreeIterator<uint64_t,Vertex> iterator(&m_subgraph);
			while(iterator.hasNext()){
				int coverage=iterator.next()->getValue()->getCoverage();
				m_distributionOfCoverage[coverage]++;
			}
		}


		int length=2;
		uint64_t*data=(uint64_t*)m_outboxAllocator.allocate(sizeof(uint64_t)*length);
		int j=0;
		for(map<int,uint64_t>::iterator i=m_distributionOfCoverage.begin();i!=m_distributionOfCoverage.end();i++){
			int coverage=i->first;
			uint64_t count=i->second;
			if(m_mode_send_coverage_iterator==j){
				data[0]=coverage;
				data[1]=count;
				break;
			}
			j++;
		}

		//cout<<"Rank "<<getRank()<<" is sending distribution to "<<MASTER_RANK<<""<<endl;
		Message aMessage(data,length, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, m_TAG_COVERAGE_DATA,getRank());
		m_outbox.push_back(aMessage);

		m_mode_send_coverage_iterator++;
		if(m_mode_send_coverage_iterator>=(int)m_distributionOfCoverage.size()){
			m_mode_sendDistribution=false;
			char*message=m_name;
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG,MASTER_RANK, m_TAG_COVERAGE_END,getRank());
			m_outbox.push_back(aMessage);
			m_distributionOfCoverage.clear();
		}

	}else if(m_mode_send_outgoing_edges==true){ 

		if(m_mode_send_edge_sequence_id%10000==0 and m_mode_send_edge_sequence_id_position==0){
			string strand="";
			if(m_reverseComplementEdge)
				strand="(reverse complement)";
			cout<<"Rank "<<getRank()<<" is extracting outgoing edges "<<strand<<" "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<endl;
		}

		if(m_mode_send_edge_sequence_id>(int)m_myReads.size()-1){
			if(m_reverseComplementEdge==false){
				m_mode_send_edge_sequence_id_position=0;
				m_reverseComplementEdge=true;
				cout<<"Rank "<<getRank()<<" is extracting outgoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
				m_mode_send_edge_sequence_id=0;
			}else{
				m_mode_send_outgoing_edges=false;
				m_mode_send_ingoing_edges=true;
				m_mode_send_edge_sequence_id_position=0;
				cout<<"Rank "<<getRank()<<" is extracting outgoing edges (reverse complement) "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
				m_mode_send_edge_sequence_id=0;
				m_reverseComplementEdge=false;
			}
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
					if(m_reverseComplementEdge){
						VertexMer b=complementVertex(a,m_wordSize+1);
						uint64_t b_1=getKPrefix(b,m_wordSize);
						uint64_t b_2=getKSuffix(b,m_wordSize);
						int rankB=vertexRank(b_2);
						messagesStockOut[rankB].push_back(b_1);
						messagesStockOut[rankB].push_back(b_2);
					}else{
						uint64_t a_1=getKPrefix(a,m_wordSize);
						uint64_t a_2=getKSuffix(a,m_wordSize);
						int rankA=vertexRank(a_2);
						messagesStockOut[rankA].push_back(a_1);
						messagesStockOut[rankA].push_back(a_2);
					}
					
				}
			}

			m_mode_send_edge_sequence_id_position++;

			for(map<int,vector<uint64_t> >::iterator i=messagesStockOut.begin();i!=messagesStockOut.end();i++){
				int destination=i->first;
				int length=i->second.size();
				uint64_t*data=(uint64_t*)m_outboxAllocator.allocate(sizeof(uint64_t)*(length));
				for(int j=0;j<(int)i->second.size();j++){
					data[j]=i->second[j];
				}
	
				Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, m_TAG_OUT_EDGES_DATA,getRank());
				m_outbox.push_back(aMessage);
			}


			if(m_mode_send_edge_sequence_id_position>lll){
				m_mode_send_edge_sequence_id++;
				m_mode_send_edge_sequence_id_position=0;
			}
		}
	}else if(m_mode_send_ingoing_edges==true){ 

		if(m_mode_send_edge_sequence_id%10000==0 and m_mode_send_edge_sequence_id_position==0){
			string strand="";
			if(m_reverseComplementEdge)
				strand="(reverse complement)";
			cout<<"Rank "<<getRank()<<" is extracting ingoing edges "<<strand<<" "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<endl;
		}

		if(m_mode_send_edge_sequence_id>(int)m_myReads.size()-1){
			if(m_reverseComplementEdge==false){
				m_reverseComplementEdge=true;
				m_mode_send_edge_sequence_id=0;
				m_mode_send_edge_sequence_id_position=0;
				cout<<"Rank "<<getRank()<<" is extracting ingoing edges "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
			}else{
				char*message=m_name;
				Message aMessage(message, strlen(message), MPI_UNSIGNED_LONG_LONG, MASTER_RANK, m_TAG_EDGES_DISTRIBUTED,getRank());
				m_outbox.push_back(aMessage);
				m_mode_send_ingoing_edges=false;
				cout<<"Rank "<<getRank()<<" is extracting ingoing edges (reverse complement) "<<m_mode_send_edge_sequence_id<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
			}
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

					if(m_reverseComplementEdge){
						VertexMer b=complementVertex(a,m_wordSize+1);
						uint64_t b_1=getKPrefix(b,m_wordSize);
						uint64_t b_2=getKSuffix(b,m_wordSize);
						int rankB=vertexRank(b_1);
						messagesStockIn[rankB].push_back(b_1);
						messagesStockIn[rankB].push_back(b_2);
					}else{
						uint64_t a_1=getKPrefix(a,m_wordSize);
						uint64_t a_2=getKSuffix(a,m_wordSize);
						int rankA=vertexRank(a_1);
						messagesStockIn[rankA].push_back(a_1);
						messagesStockIn[rankA].push_back(a_2);
					}
				}
			}

			m_mode_send_edge_sequence_id_position++;

			// send messages
			for(map<int,vector<uint64_t> >::iterator i=messagesStockIn.begin();i!=messagesStockIn.end();i++){
				int destination=i->first;
				int length=i->second.size();
				uint64_t*data=(uint64_t*)m_outboxAllocator.allocate(sizeof(uint64_t)*(length));
				for(int j=0;j<(int)i->second.size();j++){
					data[j]=i->second[j];
				}
	
				Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, m_TAG_IN_EDGES_DATA,getRank());
				m_outbox.push_back(aMessage);
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
	cout<<"Inbox: "<<m_inbox.size()<<endl;
	cout<<"Outbox: "<<m_outbox.size()<<endl;
	cout<<"ReceivedMessages="<<m_receivedMessages<<endl;
	cout<<"SentMessages="<<m_sentMessages<<endl;
}

int Machine::vertexRank(uint64_t a){
	return hash_uint64_t(a)%getSize();
}
