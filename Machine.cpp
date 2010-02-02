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
#include<sstream>
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
	m_numberOfMachinesReadyForEdgesDistribution=-1;
	m_USE_MPI_Send=0;
	m_USE_MPI_Isend=1;
	m_Sending_Mechanism=m_USE_MPI_Send;
	m_mode_EXTENSION=false;
	m_aborted=false;
	m_sentMessages=0;
	m_readyToSeed=0;
	m_ticks=0;
	m_maxTicks=-1;
	m_watchMaxTicks=false;
	m_receivedMessages=0;
	m_wordSize=-1;
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
	m_sequence_ready_machines=0;





	m_mode=MODE_DO_NOTHING;
	m_mode_AttachSequences=false;
	m_startEdgeDistribution=false;

	m_ranksDoneAttachingReads=0;
	m_VERSION="0.0.0";

	MPI_Init(&argc,&argv);
	MPI_Comm_rank(MPI_COMM_WORLD,&m_rank);
	MPI_Comm_size(MPI_COMM_WORLD,&m_size);
	MPI_Get_processor_name (m_name, &m_nameLen); 
	if(isMaster()){
		cout<<"Rank "<<getRank()<<" welcomes you to the MPI_COMM_WORLD."<<endl;
		cout<<"Rank "<<getRank()<<": website -> http://denovoassembler.sf.net/"<<endl;
		cout<<"Rank "<<getRank()<<": version -> "<<m_VERSION<<" $Id$"<<endl;
		#ifdef OPEN_MPI
		cout<<"Rank "<<getRank()<<" using Open MPI "<<OMPI_MAJOR_VERSION<<"."<<OMPI_MINOR_VERSION<<"."<<OMPI_RELEASE_VERSION<<endl;
		#else
		cout<<"Rank "<<getRank()<<" MPI implementation is not Open MPI."<<endl;
		#endif
	}
	m_alive=true;
	m_welcomeStep=true;
	m_loadSequenceStep=false;
	m_inputFile=argv[1];
	m_vertices_sent=0;
	m_totalLetters=0;
	m_distribution_file_id=m_distribution_sequence_id=m_distribution_currentSequenceId=0;
	if(false and isMaster()){
		string pouet="ATCGATCAGCTAGCATCAG";
		int k=pouet.length();
		uint64_t kk=wordId(pouet.c_str());
		uint64_t prefix=getKPrefix(kk,k-1);
		uint64_t suffix=getKSuffix(kk,k-1);
		cout<<"En string: "<<pouet<<endl;
		cout<<"En uint64_t "<<kk<<endl;
		cout<<"En 64 bits:"<<endl;
		coutBIN(kk);
		cout<<"Prefix"<<endl;
		coutBIN(prefix);
		cout<<"Suffix"<<endl;
		coutBIN(suffix);
	}
	if(argc!=2){
		if(isMaster()){
			cout<<"You must provide a input file."<<endl;
		}
	}else{
		run();
	}

	MPI_Barrier(MPI_COMM_WORLD);
	cout<<"Rank "<<getRank()<<" finalizes."<<endl;
	MPI_Finalize();
}

void Machine::run(){
	cout<<"Rank "<<getRank()<<": hello from "<<m_name<<endl;
	if(isMaster()){
		cout<<"Rank "<<getRank()<<": I am the master among "<<getSize()<<" ranks in the MPI_COMM_WORLD."<<endl;
	}
	//cout<<"Rank "<<getRank()<<" is "<<m_name<<endl;
	while(isAlive()){
		if(m_ticks%BARRIER_PERIOD==0){
			if(!(m_watchMaxTicks and m_ticks > m_maxTicks)){
				MPI_Barrier(MPI_COMM_WORLD);
			}else{
			}
		}
		
		if(!(m_watchMaxTicks and m_ticks > m_maxTicks)){
			m_ticks++;
		}else{
			//cout<<"Rank "<<getRank()<<" is ready to die."<<endl;
			m_alive=false;
		}

		receiveMessages(); 
		checkRequests();
		processMessages();
		processData();
		sendMessages();
	}
	MPI_Barrier(MPI_COMM_WORLD);
}

void Machine::sendMessages(){
	for(int i=0;i<(int)m_outbox.size();i++){
		Message*aMessage=&(m_outbox[i]);
		if(aMessage->getDestination()==getRank()){
			int sizeOfElements=8;
			if(aMessage->getTag()==TAG_SEND_SEQUENCE){
				sizeOfElements=1;
			}
			void*newBuffer=m_inboxAllocator.allocate(sizeOfElements*aMessage->getCount());
			memcpy(newBuffer,aMessage->getBuffer(),sizeOfElements*aMessage->getCount());
			aMessage->setBuffer(newBuffer);
			m_inbox.push_back(*aMessage);
			m_receivedMessages++;
		}else{
			if(m_Sending_Mechanism==m_USE_MPI_Send){
				MPI_Send(aMessage->getBuffer(), aMessage->getCount(), aMessage->getMPIDatatype(),aMessage->getDestination(),aMessage->getTag(), MPI_COMM_WORLD);
			}else if(m_Sending_Mechanism==m_USE_MPI_Isend){
				MPI_Request request;
				MPI_Status status;
				int flag;
				MPI_Test(&request,&flag,&status);
				if(!flag){
					m_pendingMpiRequest.push_back(request);
				}
			}
		}
		m_sentMessages++;
	}
	m_outbox.clear();
	if(m_outboxAllocator.getNumberOfChunks()>1){
		m_outboxAllocator.clear();
		m_outboxAllocator.constructor();
	}
}

void Machine::checkRequests(){
	if(m_Sending_Mechanism==m_USE_MPI_Send)
		return;
	MPI_Request theRequests[1024];
	MPI_Status theStatus[1024];
	
	for(int i=0;i<(int)m_pendingMpiRequest.size();i++){
		theRequests[i]=m_pendingMpiRequest[i];
	}
	MPI_Waitall(m_pendingMpiRequest.size(),theRequests,theStatus);
	m_pendingMpiRequest.clear();
}

void Machine::receiveMessages(){
	int numberOfMessages=0;
	int flag;
	MPI_Status status;
	MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&flag,&status);
	while(flag){
		MPI_Datatype datatype=MPI_UNSIGNED_LONG_LONG;
		int sizeOfType=8;
		int tag=status.MPI_TAG;
		if(tag==TAG_SEND_SEQUENCE){
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
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS,getRank());
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
		cout<<"Rank "<<getRank()<<" "<<m_distribution_reads.size()<<" sequences to distribute"<<endl;
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
			Message aMessage(sequence, strlen(sequence), MPI_BYTE, destination, TAG_SEND_SEQUENCE,getRank());
			m_outbox.push_back(aMessage);
		}
		m_distribution_currentSequenceId++;
		m_distribution_sequence_id++;
	}
}

void Machine::attachReads(){
	//cout<<"Rank "<<getRank()<<" Attaching reads."<<endl;
	vector<string> allFiles=m_parameters.getAllFiles();
	if(m_distribution_reads.size()>0 and m_distribution_sequence_id>(int)m_distribution_reads.size()-1){
		m_distribution_file_id++;
		m_distribution_sequence_id=0;
		m_distribution_reads.clear();
	}
	if(m_distribution_file_id>(int)allFiles.size()-1){
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_MASTER_IS_DONE_ATTACHING_READS,getRank());
			m_outbox.push_back(aMessage);
		}
		m_distribution_reads.clear();
		m_distributionAllocator.clear();
		m_mode_AttachSequences=false;
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
		cout<<"Rank "<<getRank()<<" "<<m_distribution_reads.size()<<" sequences to attach"<<endl;
	}
	for(int i=0;i<1;i++){
		if(m_distribution_sequence_id>(int)m_distribution_reads.size()-1)
			break;

		int destination=m_distribution_currentSequenceId%getSize();
		int sequenceIdOnDestination=m_distribution_currentSequenceId/getSize();

		if(destination<0 or destination>getSize()-1){
			cout<<destination<<" is bad"<<endl;
		}
		char*sequence=m_distribution_reads[m_distribution_sequence_id]->getSeq();
		char vertexChar[100];
		memcpy(vertexChar,sequence,m_wordSize);
		vertexChar[m_wordSize]='\0';
		if(isValidDNA(vertexChar)){
			uint64_t vertex=wordId(vertexChar);
			// ask the machine with sequenceIdOnDestination to send the associated pointer and rank to the rank that holds vertex.
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(2*sizeof(uint64_t));
			message[0]=vertex;
			message[1]=sequenceIdOnDestination;
			Message aMessage(message,2, MPI_UNSIGNED_LONG_LONG, destination, TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER,getRank());
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


	if(tag==TAG_VERTICES_DATA){
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
	}else if(tag==TAG_EXTENSION_IS_DONE){
		m_EXTENSION_currentRankIsDone=true;
	}else if(tag==TAG_SET_WORD_SIZE){
		uint64_t*incoming=(uint64_t*)buffer;
		m_wordSize=incoming[0];
		cout<<"Rank "<<getRank()<<" WordSize="<<m_wordSize<<endl;
	}else if(tag==TAG_START_SEEDING){
		m_mode=MODE_START_SEEDING;
		m_SEEDING_iterator=new SplayTreeIterator<uint64_t,Vertex>(&m_subgraph);
		m_SEEDING_NodeInitiated=false;
		m_SEEDING_i=0;
	}else if(tag==TAG_REQUEST_VERTEX_COVERAGE){
		uint64_t*incoming=(uint64_t*)buffer;
		SplayNode<uint64_t,Vertex>*node=(SplayNode<uint64_t,Vertex>*)incoming[0];
		uint64_t coverage=node->getValue()->getCoverage();
		//cout<<"Rank "<<getRank()<<" ptr "<<node<<" resolves. "<<node->getKey()<<" "<<coverage<<endl;
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
		message[0]=coverage;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_COVERAGE_REPLY,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==TAG_REQUEST_VERTEX_INGOING_EDGES){
		uint64_t*incoming=(uint64_t*)buffer;
		SplayNode<uint64_t,Vertex>*node=(SplayNode<uint64_t,Vertex>*)incoming[0];
		int c=0;
		if(node==NULL){
			cout<<"Invalid pointer."<<endl;
		}
			
		Edge*e=node->getValue()->getFirstIngoingEdge();
		while(e!=NULL){
			c++;
			e=e->getNext();
		}
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(c*2*sizeof(uint64_t));
		int i=0;
		e=node->getValue()->getFirstIngoingEdge();
		while(e!=NULL){
			message[i]=e->getRank();
			i++;
			message[i]=(uint64_t)e->getPtr();
			++i;
			e=e->getNext();
		}
		Message aMessage(message,c*2,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY,getRank());
		m_outbox.push_back(aMessage);
		//cout<<"Rank "<<getRank()<<" sends outgoing edges."<<endl;
	}else if(tag==TAG_ASK_EXTENSION){
		m_EXTENSION_initiated=false;
		m_mode_EXTENSION=true;
	}else if(tag==TAG_ASK_REVERSE_COMPLEMENT){
		uint64_t*incoming=(uint64_t*)buffer;
		SplayNode<uint64_t,Vertex>*node=(SplayNode<uint64_t,Vertex>*)incoming[0];
		uint64_t value=node->getKey();
		uint64_t reverseComplement=complementVertex(value,m_wordSize);
		int rank=vertexRank(reverseComplement);
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(2*sizeof(uint64_t));
		Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,rank,TAG_REQUEST_VERTEX_POINTER,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==TAG_ASK_IS_ASSEMBLED){
		uint64_t*incoming=(uint64_t*)buffer;
		SplayNode<uint64_t,Vertex>*node=(SplayNode<uint64_t,Vertex>*)incoming[0];
		bool isAssembled=node->getValue()->isAssembled();
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
		message[0]=isAssembled;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_IS_ASSEMBLED_REPLY,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==TAG_ASK_IS_ASSEMBLED_REPLY){
		uint64_t*incoming=(uint64_t*)buffer;
		m_EXTENSION_VertexAssembled_received=true;
		m_EXTENSION_vertexIsAssembledResult=(bool)incoming[0];
	}else if(tag==TAG_REQUEST_VERTEX_OUTGOING_EDGES){
		uint64_t*incoming=(uint64_t*)buffer;
		SplayNode<uint64_t,Vertex>*node=(SplayNode<uint64_t,Vertex>*)incoming[0];
		//cout<<"Rank "<<getRank()<<" prepares outgoing edges, pointer="<<node<<"."<<endl;
		int c=0;
		if(node==NULL){
			cout<<"Invalid pointer."<<endl;
		}
			
		Edge*e=node->getValue()->getFirstOutgoingEdge();
		while(e!=NULL){
			c++;
			e=e->getNext();
		}
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(c*2*sizeof(uint64_t));
		int i=0;
		e=node->getValue()->getFirstOutgoingEdge();
		while(e!=NULL){
			message[i]=e->getRank();
			i++;
			message[i]=(uint64_t)e->getPtr();
			++i;
			e=e->getNext();
		}
		Message aMessage(message,c*2,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,getRank());
		m_outbox.push_back(aMessage);
		//cout<<"Rank "<<getRank()<<" sends outgoing edges."<<endl;
	}else if(tag==TAG_GOOD_JOB_SEE_YOU_SOON){
		//m_alive=false;
		uint64_t*incoming=(uint64_t*)buffer;
		m_maxTicks=(incoming[0]+1000);
		
		//cout<<"Rank "<<getRank()<<" ticks="<<m_ticks<<", maxTicks="<<m_maxTicks<<endl;
		m_watchMaxTicks=true;
	}else if(tag==TAG_SEEDING_IS_OVER){
		m_numberOfRanksDoneSeeding++;
	}else if(tag==TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER){
		uint64_t*incoming=(uint64_t*)buffer;
		uint64_t vertex=incoming[0];
		int sequenceIdOnDestination=(int)incoming[1];
		if(sequenceIdOnDestination%10000==0){
			cout<<"Rank "<<getRank()<<" attaches sequences. "<<sequenceIdOnDestination<<"/"<<m_myReads.size()<<endl;
		}
		//cout<<"Rank "<<getRank()<<" "<<sequenceIdOnDestination<<" of "<<m_myReads.size()<<endl;
		void*pointer=(void*)m_myReads[sequenceIdOnDestination];
		int rankToSendInformation=vertexRank(vertex);
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(2*sizeof(uint64_t));
		message[0]=vertex;
		message[1]=(uint64_t)pointer;
		Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,rankToSendInformation,TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY){
		uint64_t*incoming=(uint64_t*)buffer;
		uint64_t vertex=incoming[0];
		void*ptr=(void*)incoming[1];
		
		SplayNode<uint64_t,Vertex>*node=m_subgraph.find(vertex);
		if(node==NULL){
			cout<<" Rank="<<getRank()<<" NULL "<<vertex<<endl;
		}else{
			Vertex*vertex=node->getValue();
			vertex->addRead(source,ptr,&m_persistentAllocator);
		}
	}else if(tag==TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY){
		uint64_t*incoming=(uint64_t*)buffer;
		int i=0;
		m_SEEDING_Inedge=NULL;
		while(i<count){
			Edge*edge=(Edge*)m_seedingAllocator.allocate(sizeof(Edge));
			int rank=incoming[i];
			i++;
			void*ptr=(void*)incoming[i];
			++i;
			edge->constructor(rank,ptr);
			if(m_SEEDING_Inedge==NULL){
			}else{
				edge->setNext(m_SEEDING_Inedge);
			}
			m_SEEDING_Inedge=edge;
		}
		m_SEEDING_InedgesReceived=true;
	}else if(tag==TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY){
		uint64_t*incoming=(uint64_t*)buffer;
		int i=0;
		m_SEEDING_edge=NULL;
		while(i<count){
			Edge*edge=(Edge*)m_seedingAllocator.allocate(sizeof(Edge));
			int rank=incoming[i];
			i++;
			void*ptr=(void*)incoming[i];
			++i;
			edge->constructor(rank,ptr);
			if(m_SEEDING_edge==NULL){
			}else{
				edge->setNext(m_SEEDING_edge);
			}
			m_SEEDING_edge=edge;
		}
		m_SEEDING_edgesReceived=true;
	}else if(tag==TAG_MASTER_IS_DONE_ATTACHING_READS){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,getRank());
		m_outbox.push_back(aMessage);
		cout<<"Rank "<<getRank()<<" attaches sequences. "<<m_myReads.size()<<"/"<<m_myReads.size()<<" (DONE)"<<endl;
	}else if(tag==TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY){
		m_ranksDoneAttachingReads++;
	}else if(tag==TAG_REQUEST_VERTEX_KEY_AND_COVERAGE){
		uint64_t*incoming=(uint64_t*)buffer;
		SplayNode<uint64_t,Vertex>*node=(SplayNode<uint64_t,Vertex>*)incoming[0];
		uint64_t key=node->getKey();
		uint64_t coverage=node->getValue()->getCoverage();
		//cout<<"Rank "<<getRank()<<" ptr "<<node<<" resolves. "<<node->getKey()<<" "<<coverage<<endl;
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(2*sizeof(uint64_t));
		message[0]=key;
		message[1]=coverage;
		Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY,getRank());
		m_outbox.push_back(aMessage);
		//cout<<"Rank "<<getRank()<<" replies to a TAG_REQUEST_VERTEX_KEY_AND_COVERAGE"<<endl;
	}else if(tag==TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY){
		uint64_t*incoming=(uint64_t*)buffer;
		m_SEEDING_receivedKey=incoming[0];
		m_SEEDING_receivedVertexCoverage=incoming[1];
		m_SEEDING_vertexKeyAndCoverageReceived=true;
		//cout<<"Rank "<<getRank()<<" received key and coverage (via MPI_Recv.)."<<endl;
	}else if(tag==TAG_REQUEST_VERTEX_COVERAGE_REPLY){
		uint64_t*incoming=(uint64_t*)buffer;
		m_SEEDING_receivedVertexCoverage=incoming[0];
		m_SEEDING_vertexCoverageReceived=true;
		//cout<<"Coverage is "<<m_SEEDING_receivedVertexCoverage<<endl;
	// receive coverage data, and merge it
	}else if(tag==TAG_COVERAGE_DATA){
		int length=count;
		uint64_t*incoming=(uint64_t*)buffer;

		for(int i=0;i<length;i+=2){
			int coverage=incoming[i+0];
			uint64_t count=incoming[i+1];
			//cout<<coverage<<" "<<count<<endl;
			m_coverageDistribution[coverage]+=count;
		}
	// receive an outgoing edge in respect to prefix, along with the pointer for suffix
	}else if(tag==TAG_OUT_EDGE_DATA_WITH_PTR){
		uint64_t*incoming=(uint64_t*)buffer;
		void*ptr=(void*)incoming[1];
		uint64_t prefix=incoming[0];
		//cout<<"Rank "<<getRank()<<" TAG_OUT_EDGE_DATA_WITH_PTR "<<idToWord(prefix,m_wordSize)<<"->"<<idToWord(suffix,m_wordSize)<<endl;
		int rank=source;
		
		SplayNode<uint64_t,Vertex>*node=m_subgraph.find(prefix);
		if(node==NULL){
			cout<<"NULL "<<prefix<<" Rank="<<getRank()<<endl;
		}else{
			Vertex*vertex=node->getValue();
			vertex->addOutgoingEdge(rank,ptr,&m_persistentAllocator);
		}
	// receive an ingoing edge in respect to prefix, along with the pointer for suffix
	}else if(tag==TAG_IN_EDGE_DATA_WITH_PTR){
		uint64_t*incoming=(uint64_t*)buffer;
		uint64_t suffix=incoming[0];
		void*ptr=(void*)incoming[1];
		int rank=source;


		SplayNode<uint64_t,Vertex>*node=m_subgraph.find(suffix);
		if(node==NULL){
			//cout<<"NULL "<<prefix<<endl;
		}else{
			Vertex*vertex=node->getValue();
			vertex->addIngoingEdge(rank,ptr,&m_persistentAllocator);
		}
	}else if(tag==TAG_SEND_COVERAGE_VALUES){
		uint64_t*incoming=(uint64_t*)buffer;
		m_minimumCoverage=incoming[0];
		m_seedCoverage=incoming[1];
		m_peakCoverage=incoming[2];
		Message aMessage(m_name,NULL,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_READY_TO_SEED,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==TAG_READY_TO_SEED){
		m_readyToSeed++;
	// receive a out edge, send it back with the pointer
	}else if(tag==TAG_OUT_EDGES_DATA){
		int length=count;
		uint64_t*incoming=(uint64_t*)buffer;
		

		for(int i=0;i<(int)length;i+=2){
			int currentLength=2;
			uint64_t*sendBuffer=(uint64_t*)m_outboxAllocator.allocate(currentLength*sizeof(uint64_t));
			sendBuffer[0]=incoming[i+0];
			sendBuffer[1]=(uint64_t)m_subgraph.find(incoming[i+1]);
			int destination=vertexRank(incoming[i+0]);


			Message aMessage(sendBuffer,currentLength,MPI_UNSIGNED_LONG_LONG,destination,TAG_OUT_EDGE_DATA_WITH_PTR,getRank());
			m_outbox.push_back(aMessage);
		}

	// receive an ingoing edge, send it back with the pointer
	}else if(tag==TAG_IN_EDGES_DATA){
		int length=count;
		uint64_t*incoming=(uint64_t*)buffer;

		for(int i=0;i<(int)length;i+=2){
			int currentLength=3;
			uint64_t*sendBuffer=(uint64_t*)m_outboxAllocator.allocate(currentLength*sizeof(uint64_t));
			sendBuffer[0]=incoming[i+1];
			sendBuffer[1]=(uint64_t)m_subgraph.find(incoming[i+0]);
			
			int destination=vertexRank(incoming[i+1]);

			Message aMessage(sendBuffer,currentLength,MPI_UNSIGNED_LONG_LONG,destination,TAG_IN_EDGE_DATA_WITH_PTR,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(tag==TAG_WELCOME){

	}else if(tag==TAG_SEND_SEQUENCE){
		int length=count;
		char*incoming=(char*)m_inboxAllocator.allocate(count*sizeof(char)+1);
		for(int i=0;i<(int)length;i++)
			incoming[i]=((char*)buffer)[i];

		incoming[length]='\0';
		Read*myRead=(Read*)m_persistentAllocator.allocate(sizeof(Read));
		myRead->copy(NULL,incoming,&m_persistentAllocator);
		m_myReads.push_back(myRead);
		if(m_myReads.size()%10000==0){
			cout<<"Rank "<<getRank()<<" has "<<m_myReads.size()<<" sequences"<<endl;
		}
	}else if(tag==TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS){
		cout<<"Rank "<<getRank()<<" has "<<m_myReads.size()<<" sequences"<<endl;
		char*message=m_name;
		Message aMessage(message,0,MPI_UNSIGNED_LONG_LONG,source,TAG_SEQUENCES_READY,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==TAG_SHOW_VERTICES){
		cout<<"Rank "<<getRank()<<" has "<<m_subgraph.size()<<" vertices (DONE)"<<endl;
	}else if(tag==TAG_SEQUENCES_READY){
		m_sequence_ready_machines++;
	}else if(tag==TAG_START_EDGES_DISTRIBUTION_ANSWER){
		m_numberOfMachinesReadyForEdgesDistribution++;
		//cout<<"Rank "<<getRank()<<" m_numberOfMachinesReadyForEdgesDistribution="<<m_numberOfMachinesReadyForEdgesDistribution<<endl;
	}else if(tag==TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER){
		m_numberOfMachinesReadyToSendDistribution++;
	}else if(tag==TAG_PREPARE_COVERAGE_DISTRIBUTION){
		//cout<<"Rank "<<getRank()<<" prepares its distribution."<<endl;
		m_mode_send_coverage_iterator=0;
		m_mode_sendDistribution=true;
	}else if(tag==TAG_START_EDGES_DISTRIBUTION_ASK){
		char*message=m_name;
		Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, source, TAG_START_EDGES_DISTRIBUTION_ANSWER,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION){
		char*message=m_name;
		Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, source, TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER,getRank());
		m_outbox.push_back(aMessage);
	}else if(tag==TAG_START_EDGES_DISTRIBUTION){
		m_mode_send_outgoing_edges=true;
		m_mode_send_edge_sequence_id=0;
	}else if(tag==TAG_START_VERTICES_DISTRIBUTION){
		m_mode_send_vertices=true;
		m_mode_send_vertices_sequence_id=0;
	}else if(tag==TAG_VERTICES_DISTRIBUTED){
		m_numberOfMachinesDoneSendingVertices++;
	}else if(tag==TAG_COVERAGE_END){
		m_numberOfMachinesDoneSendingCoverage++;
		//cout<<"m_numberOfMachinesDoneSendingCoverage="<<m_numberOfMachinesDoneSendingCoverage<<" "<<source<<" is done"<<endl;
	}else if(tag==TAG_EDGES_DISTRIBUTED){
		m_numberOfMachinesDoneSendingEdges++;
	}else{
		cout<<"Unknown tag"<<tag<<endl;
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
	if(m_aborted){
		return;
	}
	if(!m_parameters.isInitiated()&&isMaster()){
		ifstream f(m_inputFile);
		if(!f){
			cout<<"Rank "<<getRank()<<" invalid input file."<<endl;
			m_aborted=true;
			killRanks();
			return;
		}
		m_parameters.load(m_inputFile);
		for(int i=0;i<getSize();i++){
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			message[0]=m_parameters.getWordSize();
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,i,TAG_SET_WORD_SIZE,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_welcomeStep==true && m_loadSequenceStep==false&&isMaster()){
		loadSequences();
	}else if(m_loadSequenceStep==true && m_mode_send_vertices==false&&isMaster() and m_sequence_ready_machines==getSize()&&m_messageSentForVerticesDistribution==false){
		cout<<"Rank "<<getRank()<<" tells others to start vertices distribution"<<endl;
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			Message aMessage(sequence, 0, MPI_UNSIGNED_LONG_LONG,i, TAG_START_VERTICES_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
		m_messageSentForVerticesDistribution=true;
	}else if(m_numberOfMachinesDoneSendingVertices==getSize()){
		m_numberOfMachinesReadyForEdgesDistribution=0;
		m_numberOfMachinesDoneSendingVertices=-1;
		for(int i=0;i<getSize();i++){
			char*message=m_name;
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_SHOW_VERTICES,getRank());
			m_outbox.push_back(aMessage);
		}

		cout<<"Rank "<<getRank()<<" attaches reads now."<<endl;
		m_mode_AttachSequences=true;
		m_distribution_file_id=m_distribution_sequence_id=m_distribution_currentSequenceId=0;
		m_startEdgeDistribution=false;
	}else if(m_startEdgeDistribution){
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			Message aMessage(sequence, 0, MPI_UNSIGNED_LONG_LONG,i, TAG_START_EDGES_DISTRIBUTION_ASK,getRank());
			m_outbox.push_back(aMessage);
		}
		m_startEdgeDistribution=false;
	}else if(m_numberOfMachinesReadyForEdgesDistribution==getSize() and isMaster()){
		cout<<"Rank "<<getRank()<<" tells others to start edges distribution."<<endl;// m_numberOfMachinesReadyForEdgesDistribution="<<m_numberOfMachinesReadyForEdgesDistribution<<endl;
		m_numberOfMachinesReadyForEdgesDistribution=-1;
		for(int i=0;i<getSize();i++){
			char*sequence=m_name;
			Message aMessage(sequence, 0, MPI_UNSIGNED_LONG_LONG,i, TAG_START_EDGES_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_numberOfMachinesDoneSendingCoverage==getSize()){
		m_numberOfMachinesDoneSendingCoverage=-1;
		CoverageDistribution distribution(m_coverageDistribution,m_parameters.getDirectory());
		m_minimumCoverage=distribution.getMinimumCoverage();
		m_peakCoverage=distribution.getPeakCoverage();
		m_seedCoverage=(m_minimumCoverage+m_peakCoverage)/2;
		cout<<"Rank "<<getRank()<<" MinimumCoverage = "<<m_minimumCoverage<<endl;
		cout<<"Rank "<<getRank()<<" PeakCoverage = "<<m_peakCoverage<<endl;
		cout<<"Rank "<<getRank()<<" SeedCoverage = "<<m_seedCoverage<<endl;

		// see these values to everyone.
		uint64_t*buffer=(uint64_t*)m_outboxAllocator.allocate(3*sizeof(uint64_t));
		buffer[0]=m_minimumCoverage;
		buffer[1]=m_seedCoverage;
		buffer[2]=m_peakCoverage;
		for(int i=0;i<getSize();i++){
			Message aMessage(buffer,3,MPI_UNSIGNED_LONG_LONG,i,TAG_SEND_COVERAGE_VALUES,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_mode_send_vertices==true){
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
				Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_VERTICES_DISTRIBUTED,getRank());
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
				
				
				Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_VERTICES_DATA,getRank());
				m_outbox.push_back(aMessage);
				m_vertices_sent+=length;
			}

			if(m_mode_send_vertices_sequence_id_position>lll){
				m_mode_send_vertices_sequence_id++;
				m_mode_send_vertices_sequence_id_position=0;
			}
		}
		
	}else if(m_numberOfMachinesDoneSendingEdges==getSize()){
		m_numberOfMachinesDoneSendingEdges=-1;
		cout<<"Rank "<<getRank()<<" says that edges are distributed."<<endl;
		for(int i=0;i<getSize();i++){
			char*message=m_name;
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, i, TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_numberOfMachinesReadyToSendDistribution==getSize()){
		//m_numberOfMachinesReadyToSendDistribution=-1;

		if(m_machineRank<=m_numberOfMachinesDoneSendingCoverage){
			cout<<"Rank "<<getRank()<<" tells "<<m_machineRank<<" to distribute its distribution."<<endl;
			char*message=m_name;
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG, m_machineRank, TAG_PREPARE_COVERAGE_DISTRIBUTION,getRank());
			m_outbox.push_back(aMessage);
			m_machineRank++;
		}

		if(m_machineRank==getSize()){
			m_numberOfMachinesReadyToSendDistribution=-1;
		}
	}else if(m_ranksDoneAttachingReads==getSize()){
		m_ranksDoneAttachingReads=-1;
		m_startEdgeDistribution=true;
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
		Message aMessage(data,length, MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_COVERAGE_DATA,getRank());
		m_outbox.push_back(aMessage);

		m_mode_send_coverage_iterator++;
		if(m_mode_send_coverage_iterator>=(int)m_distributionOfCoverage.size()){
			m_mode_sendDistribution=false;
			char*message=m_name;
			Message aMessage(message, 0, MPI_UNSIGNED_LONG_LONG,MASTER_RANK, TAG_COVERAGE_END,getRank());
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
	
				Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_OUT_EDGES_DATA,getRank());
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
				Message aMessage(message, strlen(message), MPI_UNSIGNED_LONG_LONG, MASTER_RANK, TAG_EDGES_DISTRIBUTED,getRank());
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
	
				Message aMessage(data, length, MPI_UNSIGNED_LONG_LONG,destination, TAG_IN_EDGES_DATA,getRank());
				m_outbox.push_back(aMessage);
			}

			if(m_mode_send_edge_sequence_id_position>lll){
				m_mode_send_edge_sequence_id++;
				m_mode_send_edge_sequence_id_position=0;
			}
		}
	}else if(m_readyToSeed==getSize()){
		m_readyToSeed=-1;
		m_numberOfRanksDoneSeeding=0;
		// tell everyone to seed now.
		for(int i=0;i<getSize();i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,TAG_START_SEEDING,getRank());
			m_outbox.push_back(aMessage);
		}
	}else if(m_mode==MODE_START_SEEDING){

		// assign a first vertex
		if(!m_SEEDING_NodeInitiated){
			if(!m_SEEDING_iterator->hasNext()){
				m_mode=MODE_DO_NOTHING;
				cout<<"Rank "<<getRank()<<" seeding vertices. "<<m_SEEDING_i<<"/"<<m_subgraph.size()<<" (DONE)"<<endl;
				m_seedingAllocator.clear();
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_SEEDING_IS_OVER,getRank());
				m_outbox.push_back(aMessage);
			}else{
				if(m_SEEDING_i % 10000 ==0){
					cout<<"Rank "<<getRank()<<" seeding vertices. "<<m_SEEDING_i<<"/"<<m_subgraph.size()<<endl;
				}
				m_SEEDING_node=m_SEEDING_iterator->next();
				m_SEEDING_currentRank=getRank();
				m_SEEDING_currentVertex=m_SEEDING_node->getKey();
				m_SEEDING_currentPointer=(void*)m_SEEDING_node;
				m_SEEDING_testInitiated=false;
				m_SEEDING_1_1_test_done=false;
				m_SEEDING_i++;
				m_SEEDING_NodeInitiated=true;
				m_SEEDING_firstVertexTestDone=false;
				//cout<<"Rank "<<getRank()<<" setting first "<<idToWord(m_SEEDING_currentVertex,m_wordSize)<<endl;
				if(m_seedingAllocator.getNumberOfChunks()>1){
					m_seedingAllocator.clear();
					m_seedingAllocator.constructor();
				}	
			}
		// check that this node has 1 ingoing edge and 1 outgoing edge.
		}else if(!m_SEEDING_firstVertexTestDone){
			if(!m_SEEDING_1_1_test_done){
				do_1_1_test();
			}else{
				if(!m_SEEDING_1_1_test_result){
					m_SEEDING_NodeInitiated=false;// abort
					//cout<<"Rank "<<getRank()<<" aborts at first."<<endl;
				}else{
					m_SEEDING_firstVertexParentTestDone=false;
					m_SEEDING_firstVertexTestDone=true;
					m_SEEDING_currentRank=m_SEEDING_currentParentRank;
					m_SEEDING_currentPointer=m_SEEDING_currentParentPointer;
					//cout<<"Rank "<<getRank()<<" Check parent edge. "<<idToWord(m_SEEDING_currentParentVertex,m_wordSize)<<"->"<<idToWord(m_SEEDING_currentVertex,m_wordSize)<<endl;
					m_SEEDING_currentVertex=m_SEEDING_currentParentVertex;
					m_SEEDING_testInitiated=false;
					m_SEEDING_1_1_test_done=false;
					//cout<<"Rank "<<getRank()<<" first is ok."<<endl;
				}
			}

		// check that the parent does not have 1 ingoing edge and 1 outgoing edge
		}else if(!m_SEEDING_firstVertexParentTestDone){
			if(!m_SEEDING_1_1_test_done){
				do_1_1_test();
			}else{
				if(m_SEEDING_1_1_test_result){
					m_SEEDING_NodeInitiated=false;//abort
				}else{
					m_SEEDING_firstVertexParentTestDone=true;
					m_SEEDING_currentSeedRanks.clear();
					m_SEEDING_currentSeedPointers.clear();
					m_SEEDING_vertices.clear();
					// restore original starter.
					m_SEEDING_currentVertex=m_SEEDING_node->getKey();
					m_SEEDING_currentPointer=(void*)m_SEEDING_node;
					m_SEEDING_currentRank=getRank();
					m_SEEDING_testInitiated=false;
					m_SEEDING_1_1_test_done=false;
					//cout<<"Rank "<<getRank()<<" first's parent is ok too."<<endl;
				}
			}

		// check if currentVertex has 1 ingoing edge and 1 outgoing edge, if yes, add it
		}else{
			// attempt to add m_SEEDING_currentVertex
			if(!m_SEEDING_1_1_test_done){
				do_1_1_test();
			}else{
				if(m_SEEDING_vertices.count(m_SEEDING_currentVertex)>0){
					m_SEEDING_1_1_test_result=false;
					//cout<<"Rank "<<getRank()<<" already in ";
					cout<<endl;
				}
				if(!m_SEEDING_1_1_test_result){
					m_SEEDING_NodeInitiated=false;
					//cout<<"Rank "<<getRank()<<" completed with "<<m_SEEDING_seed.size()<<" vertices result="<<m_SEEDING_1_1_test_result<<endl;
					ostringstream a;
/*
					a<<idToWord(m_SEEDING_seed[0],m_wordSize);
					for(int i=1;i<(int)m_SEEDING_seed.size();i++){
						a<<getLastSymbol(m_SEEDING_seed[i],m_wordSize);
					}
*/
					string contig=a.str();
					cout<<"Rank "<<getRank()<<endl<<m_SEEDING_currentSeedPointers.size()<<" contig "<<contig<<endl;
					m_SEEDING_seedRanks.push_back(m_SEEDING_currentSeedRanks);
					m_SEEDING_seedPointers.push_back(m_SEEDING_currentSeedPointers);
				}else{
					//cout<<"Rank "<<getRank()<<" adding "<<idToWord(m_SEEDING_currentVertex,m_wordSize)<<" Rank="<<m_SEEDING_currentRank<<" Pointer="<<m_SEEDING_currentPointer<<endl;
					//m_SEEDING_seed.push_back(m_SEEDING_currentVertex);
					m_SEEDING_currentSeedRanks.push_back(m_SEEDING_currentRank);
					m_SEEDING_currentSeedPointers.push_back(m_SEEDING_currentPointer);

					m_SEEDING_vertices.insert(m_SEEDING_currentVertex);
					m_SEEDING_currentRank=m_SEEDING_currentChildRank;
					m_SEEDING_currentPointer=m_SEEDING_currentChildPointer;
					//cout<<"Rank "<<getRank()<<" seeking next Rank="<<m_SEEDING_currentChildRank<<" Pointer="<<m_SEEDING_currentChildPointer<<" visual: "<<idToWord(m_SEEDING_currentVertex,m_wordSize)<<"->"<<idToWord(m_SEEDING_currentChildVertex,m_wordSize)<<endl;
					m_SEEDING_currentVertex=m_SEEDING_currentChildVertex;
					m_SEEDING_testInitiated=false;
					m_SEEDING_1_1_test_done=false;
				}
			}
		}
	}else if(m_numberOfRanksDoneSeeding==getSize()){
		m_numberOfRanksDoneSeeding=-1;
		cout<<"Rank "<<getRank()<<" All work is done, good job ranks!"<<endl;
		killRanks();

		//m_mode=MODE_EXTENSION_ASK;
		//m_EXTENSION_rank=-1;
		//m_EXTENSION_currentRankIsSet=false;
	}else if(m_mode_AttachSequences){
		attachReads();
	}else if(m_mode==MODE_EXTENSION_ASK and isMaster()){
		if(!m_EXTENSION_currentRankIsSet){
			m_EXTENSION_currentRankIsSet=true;
			m_EXTENSION_currentRankIsStarted=false;
			if(m_EXTENSION_rank==-1){
				m_EXTENSION_rank=0;
			}else{
				m_EXTENSION_rank++;
			}
		}
		if(m_EXTENSION_rank==getSize()){//done
			cout<<"Rank "<<getRank()<<": extension done."<<endl;
			m_mode=MODE_DO_NOTHING;
			killRanks();
		}else if(!m_EXTENSION_currentRankIsStarted){
			m_EXTENSION_currentRankIsStarted=true;
			cout<<"Rank "<<getRank()<<" tells "<<m_EXTENSION_rank<<" to extend its seeds."<<endl;
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_rank,TAG_ASK_EXTENSION,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_currentRankIsDone=false;
		}else if(m_EXTENSION_currentRankIsDone){
			m_EXTENSION_currentRankIsSet=false; // go with the next one.
		}
	}

	if(m_mode_EXTENSION){
		extendSeeds();
	}
}

/*
 * check if (m_SEEDING_currentRank,m_SEEDING_currentPointer) has
 * 1 ingoing edge and 1 outgoing edge
 *
 * before entering the first call, m_SEEDING_testInitiated and m_SEEDING_1_1_test_done must be false
 *
 * outputs:
 *
 *  m_SEEDING_1_1_test_done
 *  m_SEEDING_currentChildVertex
 *  m_SEEDING_currentChildRank
 *  m_SEEDING_currentChildPointer
 *  m_SEEDING_currentParentRank
 *  m_SEEDING_currentParentPointer
 *
 *
 *  internals:
 *
 *  m_SEEDING_InedgesRequested
 *  m_SEEDING_InedgesReceived
 *  m_SEEDING_Inedge
 *  m_SEEDING_edgesRequested
 *  m_SEEDING_edgesReceived
 *  m_SEEDING_edge
 */
void Machine::do_1_1_test(){
	
	if(m_SEEDING_1_1_test_done){
		return;
	}else if(!m_SEEDING_testInitiated){
		//cout<<"Rank "<<getRank()<<" initiating Rank="<<m_SEEDING_currentRank<<" Pointer="<<m_SEEDING_currentPointer<<endl;
		m_SEEDING_testInitiated=true;
		m_SEEDING_ingoingEdgesDone=false;
		m_SEEDING_InedgesRequested=false;
		m_SEEDING_outgoingEdgesDone=false;
	}else if(!m_SEEDING_ingoingEdgesDone){
		if(!m_SEEDING_InedgesRequested){
			//cout<<"Rank "<<getRank()<<" requests ingoing edges."<<endl;
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			message[0]=(uint64_t)m_SEEDING_currentPointer;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_SEEDING_currentRank,TAG_REQUEST_VERTEX_INGOING_EDGES,getRank());
			m_outbox.push_back(aMessage);
			m_SEEDING_InedgesRequested=true;
			m_SEEDING_numberOfIngoingEdges=0;
			m_SEEDING_numberOfIngoingEdgesWithSeedCoverage=0;
			m_SEEDING_vertexKeyAndCoverageRequested=false;
			m_SEEDING_InedgesReceived=false;
		}else if(m_SEEDING_InedgesReceived){
			if(m_SEEDING_Inedge!=NULL){
				if(!m_SEEDING_vertexKeyAndCoverageRequested){
					uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
					message[0]=(uint64_t)m_SEEDING_Inedge->getPtr();
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_SEEDING_Inedge->getRank(),TAG_REQUEST_VERTEX_KEY_AND_COVERAGE,getRank());
					m_outbox.push_back(aMessage);
					m_SEEDING_vertexKeyAndCoverageRequested=true;
					m_SEEDING_vertexKeyAndCoverageReceived=false;
				}else if(m_SEEDING_vertexKeyAndCoverageReceived){
					if(m_SEEDING_numberOfIngoingEdges==0 and m_SEEDING_Inedge->getNext()==NULL){//there is only one anyway
						m_SEEDING_currentParentRank=m_SEEDING_Inedge->getRank();
						m_SEEDING_currentParentPointer=m_SEEDING_Inedge->getPtr();
						m_SEEDING_currentParentVertex=m_SEEDING_receivedKey;
					}
					if(m_SEEDING_receivedVertexCoverage>=m_seedCoverage){
						m_SEEDING_currentParentVertex=m_SEEDING_receivedKey;
						m_SEEDING_currentParentRank=m_SEEDING_Inedge->getRank();
						m_SEEDING_currentParentPointer=m_SEEDING_Inedge->getPtr();
						m_SEEDING_numberOfIngoingEdgesWithSeedCoverage++;
					}
					m_SEEDING_Inedge=m_SEEDING_Inedge->getNext();
					m_SEEDING_numberOfIngoingEdges++;
					//cout<<"Rank "<<getRank()<<" checking ingoing edge. "<<idToWord(m_SEEDING_receivedKey,m_wordSize)<<"->"<<idToWord(m_SEEDING_currentVertex,m_wordSize)<<endl;
				}
			}else{// done analyzing ingoing edges.
				m_SEEDING_ingoingEdgesDone=true;
				m_SEEDING_outgoingEdgesDone=false;
				m_SEEDING_edgesRequested=false;
				//cout<<"Rank "<<getRank()<<" ingoing is done., edges="<<m_SEEDING_numberOfIngoingEdges<<" >=sc "<<m_SEEDING_numberOfIngoingEdgesWithSeedCoverage<<""<<endl;
			}
		}
	}else if(!m_SEEDING_outgoingEdgesDone){
		if(!m_SEEDING_edgesRequested){
			//cout<<"Rank "<<getRank()<<" requests outgoing edges."<<endl;
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			message[0]=(uint64_t)m_SEEDING_currentPointer;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_SEEDING_currentRank,TAG_REQUEST_VERTEX_OUTGOING_EDGES,getRank());
			m_outbox.push_back(aMessage);
			m_SEEDING_edgesRequested=true;
			m_SEEDING_numberOfOutgoingEdges=0;
			m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage=0;
			m_SEEDING_vertexKeyAndCoverageRequested=false;
			m_SEEDING_edgesReceived=false;
		}else if(m_SEEDING_edgesReceived){
			if(m_SEEDING_edge!=NULL){
				if(!m_SEEDING_vertexKeyAndCoverageRequested){
					uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
					message[0]=(uint64_t)m_SEEDING_edge->getPtr();
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_SEEDING_edge->getRank(),TAG_REQUEST_VERTEX_KEY_AND_COVERAGE,getRank());
					m_outbox.push_back(aMessage);
					m_SEEDING_vertexKeyAndCoverageRequested=true;
					m_SEEDING_vertexKeyAndCoverageReceived=false;
				}else if(m_SEEDING_vertexKeyAndCoverageReceived){
					if(m_SEEDING_numberOfOutgoingEdges==0 and m_SEEDING_edge->getNext()==NULL){//there is only one anyway
						m_SEEDING_currentChildRank=m_SEEDING_edge->getRank();
						m_SEEDING_currentChildPointer=m_SEEDING_edge->getPtr();
						m_SEEDING_currentChildVertex=m_SEEDING_receivedKey;
						//cout<<"Rank "<<getRank()<<" only one "<<idToWord(m_SEEDING_receivedKey,m_wordSize)<<endl;
					}
					if(m_SEEDING_receivedVertexCoverage>=m_seedCoverage){
						m_SEEDING_currentChildRank=m_SEEDING_edge->getRank();
						m_SEEDING_currentChildPointer=m_SEEDING_edge->getPtr();
						m_SEEDING_currentChildVertex=m_SEEDING_receivedKey;
						m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage++;
					}
					//cout<<"Rank "<<getRank()<<" checking outgoing edge. "<<idToWord(m_SEEDING_currentVertex,m_wordSize)<<"->"<<idToWord(m_SEEDING_receivedKey,m_wordSize)<<endl;
					m_SEEDING_edge=m_SEEDING_edge->getNext();
					m_SEEDING_numberOfOutgoingEdges++;
				}
			}else{// done analyzing ingoing edges.
				m_SEEDING_outgoingEdgesDone=true;
			}
		}


	}else{
		m_SEEDING_1_1_test_done=true;
		m_SEEDING_1_1_test_result=(m_SEEDING_numberOfIngoingEdges==1 or m_SEEDING_numberOfIngoingEdgesWithSeedCoverage==1)and
			(m_SEEDING_numberOfOutgoingEdges==1 or m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage==1);
		//cout<<"Rank "<<getRank()<<" result is "<<m_SEEDING_1_1_test_result<<" Rank="<<m_SEEDING_currentRank<<" Pointer="<<m_SEEDING_currentPointer<<" "<<endl;

		//cout<<"Rank "<<getRank()<<" Values "<<m_SEEDING_currentPointer<<" "<<m_SEEDING_numberOfIngoingEdges<<" "<<m_SEEDING_numberOfIngoingEdgesWithSeedCoverage<<" "<<m_SEEDING_numberOfOutgoingEdges<<" "<<m_SEEDING_numberOfOutgoingEdgesWithSeedCoverage<<endl;
	}
}

void Machine::killRanks(){
	for(int i=0;i<getSize();i++){
		uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
		message[0]=m_ticks;
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,i,TAG_GOOD_JOB_SEE_YOU_SOON,getRank());
		m_outbox.push_back(aMessage);
	}
}

bool Machine::isMaster(){
	return getRank()==MASTER_RANK;
}

void Machine::extendSeeds(){
	if(m_SEEDING_seedRanks.size()==0){
		m_mode_EXTENSION=false;
		cout<<"Rank "<<getRank()<<" has finished extending its seeds (0 seeds)."<<endl;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_IS_DONE,getRank());
		m_outbox.push_back(aMessage);
		return;
	}
	if(!m_EXTENSION_initiated){
		m_EXTENSION_initiated=true;
		m_EXTENSION_currentSeedIndex=0;
		m_EXTENSION_currentPosition=0;
		m_EXTENSION_currentRank=m_SEEDING_seedRanks[m_EXTENSION_currentSeedIndex][m_EXTENSION_currentPosition];
		m_EXTENSION_currentPointer=m_SEEDING_seedPointers[m_EXTENSION_currentSeedIndex][m_EXTENSION_currentPosition];
	}
	if(m_EXTENSION_currentSeedIndex==(int)m_SEEDING_seedRanks.size()-1){
		m_mode_EXTENSION=false;
		cout<<"Rank "<<getRank()<<" has finished extending its seeds."<<endl;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_IS_DONE,getRank());
		m_outbox.push_back(aMessage);
		return;
	}

	

	// algorithms here.
	// if the current vertex is assembled or if its reverse complement is assembled, return
	// else, mark it as assembled, and mark its reverse complement as assembled too.
	// 	enumerate the available choices
	// 	if choices are included in the seed itself
	// 		choose it
	// 	else
	// 		use read paths or pairs of reads to resolve the repeat.
	
	if(!m_EXTENSION_checkedIfCurrentVertexIsAssembled){
		checkIfCurrentVertexIsAssembled();
	}else if(m_EXTENSION_vertexIsAssembledResult){
		m_EXTENSION_currentSeedIndex++;// skip the current one.
		// TODO: check if the position !=0
		cout<<"Rank" <<getRank()<<" already assembled."<<endl;
	}else if(!m_EXTENSION_markedCurrentVertexAsAssembled){
		markCurrentVertexAsAssembled();
	}else if(!m_EXTENSION_enumerateChoices){
		enumerateChoices();
	}else if(!m_EXTENSION_choose){
		doChoice();
	}

}

void Machine::enumerateChoices(){
	m_EXTENSION_enumerateChoices=true;
	m_EXTENSION_choose=false;
}

void Machine::doChoice(){
	m_EXTENSION_choose=true;

	// seek next one.
	cout<<"Rank "<<getRank()<<": "<<m_SEEDING_seedRanks[m_EXTENSION_currentSeedIndex].size()<<" VERtices."<<endl;
	m_EXTENSION_currentSeedIndex++;
}
void Machine::checkIfCurrentVertexIsAssembled(){
	if(!m_EXTENSION_directVertexDone){
		if(!m_EXTENSION_VertexAssembled_requested){
			cout<<"Rank "<<getRank()<<" requests 'isAssembled'."<<endl;
			m_EXTENSION_VertexAssembled_requested=true;
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			message[0]=(uint64_t)m_EXTENSION_currentPointer;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_currentRank,TAG_ASK_IS_ASSEMBLED,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_VertexAssembled_received=false;
		}else if(m_EXTENSION_VertexAssembled_received){
			cout<<"Rank "<<getRank()<<" receives 'isAssembled'."<<endl;
			if(m_EXTENSION_receivedAssembled){
				m_EXTENSION_vertexIsAssembledResult=true;
				m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
			}
			m_EXTENSION_directVertexDone=true;
		}
	}else if(!m_EXTENSION_reverseVertexDone){
		if(!m_EXTENSION_reverseComplement_requested){
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			message[0]=(uint64_t)m_EXTENSION_currentPointer;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_currentRank,TAG_ASK_REVERSE_COMPLEMENT,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_reverseComplement_requested=true;
			m_EXTENSION_reverseComplement_received=false;
			m_EXTENSION_VertexAssembled_requested=false;
		}else if(m_EXTENSION_reverseComplement_received){
			if(!m_EXTENSION_VertexAssembled_requested){
				m_EXTENSION_VertexAssembled_requested=true;
				uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
				message[0]=(uint64_t)m_EXTENSION_currentPointer;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_currentRank,TAG_ASK_IS_ASSEMBLED,getRank());
				m_outbox.push_back(aMessage);
				m_EXTENSION_VertexAssembled_received=false;
			}else if(m_EXTENSION_VertexAssembled_received){
				if(m_EXTENSION_receivedAssembled){
					m_EXTENSION_vertexIsAssembledResult=true;
					m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
				}else{
					m_EXTENSION_vertexIsAssembledResult=false;
					m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
				}
				m_EXTENSION_markedCurrentVertexAsAssembled=false;
			}
		}
	}
}

void Machine::markCurrentVertexAsAssembled(){
	if(!m_EXTENSION_directVertexDone){
		if(!m_EXTENSION_VertexMarkAssembled_requested){
			m_EXTENSION_VertexMarkAssembled_requested=true;
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			message[0]=(uint64_t)m_EXTENSION_currentPointer;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_currentRank,TAG_MARK_AS_ASSEMBLED,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_directVertexDone=true;
		}
	}else if(!m_EXTENSION_reverseVertexDone){
		if(!m_EXTENSION_reverseComplement_requested){
			uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
			message[0]=(uint64_t)m_EXTENSION_currentPointer;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_currentRank,TAG_ASK_REVERSE_COMPLEMENT,getRank());
			m_outbox.push_back(aMessage);
			m_EXTENSION_reverseComplement_requested=true;
			m_EXTENSION_reverseComplement_received=false;
			m_EXTENSION_VertexAssembled_requested=false;
		}else if(m_EXTENSION_reverseComplement_received){
			if(!m_EXTENSION_VertexMarkAssembled_requested){
				m_EXTENSION_VertexMarkAssembled_requested=true;
				uint64_t*message=(uint64_t*)m_outboxAllocator.allocate(1*sizeof(uint64_t));
				message[0]=(uint64_t)m_EXTENSION_currentPointer;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,m_EXTENSION_currentRank,TAG_MARK_AS_ASSEMBLED,getRank());
				m_outbox.push_back(aMessage);
				m_EXTENSION_VertexMarkAssembled_received=false;
			}else if(m_EXTENSION_VertexMarkAssembled_received){
				m_EXTENSION_reverseVertexDone=true;
				m_EXTENSION_enumerateChoices=false;
			}
		}
	}
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
