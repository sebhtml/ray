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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#include<assert.h>
#include<Read.h>
#include<MessageProcessor.h>
#include<common_functions.h>
#include<ReadAnnotation.h>
#include<SplayTree.h>
#include<Direction.h>
#include<SplayNode.h>
#include<MyForest.h>
#include<SplayTreeIterator.h>
#include<FusionData.h>
#include<Parameters.h>

void MessageProcessor::processMessage(Message*message){
	int tag=message->getTag();
	FNMETHOD f=m_methods[tag];
	(this->*f)(message);
}


void MessageProcessor::call_TAG_WELCOME(Message*message){
}

void MessageProcessor::call_TAG_SEND_SEQUENCE(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	int length=count;
	char*incoming=(char*)(*m_inboxAllocator).allocate(count*sizeof(char)+1);
	for(int i=0;i<(int)length;i++)
		incoming[i]=((char*)buffer)[i];

	incoming[length]='\0';
	Read*myRead=(Read*)(*m_persistentAllocator).allocate(sizeof(Read));
	myRead->copy(NULL,incoming,&(*m_persistentAllocator));
	(*m_myReads).push_back(myRead);
	#ifdef SHOW_PROGRESS
	if((*m_myReads).size()%100000==0){
		cout<<"Rank "<<rank<<" has "<<(*m_myReads).size()<<" sequences"<<endl;
	}
	#endif
}

void MessageProcessor::call_TAG_SEQUENCES_READY(Message*message){
	(*m_sequence_ready_machines)++;
}

void MessageProcessor::call_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS(Message*message){
	int source=message->getSource();
	#ifdef SHOW_PROGRESS
	cout<<"Rank "<<rank<<" has "<<(*m_myReads).size()<<" sequences"<<endl;
	#endif
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,TAG_SEQUENCES_READY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_VERTICES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int length=count;
	for(int i=0;i<length;i++){
		VERTEX_TYPE l=incoming[i];

		#ifdef SHOW_PROGRESS
		if((*m_last_value)!=(int)m_subgraph->size() and (int)m_subgraph->size()%100000==0){
			(*m_last_value)=m_subgraph->size();
			cout<<"Rank "<<rank<<" has "<<m_subgraph->size()<<" vertices "<<endl;
		}
		#endif
		SplayNode<VERTEX_TYPE,Vertex>*tmp=m_subgraph->insert(l);
		#ifdef DEBUG
		assert(tmp!=NULL);
		#endif
		if(m_subgraph->inserted()){
			tmp->getValue()->constructor(); 
		}
		tmp->getValue()->setCoverage(tmp->getValue()->getCoverage()+1);
		#ifdef DEBUG
		assert(tmp->getValue()->getCoverage()>0);
		#endif
	}
}

void MessageProcessor::call_TAG_VERTICES_DISTRIBUTED(Message*message){
	(*m_numberOfMachinesDoneSendingVertices)++;
}

void MessageProcessor::call_TAG_VERTEX_PTR_REQUEST(Message*message){
}
void MessageProcessor::call_TAG_OUT_EDGE_DATA_WITH_PTR(Message*message){
}

void MessageProcessor::call_TAG_OUT_EDGES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int length=count;

	for(int i=0;i<(int)length;i+=2){
		VERTEX_TYPE prefix=incoming[i+0];
		VERTEX_TYPE suffix=incoming[i+1];
		#ifdef DEBUG
		assert(m_subgraph->find(prefix)!=NULL);
		#endif
		m_subgraph->find(prefix)->getValue()->addOutgoingEdge(suffix,(*m_wordSize),&(*m_persistentAllocator));
		#ifdef DEBUG
		vector<VERTEX_TYPE> newEdges=m_subgraph->find(prefix)->getValue()->getOutgoingEdges(prefix,(*m_wordSize));
		bool found=false;
		for(int i=0;i<(int)newEdges.size();i++){
			if(newEdges[i]==suffix)
				found=true;
		}
		if(newEdges.size()==0){
			cout<<"prefix,suffix"<<endl;
			coutBIN(prefix);
			coutBIN(suffix);
			cout<<idToWord(prefix,(*m_wordSize))<<endl;
			cout<<idToWord(suffix,(*m_wordSize))<<endl;
		}
		assert(newEdges.size()>0);
		if(!found){
			cout<<"prefix,suffix"<<endl;
			coutBIN(prefix);
			coutBIN(suffix);
			cout<<idToWord(prefix,(*m_wordSize))<<endl;
			cout<<idToWord(suffix,(*m_wordSize))<<endl;

			cout<<" ARC"<<endl;
			coutBIN(prefix);
			coutBIN(suffix);
			cout<<"EDges."<<endl;
			for(int i=0;i<(int)newEdges.size();i++){
				coutBIN(newEdges[i]);
			}

		}
		assert(found);
		#endif
	}
}

void MessageProcessor::call_TAG_SHOW_VERTICES(Message*message){
}

void MessageProcessor::call_TAG_START_VERTICES_DISTRIBUTION(Message*message){
	// wait for everyone
	MPI_Barrier(MPI_COMM_WORLD);
	(*m_mode_send_vertices)=true;
	(*m_mode_send_vertices_sequence_id)=0;
}

void MessageProcessor::call_TAG_EDGES_DISTRIBUTED(Message*message){
	(*m_numberOfMachinesDoneSendingEdges)++;
}

void MessageProcessor::call_TAG_IN_EDGES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int length=count;

	for(int i=0;i<(int)length;i+=2){
		VERTEX_TYPE prefix=incoming[i+0];
		VERTEX_TYPE suffix=incoming[i+1];
		m_subgraph->find(suffix)->getValue()->addIngoingEdge(prefix,(*m_wordSize),&(*m_persistentAllocator));
		#ifdef DEBUG
		bool found=false;
		vector<VERTEX_TYPE> edges=m_subgraph->find(suffix)->getValue()->getIngoingEdges(suffix,(*m_wordSize));
		for(int i=0;i<(int)edges.size();i++){
			if(edges[i]==prefix)
				found=true;
		}
		assert(found);
		#endif
	}
}

void MessageProcessor::call_TAG_IN_EDGE_DATA_WITH_PTR(Message*message){
}

void MessageProcessor::call_TAG_START_EDGES_DISTRIBUTION(Message*message){
	(*m_mode_send_outgoing_edges)=true;
}

void MessageProcessor::call_TAG_START_EDGES_DISTRIBUTION_ASK(Message*message){
	int source=message->getSource();
	Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, source, TAG_START_EDGES_DISTRIBUTION_ANSWER,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_START_EDGES_DISTRIBUTION_ANSWER(Message*message){
	(*m_numberOfMachinesReadyForEdgesDistribution)++;
}

void MessageProcessor::call_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message){
	int source=message->getSource();
	cout<<"Rank "<<rank<<" has "<<m_subgraph->size()<<" vertices"<<endl;
	Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, source, TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER(Message*message){
	(*m_numberOfMachinesReadyToSendDistribution)++;
}

void MessageProcessor::call_TAG_PREPARE_COVERAGE_DISTRIBUTION(Message*message){
	(*m_mode_send_coverage_iterator)=0;
	(*m_mode_sendDistribution)=true;
}

void MessageProcessor::call_TAG_COVERAGE_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int length=count;

	for(int i=0;i<length;i+=2){
		int coverage=incoming[i+0];
		VERTEX_TYPE count=incoming[i+1];
		(*m_coverageDistribution)[coverage]+=count;
	}
}

void MessageProcessor::call_TAG_COVERAGE_END(Message*message){
	(*m_numberOfMachinesDoneSendingCoverage)++;
}

void MessageProcessor::call_TAG_SEND_COVERAGE_VALUES(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_minimumCoverage)=incoming[0];
	(*m_seedCoverage)=incoming[1];
	(*m_peakCoverage)=incoming[2];
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_RECEIVED_COVERAGE_INFORMATION,rank);
	m_outbox->push_back(aMessage);
	m_oa->constructor((*m_peakCoverage));
}

void MessageProcessor::call_TAG_READY_TO_SEED(Message*message){
	(*m_readyToSeed)++;
}

void MessageProcessor::call_TAG_START_SEEDING(Message*message){
	(*m_mode)=MODE_START_SEEDING;
	map<int,map<int,int> > edgesDistribution;
	
	for(int i=0;i<m_subgraph->getNumberOfTrees();i++){
		SplayTreeIterator<VERTEX_TYPE,Vertex> seedingIterator(m_subgraph->getTree(i));
		while(seedingIterator.hasNext()){
			SplayNode<VERTEX_TYPE,Vertex>*node=seedingIterator.next();
			edgesDistribution[node->getValue()->getIngoingEdges(node->getKey(),(*m_wordSize)).size()][node->getValue()->getOutgoingEdges(node->getKey(),(*m_wordSize)).size()]++;
			(*m_SEEDING_nodes).push_back(node->getKey());
		}
	}
	#ifdef DEBUG
	//cout<<"Ingoing and outgoing edges."<<endl;
	for(map<int,map<int,int> >::iterator i=edgesDistribution.begin();i!=edgesDistribution.end();++i){
		for(map<int,int>::iterator j=i->second.begin();j!=i->second.end();++j){
			//cout<<i->first<<" "<<j->first<<" "<<j->second<<endl;
		}
	}
	#endif
	(*m_SEEDING_NodeInitiated)=false;
	(*m_SEEDING_i)=0;
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_COVERAGE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef DEBUG
	assert(node!=NULL);
	#endif
	VERTEX_TYPE coverage=node->getValue()->getCoverage();
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
	message2[0]=coverage;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_COVERAGE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_COVERAGE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_SEEDING_receivedVertexCoverage)=incoming[0];
	(*m_SEEDING_vertexCoverageReceived)=true;
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	SplayNode<VERTEX_TYPE,Vertex>*node=(SplayNode<VERTEX_TYPE,Vertex>*)incoming[0];
	VERTEX_TYPE key=node->getKey();
	VERTEX_TYPE coverage=node->getValue()->getCoverage();
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(2*sizeof(VERTEX_TYPE));
	message2[0]=key;
	message2[1]=coverage;
	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_SEEDING_receivedKey)=incoming[0];
	(*m_SEEDING_receivedVertexCoverage)=incoming[1];
	(*m_SEEDING_vertexKeyAndCoverageReceived)=true;
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_OUTGOING_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	vector<VERTEX_TYPE> outgoingEdges=m_subgraph->find(incoming[0])->getValue()->getOutgoingEdges(incoming[0],*m_wordSize);
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(outgoingEdges.size()*sizeof(VERTEX_TYPE));
	for(int i=0;i<(int)outgoingEdges.size();i++){
		message2[i]=outgoingEdges[i];
	}
	Message aMessage(message2,outgoingEdges.size(),MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_SEEDING_receivedOutgoingEdges).clear();
	for(int i=0;i<count;i++){
		(*m_SEEDING_receivedOutgoingEdges).push_back(incoming[i]);
	}
	(*m_SEEDING_edgesReceived)=true;
}

void MessageProcessor::call_TAG_SEEDING_IS_OVER(Message*message){
	(*m_numberOfRanksDoneSeeding)++;
}

void MessageProcessor::call_TAG_GOOD_JOB_SEE_YOU_SOON(Message*message){
	(*m_alive)=false;
}

void MessageProcessor::call_TAG_I_GO_NOW(Message*message){
}

void MessageProcessor::call_TAG_SET_WORD_SIZE(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_wordSize)=incoming[0];
}

void MessageProcessor::call_TAG_MASTER_IS_DONE_ATTACHING_READS(Message*message){
	int source=message->getSource();
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY(Message*message){
	(*m_ranksDoneAttachingReads)++;
}

void MessageProcessor::call_TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER(Message*message){
}
void MessageProcessor::call_TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY(Message*message){
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_INGOING_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef DEBUG
	assert(node!=NULL);
	#endif
	vector<VERTEX_TYPE> ingoingEdges=node->getValue()->getIngoingEdges(incoming[0],*m_wordSize);
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(ingoingEdges.size()*sizeof(VERTEX_TYPE));
	for(int i=0;i<(int)ingoingEdges.size();i++){
		message2[i]=ingoingEdges[i];
	}
	Message aMessage(message2,ingoingEdges.size(),MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_SEEDING_receivedIngoingEdges).clear();
	for(int i=0;i<count;i++){
		(*m_SEEDING_receivedIngoingEdges).push_back(incoming[i]);
	}
	(*m_SEEDING_InedgesReceived)=true;
}

void MessageProcessor::call_TAG_EXTENSION_IS_DONE(Message*message){
	(*m_EXTENSION_numberOfRanksDone)++;
	(*m_EXTENSION_currentRankIsDone)=true;
}

void MessageProcessor::call_TAG_ASK_EXTENSION(Message*message){
	(*m_EXTENSION_initiated)=false;
	(*m_mode_EXTENSION)=true;
	(*m_last_value)=-1;
}

void MessageProcessor::call_TAG_ASK_IS_ASSEMBLED(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef DEBUG
	assert(node!=NULL);
	#endif
	bool isAssembled=node->getValue()->isAssembled();
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
	message2[0]=isAssembled;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_IS_ASSEMBLED_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_ASK_REVERSE_COMPLEMENT(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	SplayNode<VERTEX_TYPE,Vertex>*node=(SplayNode<VERTEX_TYPE,Vertex>*)incoming[0];
	VERTEX_TYPE value=node->getKey();
	VERTEX_TYPE reverseComplement=complementVertex(value,*m_wordSize,(*m_colorSpaceMode));
	int rank=vertexRank(reverseComplement,size);
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(2*sizeof(VERTEX_TYPE));
	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,rank,TAG_REQUEST_VERTEX_POINTER,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_REQUEST_VERTEX_POINTER(Message*message){
}

void MessageProcessor::call_TAG_ASK_IS_ASSEMBLED_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_EXTENSION_VertexAssembled_received)=true;
	(*m_EXTENSION_vertexIsAssembledResult)=(bool)incoming[0];
}

void MessageProcessor::call_TAG_MARK_AS_ASSEMBLED(Message*message){
}

void MessageProcessor::call_TAG_ASK_EXTENSION_DATA(Message*message){
	(*m_mode)=MODE_SEND_EXTENSION_DATA;
	(*m_SEEDING_i)=0;
	(*m_EXTENSION_currentPosition)=0;
}

void MessageProcessor::call_TAG_EXTENSION_DATA(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_allPaths)[(*m_allPaths).size()-1].push_back(incoming[0]);
}

void MessageProcessor::call_TAG_EXTENSION_END(Message*message){
}

void MessageProcessor::call_TAG_EXTENSION_DATA_END(Message*message){
	(*m_EXTENSION_currentRankIsDone)=true;
}

void MessageProcessor::call_TAG_ATTACH_SEQUENCE(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	for(int i=0;i<count;i+=4){
		VERTEX_TYPE vertex=incoming[i+0];
		int rank=incoming[i+1];
		int sequenceIdOnDestination=(int)incoming[i+2];
		char strand=(char)incoming[i+3];
		#ifdef DEBUG
		assert(m_subgraph->find(vertex)!=NULL);
		#endif
		m_subgraph->find(vertex)->getValue()->addRead(rank,sequenceIdOnDestination,strand,&(*m_persistentAllocator));
	}
}

void MessageProcessor::call_TAG_REQUEST_READS(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef DEBUG
	assert(node!=NULL);
	#endif
	Vertex*theVertex=node->getValue();
	#ifdef DEBUG
	assert(theVertex!=NULL);
	#endif
	ReadAnnotation*e=theVertex->getReads();
	int maxToProcess=MPI_BTL_SM_EAGER_LIMIT/sizeof(VERTEX_TYPE)-3;
	maxToProcess=maxToProcess-maxToProcess%3;
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(4096);
	int j=0;
	// send a maximum of maxToProcess individually

	// pad the message with a sentinel value  sentinel/0/sentinel
	message2[j++]=m_sentinelValue;
	message2[j++]=0;
	message2[j++]=m_sentinelValue;
	if(e==NULL){
		// end is sentinel/sentinel/sentinel
		message2[j++]=m_sentinelValue;
		message2[j++]=m_sentinelValue;
		message2[j++]=m_sentinelValue;

		Message aMessage(message2,j,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_READS_REPLY,rank);
		m_outbox->push_back(aMessage);
	}
	while(e!=NULL){
		message2[j++]=e->getRank();
		message2[j++]=e->getReadIndex();
		message2[j++]=e->getStrand();
		e=e->getNext();
		// if we reached the maximum of nothing is to be processed after
		if(j==maxToProcess || e==NULL){
			// pop the message on the MPI collective
			if(e==NULL){
				// end is sentinel/sentinel/sentinel
				message2[j++]=m_sentinelValue;
				message2[j++]=m_sentinelValue;
				message2[j++]=m_sentinelValue;
			}
			Message aMessage(message2,j,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_READS_REPLY,rank);
			m_outbox->push_back(aMessage);
			// if more reads are to be sent
			if(e!=NULL){
				//allocate another chunk
				message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(4096);
				j=0;
			}
		}
	}
}

void MessageProcessor::call_TAG_REQUEST_READS_REPLY(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	for(int i=0;i<count;i+=3){
		// beginning of transmission, s,0,s
		if(incoming[i]==m_sentinelValue 
		&& incoming[i+1]==0
		&& incoming[i+2]==m_sentinelValue){
			m_EXTENSION_receivedReads->clear();
		// end of transmission, s,s,s
		}else if(incoming[i]==m_sentinelValue 
		&& incoming[i+1]==m_sentinelValue
		&& incoming[i+2]==m_sentinelValue){
			(*m_EXTENSION_reads_received)=true;
		}else{
			int rank=incoming[i];
			int index=incoming[i+1];
			char strand=incoming[i+2];
			ReadAnnotation e;
			e.constructor(rank,index,strand);
			m_EXTENSION_receivedReads->push_back(e);
		}
	}
}

void MessageProcessor::call_TAG_ASK_READ_VERTEX_AT_POSITION(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	char strand=incoming[2];
	VERTEX_TYPE vertex=(*m_myReads)[incoming[0]]->Vertex(incoming[1],(*m_wordSize),strand,(*m_colorSpaceMode));
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
	message2[0]=vertex;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_READ_VERTEX_AT_POSITION_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY(Message*message){
	void*buffer=message->getBuffer();
	(*m_EXTENSION_read_vertex_received)=true;
	(*m_EXTENSION_receivedReadVertex)=((VERTEX_TYPE*)buffer)[0];
}

void MessageProcessor::call_TAG_ASK_READ_LENGTH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int length=(*m_myReads)[incoming[0]]->length();
	
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
	message2[0]=length;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_READ_LENGTH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_ASK_READ_LENGTH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_EXTENSION_readLength_received)=true;
	(*m_EXTENSION_receivedLength)=incoming[0];
}

void MessageProcessor::call_TAG_SAVE_WAVE_PROGRESSION(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef DEBUG
	assert(node!=NULL);
	#endif
	int wave=incoming[1];
	int progression=incoming[2];
	node->getValue()->addDirection(wave,progression,&(*m_directionsAllocator));
}

void MessageProcessor::call_TAG_COPY_DIRECTIONS(Message*message){
	(*m_mode)=MODE_COPY_DIRECTIONS;
	for(int i=0;i<m_subgraph->getNumberOfTrees();i++){
		SplayTreeIterator<VERTEX_TYPE,Vertex> seedingIterator(m_subgraph->getTree(i));
		while(seedingIterator.hasNext()){
			SplayNode<VERTEX_TYPE,Vertex>*node=seedingIterator.next();
			(*m_SEEDING_nodes).push_back(node->getKey());
		}
	}
	(*m_SEEDING_i)=0;
}

void MessageProcessor::call_TAG_ASSEMBLE_WAVES(Message*message){
	(*m_mode)=MODE_ASSEMBLE_WAVES;
	(*m_SEEDING_i)=0;
}

void MessageProcessor::call_TAG_SAVE_WAVE_PROGRESSION_REVERSE(Message*message){
}

void MessageProcessor::call_TAG_ASSEMBLE_WAVES_DONE(Message*message){
	(*m_EXTENSION_currentRankIsDone)=true;
}

void MessageProcessor::call_TAG_START_FUSION(Message*message){
	(*m_mode)=MODE_FUSION;
	(*m_SEEDING_i)=0;

	m_fusionData->m_FUSION_direct_fusionDone=false;
	m_fusionData->m_FUSION_first_done=false;
	m_fusionData->m_FUSION_paths_requested=false;
}

void MessageProcessor::call_TAG_FUSION_DONE(Message*message){
	m_fusionData->m_FUSION_numberOfRanksDone++;
}

void MessageProcessor::call_TAG_ASK_VERTEX_PATHS_SIZE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	SplayNode<VERTEX_TYPE,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef DEBUG
	assert(node!=NULL);
	#endif
	vector<Direction> paths=node->getValue()->getDirections();
	m_fusionData->m_FUSION_cachedDirections[source]=paths;
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
	message2[0]=paths.size();
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_VERTEX_PATHS_SIZE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_ASK_VERTEX_PATHS_SIZE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	m_fusionData->m_FUSION_paths_received=true;
	m_fusionData->m_FUSION_receivedPaths.clear();
	m_fusionData->m_FUSION_numberOfPaths=incoming[0];
}

void MessageProcessor::call_TAG_GET_PATH_LENGTH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int id=incoming[0];
	int length=0;
	#ifdef DEBUG
	assert(m_fusionData->m_FUSION_identifier_map.count(id)>0);
	#endif
	if(m_fusionData->m_FUSION_identifier_map.count(id)>0){
		length=(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size();
	}

	#ifdef DEBUG
	assert(length>0);
	#endif
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(sizeof(VERTEX_TYPE));
	message2[0]=length;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,TAG_GET_PATH_LENGTH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_GET_PATH_LENGTH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	m_fusionData->m_FUSION_receivedLength=incoming[0];
	m_fusionData->m_FUSION_pathLengthReceived=true;
}

void MessageProcessor::call_TAG_CALIBRATION_MESSAGE(Message*message){
}
void MessageProcessor::call_TAG_BEGIN_CALIBRATION(Message*message){
}
void MessageProcessor::call_TAG_END_CALIBRATION(Message*message){
}
void MessageProcessor::call_TAG_COMMUNICATION_STABILITY_MESSAGE(Message*message){
}

void MessageProcessor::call_TAG_ASK_VERTEX_PATH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int i=incoming[0];
	Direction d=m_fusionData->m_FUSION_cachedDirections[source][i];
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(2*sizeof(VERTEX_TYPE));
	message2[0]=d.getWave();
	message2[1]=d.getProgression();
	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,source,TAG_ASK_VERTEX_PATH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_ASK_VERTEX_PATH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	m_fusionData->m_FUSION_path_received=true;
	int pathId=incoming[0];
	int position=incoming[1];
	m_fusionData->m_FUSION_receivedPath.constructor(pathId,position);
}

void MessageProcessor::call_TAG_INDEX_PAIRED_SEQUENCE(Message*message){
	int count=message->getCount();
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	for(int i=0;i<count;i+=5){
		PairedRead*t=(PairedRead*)(*m_persistentAllocator).allocate(sizeof(PairedRead));
		int length=incoming[i+3];
		int deviation=incoming[i+4];

		t->constructor(incoming[i+1],incoming[i+2],length,deviation);
		(*m_myReads)[incoming[i+0]]->setPairedRead(t);
	}
}

void MessageProcessor::call_TAG_HAS_PAIRED_READ(Message*message){
	int source=message->getSource();
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int index=incoming[0];
	#ifdef DEBUG
	assert(index<(int)m_myReads->size());
	#endif
	message2[0]=(*m_myReads)[index]->hasPairedRead();
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,TAG_HAS_PAIRED_READ_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_HAS_PAIRED_READ_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_EXTENSION_hasPairedReadAnswer)=incoming[0];
	(*m_EXTENSION_hasPairedReadReceived)=true;
}

void MessageProcessor::call_TAG_GET_PAIRED_READ(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int index=incoming[0];
	#ifdef DEBUG
	assert(index<(int)m_myReads->size());
	#endif
	PairedRead*t=(*m_myReads)[index]->getPairedRead();
	PairedRead dummy;
	dummy.constructor(0,0,0,0);
	if(t==NULL){
		t=&dummy;
	}
	#ifdef DEBUG
	assert(t!=NULL);
	#endif
	VERTEX_TYPE*message2=(VERTEX_TYPE*)m_outboxAllocator->allocate(4*sizeof(VERTEX_TYPE));
	message2[0]=t->getRank();
	message2[1]=t->getId();
	message2[2]=t->getAverageFragmentLength();
	message2[3]=t->getStandardDeviation();
	Message aMessage(message2,4,MPI_UNSIGNED_LONG_LONG,source,TAG_GET_PAIRED_READ_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_GET_PAIRED_READ_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_EXTENSION_pairedRead).constructor(incoming[0],incoming[1],incoming[2],incoming[3]);
	(*m_EXTENSION_pairedSequenceReceived)=true;
}

void MessageProcessor::call_TAG_CLEAR_DIRECTIONS(Message*message){
	int source=message->getSource();
	// clearing old data too!.
	(*m_FINISH_pathLengths).clear();

	cout<<"Rank "<<rank<<" is clearing its directions"<<endl;
	// clear graph
	for(int i=0;i<m_subgraph->getNumberOfTrees();i++){
		SplayTreeIterator<VERTEX_TYPE,Vertex> iterator(m_subgraph->getTree(i));
		while(iterator.hasNext()){
			iterator.next()->getValue()->clearDirections();
		}
	}
	(*m_directionsAllocator).clear();
	(*m_directionsAllocator).constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);


	// add the FINISHING bits
	for(int i=0;i<(int)(*m_FINISH_newFusions).size();i++){
		#ifdef SHOW_PROGRESS
		#endif
		(*m_EXTENSION_contigs).push_back((*m_FINISH_newFusions)[i]);
	}

	(*m_FINISH_newFusions).clear();


	cout<<"Rank "<<rank<<" keeps unique fusions."<<endl;
	vector<vector<VERTEX_TYPE> > fusions;
	for(int i=0;i<(int)(*m_EXTENSION_contigs).size();i++){
		int id=(*m_EXTENSION_identifiers)[i];
		if(m_fusionData->m_FUSION_eliminated.count(id)==0){
			fusions.push_back((*m_EXTENSION_contigs)[i]);
			vector<VERTEX_TYPE> rc;
			for(int j=(*m_EXTENSION_contigs)[i].size()-1;j>=0;j--){
				rc.push_back(complementVertex((*m_EXTENSION_contigs)[i][j],*m_wordSize,(*m_colorSpaceMode)));
			}
			fusions.push_back(rc);
		}
	}

	cout<<"Rank "<<rank<<" renames its fusions."<<endl;
	(*m_EXTENSION_identifiers).clear();
	m_fusionData->m_FUSION_eliminated.clear();
	for(int i=0;i<(int)fusions.size();i++){
		int id=i*MAX_NUMBER_OF_MPI_PROCESSES+rank;
		#ifdef DEBUG
		assert(id%MAX_NUMBER_OF_MPI_PROCESSES<size);
		#endif
		(*m_EXTENSION_identifiers).push_back(id);
	}

	for(int i=0;i<(int)(*m_EXTENSION_identifiers).size();i++){
		int id=(*m_EXTENSION_identifiers)[i];
		m_fusionData->m_FUSION_identifier_map[id]=i;
	}

	(*m_EXTENSION_contigs).clear();
	(*m_EXTENSION_contigs)=fusions;

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,TAG_CLEAR_DIRECTIONS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_CLEAR_DIRECTIONS_REPLY(Message*message){
	(*m_CLEAR_n)++;
}

void MessageProcessor::call_TAG_FINISH_FUSIONS(Message*message){
	(*m_mode)=MODE_FINISH_FUSIONS;
	(*m_FINISH_fusionOccured)=false;
	(*m_SEEDING_i)=0;
	(*m_EXTENSION_currentPosition)=0;
	m_fusionData->m_FUSION_first_done=false;
	(*m_Machine_getPaths_INITIALIZED)=false;
	(*m_Machine_getPaths_DONE)=false;
}

void MessageProcessor::call_TAG_FINISH_FUSIONS_FINISHED(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_FINISH_n)++;
	if(incoming[0]){
		(*m_nextReductionOccured)=incoming[0];
	}
}

void MessageProcessor::call_TAG_DISTRIBUTE_FUSIONS(Message*message){
	(*m_mode)=MODE_DISTRIBUTE_FUSIONS;
	(*m_SEEDING_i)=0;
	(*m_EXTENSION_currentPosition)=0;
}

void MessageProcessor::call_TAG_DISTRIBUTE_FUSIONS_FINISHED(Message*message){
	(*m_DISTRIBUTE_n)++;
}

void MessageProcessor::call_TAG_EXTENSION_START(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	vector<VERTEX_TYPE> a;
	(*m_allPaths).push_back(a);
	int id=incoming[0];
	#ifdef DEBUG
	int rank=id%MAX_NUMBER_OF_MPI_PROCESSES;
	assert(rank<size);
	#endif
	(*m_identifiers).push_back(id);
	(*m_allIdentifiers)[id]=m_identifiers->size()-1;

}

void MessageProcessor::call_TAG_ELIMINATE_PATH(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	m_fusionData->m_FUSION_eliminated.insert(incoming[0]);
}

void MessageProcessor::call_TAG_GET_PATH_VERTEX(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int id=incoming[0];
	int position=incoming[1];
	#ifdef DEBUG
	assert(m_fusionData->m_FUSION_identifier_map.count(id)>0);
	#endif
	#ifdef DEBUG
	if(position>=(int)(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size()){
		cout<<"Pos="<<position<<" Length="<<(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size()<<endl;
	}
	assert(position<(int)(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size());
	#endif
	VERTEX_TYPE*messageBytes=(VERTEX_TYPE*)m_outboxAllocator->allocate(sizeof(VERTEX_TYPE));
	messageBytes[0]=(*m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]][position];
	Message aMessage(messageBytes,1,MPI_UNSIGNED_LONG_LONG,source,TAG_GET_PATH_VERTEX_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_GET_PATH_VERTEX_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_FINISH_vertex_received)=true;
	(*m_FINISH_received_vertex)=incoming[0];
}

void MessageProcessor::call_TAG_SET_COLOR_MODE(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_colorSpaceMode)=incoming[0];
}

void MessageProcessor::call_TAG_AUTOMATIC_DISTANCE_DETECTION(Message*message){
	(*m_mode)=MODE_AUTOMATIC_DISTANCE_DETECTION;
	(*m_SEEDING_i)=0;
	(*m_EXTENSION_currentPosition)=0;
	ed->m_EXTENSION_reads_requested=false;
}

void MessageProcessor::call_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE(Message*message){
	(*m_numberOfRanksDoneDetectingDistances)++;
}

void MessageProcessor::call_TAG_LIBRARY_DISTANCE(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	for(int i=0;i<count;i+=3){
		parameters->addDistance(incoming[i+0],incoming[i+1],incoming[i+2]);
	}
}

void MessageProcessor::call_TAG_ASK_LIBRARY_DISTANCES(Message*message){
	(*m_mode)=MODE_SEND_LIBRARY_DISTANCES;
	(*m_libraryIterator)=0;
	(*m_libraryIndexInitiated)=false;
}

void MessageProcessor::call_TAG_ASK_LIBRARY_DISTANCES_FINISHED(Message*message){
	(*m_numberOfRanksDoneSendingDistances)++;
}

void MessageProcessor::call_TAG_UPDATE_LIBRARY_INFORMATION(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	for(int i=0;i<count;i+=3){
		#ifdef DEBUG
		assert((*m_myReads)[incoming[i+0]]->hasPairedRead());
		#endif
		(*m_myReads)[incoming[i+0]]->getPairedRead()->updateLibrary(incoming[i+1],incoming[i+2]);
	}
}

void MessageProcessor::call_TAG_RECEIVED_COVERAGE_INFORMATION(Message*message){
	(*m_numberOfRanksWithCoverageData)++;
}

void MessageProcessor::call_TAG_REQUEST_READ_SEQUENCE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	int index=incoming[0];
	#ifdef DEBUG
	assert(index<(int)m_myReads->size());
	#endif
	PairedRead*t=(*m_myReads)[index]->getPairedRead();
	PairedRead dummy;
	dummy.constructor(0,0,0,0);
	if(t==NULL){
		t=&dummy;
	}
	#ifdef DEBUG
	assert(t!=NULL);
	#endif
	char*seq=m_myReads->at(index)->getSeq();

	int toAllocate=roundNumber(4*sizeof(VERTEX_TYPE)+strlen(seq)+1,8);

	VERTEX_TYPE*messageBytes=(VERTEX_TYPE*)m_outboxAllocator->allocate(toAllocate);
	messageBytes[0]=t->getRank();
	messageBytes[1]=t->getId();
	messageBytes[2]=t->getAverageFragmentLength();
	messageBytes[3]=t->getStandardDeviation();
	char*dest=(char*)(messageBytes+4);
	strcpy(dest,seq);
	Message aMessage(messageBytes,toAllocate,MPI_UNSIGNED_LONG_LONG,source,TAG_REQUEST_READ_SEQUENCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_TAG_REQUEST_READ_SEQUENCE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	VERTEX_TYPE*incoming=(VERTEX_TYPE*)buffer;
	(*m_EXTENSION_pairedRead).constructor(incoming[0],incoming[1],incoming[2],incoming[3]);
	(*m_EXTENSION_pairedSequenceReceived)=true;

	seedExtender->m_receivedString=(char*)(incoming+4);
	seedExtender->m_sequenceReceived=true;
}

MessageProcessor::MessageProcessor(){
	m_sentinelValue=0;
	m_sentinelValue--;// overflow it in an obvious manner
	
	
	m_methods[TAG_WELCOME]=&MessageProcessor::call_TAG_WELCOME;
	m_methods[TAG_SEND_SEQUENCE]=&MessageProcessor::call_TAG_SEND_SEQUENCE;
	m_methods[TAG_SEQUENCES_READY]=&MessageProcessor::call_TAG_SEQUENCES_READY;
	m_methods[TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS]=&MessageProcessor::call_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS;
	m_methods[TAG_VERTICES_DATA]=&MessageProcessor::call_TAG_VERTICES_DATA;
	m_methods[TAG_VERTICES_DISTRIBUTED]=&MessageProcessor::call_TAG_VERTICES_DISTRIBUTED;
	m_methods[TAG_VERTEX_PTR_REQUEST]=&MessageProcessor::call_TAG_VERTEX_PTR_REQUEST;
	m_methods[TAG_OUT_EDGE_DATA_WITH_PTR]=&MessageProcessor::call_TAG_OUT_EDGE_DATA_WITH_PTR;
	m_methods[TAG_OUT_EDGES_DATA]=&MessageProcessor::call_TAG_OUT_EDGES_DATA;
	m_methods[TAG_SHOW_VERTICES]=&MessageProcessor::call_TAG_SHOW_VERTICES;
	m_methods[TAG_START_VERTICES_DISTRIBUTION]=&MessageProcessor::call_TAG_START_VERTICES_DISTRIBUTION;
	m_methods[TAG_EDGES_DISTRIBUTED]=&MessageProcessor::call_TAG_EDGES_DISTRIBUTED;
	m_methods[TAG_IN_EDGES_DATA]=&MessageProcessor::call_TAG_IN_EDGES_DATA;
	m_methods[TAG_IN_EDGE_DATA_WITH_PTR]=&MessageProcessor::call_TAG_IN_EDGE_DATA_WITH_PTR;
	m_methods[TAG_START_EDGES_DISTRIBUTION]=&MessageProcessor::call_TAG_START_EDGES_DISTRIBUTION;
	m_methods[TAG_START_EDGES_DISTRIBUTION_ASK]=&MessageProcessor::call_TAG_START_EDGES_DISTRIBUTION_ASK;
	m_methods[TAG_START_EDGES_DISTRIBUTION_ANSWER]=&MessageProcessor::call_TAG_START_EDGES_DISTRIBUTION_ANSWER;
	m_methods[TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION]=&MessageProcessor::call_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION;
	m_methods[TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER]=&MessageProcessor::call_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER;
	m_methods[TAG_PREPARE_COVERAGE_DISTRIBUTION]=&MessageProcessor::call_TAG_PREPARE_COVERAGE_DISTRIBUTION;
	m_methods[TAG_COVERAGE_DATA]=&MessageProcessor::call_TAG_COVERAGE_DATA;
	m_methods[TAG_COVERAGE_END]=&MessageProcessor::call_TAG_COVERAGE_END;
	m_methods[TAG_SEND_COVERAGE_VALUES]=&MessageProcessor::call_TAG_SEND_COVERAGE_VALUES;
	m_methods[TAG_READY_TO_SEED]=&MessageProcessor::call_TAG_READY_TO_SEED;
	m_methods[TAG_START_SEEDING]=&MessageProcessor::call_TAG_START_SEEDING;
	m_methods[TAG_REQUEST_VERTEX_COVERAGE]=&MessageProcessor::call_TAG_REQUEST_VERTEX_COVERAGE;
	m_methods[TAG_REQUEST_VERTEX_COVERAGE_REPLY]=&MessageProcessor::call_TAG_REQUEST_VERTEX_COVERAGE_REPLY;
	m_methods[TAG_REQUEST_VERTEX_KEY_AND_COVERAGE]=&MessageProcessor::call_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE;
	m_methods[TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY]=&MessageProcessor::call_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY;
	m_methods[TAG_REQUEST_VERTEX_OUTGOING_EDGES]=&MessageProcessor::call_TAG_REQUEST_VERTEX_OUTGOING_EDGES;
	m_methods[TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY]=&MessageProcessor::call_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY;
	m_methods[TAG_SEEDING_IS_OVER]=&MessageProcessor::call_TAG_SEEDING_IS_OVER;
	m_methods[TAG_GOOD_JOB_SEE_YOU_SOON]=&MessageProcessor::call_TAG_GOOD_JOB_SEE_YOU_SOON;
	m_methods[TAG_I_GO_NOW]=&MessageProcessor::call_TAG_I_GO_NOW;
	m_methods[TAG_SET_WORD_SIZE]=&MessageProcessor::call_TAG_SET_WORD_SIZE;
	m_methods[TAG_MASTER_IS_DONE_ATTACHING_READS]=&MessageProcessor::call_TAG_MASTER_IS_DONE_ATTACHING_READS;
	m_methods[TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY]=&MessageProcessor::call_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY;
	m_methods[TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER]=&MessageProcessor::call_TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER;
	m_methods[TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY]=&MessageProcessor::call_TAG_FORWARD_TO_ATTACH_SEQUENCE_POINTER_REPLY;
	m_methods[TAG_REQUEST_VERTEX_INGOING_EDGES]=&MessageProcessor::call_TAG_REQUEST_VERTEX_INGOING_EDGES;
	m_methods[TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY]=&MessageProcessor::call_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY;
	m_methods[TAG_EXTENSION_IS_DONE]=&MessageProcessor::call_TAG_EXTENSION_IS_DONE;
	m_methods[TAG_ASK_EXTENSION]=&MessageProcessor::call_TAG_ASK_EXTENSION;
	m_methods[TAG_ASK_IS_ASSEMBLED]=&MessageProcessor::call_TAG_ASK_IS_ASSEMBLED;
	m_methods[TAG_ASK_REVERSE_COMPLEMENT]=&MessageProcessor::call_TAG_ASK_REVERSE_COMPLEMENT;
	m_methods[TAG_REQUEST_VERTEX_POINTER]=&MessageProcessor::call_TAG_REQUEST_VERTEX_POINTER;
	m_methods[TAG_ASK_IS_ASSEMBLED_REPLY]=&MessageProcessor::call_TAG_ASK_IS_ASSEMBLED_REPLY;
	m_methods[TAG_MARK_AS_ASSEMBLED]=&MessageProcessor::call_TAG_MARK_AS_ASSEMBLED;
	m_methods[TAG_ASK_EXTENSION_DATA]=&MessageProcessor::call_TAG_ASK_EXTENSION_DATA;
	m_methods[TAG_EXTENSION_DATA]=&MessageProcessor::call_TAG_EXTENSION_DATA;
	m_methods[TAG_EXTENSION_END]=&MessageProcessor::call_TAG_EXTENSION_END;
	m_methods[TAG_EXTENSION_DATA_END]=&MessageProcessor::call_TAG_EXTENSION_DATA_END;
	m_methods[TAG_ATTACH_SEQUENCE]=&MessageProcessor::call_TAG_ATTACH_SEQUENCE;
	m_methods[TAG_REQUEST_READS]=&MessageProcessor::call_TAG_REQUEST_READS;
	m_methods[TAG_REQUEST_READS_REPLY]=&MessageProcessor::call_TAG_REQUEST_READS_REPLY;
	m_methods[TAG_ASK_READ_VERTEX_AT_POSITION]=&MessageProcessor::call_TAG_ASK_READ_VERTEX_AT_POSITION;
	m_methods[TAG_ASK_READ_VERTEX_AT_POSITION_REPLY]=&MessageProcessor::call_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY;
	m_methods[TAG_ASK_READ_LENGTH]=&MessageProcessor::call_TAG_ASK_READ_LENGTH;
	m_methods[TAG_ASK_READ_LENGTH_REPLY]=&MessageProcessor::call_TAG_ASK_READ_LENGTH_REPLY;
	m_methods[TAG_SAVE_WAVE_PROGRESSION]=&MessageProcessor::call_TAG_SAVE_WAVE_PROGRESSION;
	m_methods[TAG_COPY_DIRECTIONS]=&MessageProcessor::call_TAG_COPY_DIRECTIONS;
	m_methods[TAG_ASSEMBLE_WAVES]=&MessageProcessor::call_TAG_ASSEMBLE_WAVES;
	m_methods[TAG_SAVE_WAVE_PROGRESSION_REVERSE]=&MessageProcessor::call_TAG_SAVE_WAVE_PROGRESSION_REVERSE;
	m_methods[TAG_ASSEMBLE_WAVES_DONE]=&MessageProcessor::call_TAG_ASSEMBLE_WAVES_DONE;
	m_methods[TAG_START_FUSION]=&MessageProcessor::call_TAG_START_FUSION;
	m_methods[TAG_FUSION_DONE]=&MessageProcessor::call_TAG_FUSION_DONE;
	m_methods[TAG_ASK_VERTEX_PATHS_SIZE]=&MessageProcessor::call_TAG_ASK_VERTEX_PATHS_SIZE;
	m_methods[TAG_ASK_VERTEX_PATHS_SIZE_REPLY]=&MessageProcessor::call_TAG_ASK_VERTEX_PATHS_SIZE_REPLY;
	m_methods[TAG_GET_PATH_LENGTH]=&MessageProcessor::call_TAG_GET_PATH_LENGTH;
	m_methods[TAG_GET_PATH_LENGTH_REPLY]=&MessageProcessor::call_TAG_GET_PATH_LENGTH_REPLY;
	m_methods[TAG_CALIBRATION_MESSAGE]=&MessageProcessor::call_TAG_CALIBRATION_MESSAGE;
	m_methods[TAG_BEGIN_CALIBRATION]=&MessageProcessor::call_TAG_BEGIN_CALIBRATION;
	m_methods[TAG_END_CALIBRATION]=&MessageProcessor::call_TAG_END_CALIBRATION;
	m_methods[TAG_COMMUNICATION_STABILITY_MESSAGE]=&MessageProcessor::call_TAG_COMMUNICATION_STABILITY_MESSAGE;
	m_methods[TAG_ASK_VERTEX_PATH]=&MessageProcessor::call_TAG_ASK_VERTEX_PATH;
	m_methods[TAG_ASK_VERTEX_PATH_REPLY]=&MessageProcessor::call_TAG_ASK_VERTEX_PATH_REPLY;
	m_methods[TAG_INDEX_PAIRED_SEQUENCE]=&MessageProcessor::call_TAG_INDEX_PAIRED_SEQUENCE;
	m_methods[TAG_HAS_PAIRED_READ]=&MessageProcessor::call_TAG_HAS_PAIRED_READ;
	m_methods[TAG_HAS_PAIRED_READ_REPLY]=&MessageProcessor::call_TAG_HAS_PAIRED_READ_REPLY;
	m_methods[TAG_GET_PAIRED_READ]=&MessageProcessor::call_TAG_GET_PAIRED_READ;
	m_methods[TAG_GET_PAIRED_READ_REPLY]=&MessageProcessor::call_TAG_GET_PAIRED_READ_REPLY;
	m_methods[TAG_CLEAR_DIRECTIONS]=&MessageProcessor::call_TAG_CLEAR_DIRECTIONS;
	m_methods[TAG_CLEAR_DIRECTIONS_REPLY]=&MessageProcessor::call_TAG_CLEAR_DIRECTIONS_REPLY;
	m_methods[TAG_FINISH_FUSIONS]=&MessageProcessor::call_TAG_FINISH_FUSIONS;
	m_methods[TAG_FINISH_FUSIONS_FINISHED]=&MessageProcessor::call_TAG_FINISH_FUSIONS_FINISHED;
	m_methods[TAG_DISTRIBUTE_FUSIONS]=&MessageProcessor::call_TAG_DISTRIBUTE_FUSIONS;
	m_methods[TAG_DISTRIBUTE_FUSIONS_FINISHED]=&MessageProcessor::call_TAG_DISTRIBUTE_FUSIONS_FINISHED;
	m_methods[TAG_EXTENSION_START]=&MessageProcessor::call_TAG_EXTENSION_START;
	m_methods[TAG_ELIMINATE_PATH]=&MessageProcessor::call_TAG_ELIMINATE_PATH;
	m_methods[TAG_GET_PATH_VERTEX]=&MessageProcessor::call_TAG_GET_PATH_VERTEX;
	m_methods[TAG_GET_PATH_VERTEX_REPLY]=&MessageProcessor::call_TAG_GET_PATH_VERTEX_REPLY;
	m_methods[TAG_SET_COLOR_MODE]=&MessageProcessor::call_TAG_SET_COLOR_MODE;
	m_methods[TAG_AUTOMATIC_DISTANCE_DETECTION]=&MessageProcessor::call_TAG_AUTOMATIC_DISTANCE_DETECTION;
	m_methods[TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE]=&MessageProcessor::call_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE;
	m_methods[TAG_LIBRARY_DISTANCE]=&MessageProcessor::call_TAG_LIBRARY_DISTANCE;
	m_methods[TAG_ASK_LIBRARY_DISTANCES]=&MessageProcessor::call_TAG_ASK_LIBRARY_DISTANCES;
	m_methods[TAG_ASK_LIBRARY_DISTANCES_FINISHED]=&MessageProcessor::call_TAG_ASK_LIBRARY_DISTANCES_FINISHED;
	m_methods[TAG_UPDATE_LIBRARY_INFORMATION]=&MessageProcessor::call_TAG_UPDATE_LIBRARY_INFORMATION;
	m_methods[TAG_RECEIVED_COVERAGE_INFORMATION]=&MessageProcessor::call_TAG_RECEIVED_COVERAGE_INFORMATION;
	m_methods[TAG_REQUEST_READ_SEQUENCE]=&MessageProcessor::call_TAG_REQUEST_READ_SEQUENCE;
	m_methods[TAG_REQUEST_READ_SEQUENCE_REPLY]=&MessageProcessor::call_TAG_REQUEST_READ_SEQUENCE_REPLY;

}

void MessageProcessor::constructor(ExtensionData*ed,
			int*m_numberOfRanksDoneDetectingDistances,
			int*m_numberOfRanksDoneSendingDistances,
			Parameters*parameters,
			int*m_libraryIterator,
			bool*m_libraryIndexInitiated,
			MyForest*m_subgraph,
			MyAllocator*m_outboxAllocator,
				int rank,
			vector<ReadAnnotation>*m_EXTENSION_receivedReads,
			int*m_numberOfMachinesDoneSendingEdges,
			FusionData*m_fusionData,
			vector<vector<VERTEX_TYPE> >*m_EXTENSION_contigs,
			int*m_wordSize,
			int*m_minimumCoverage,
			int*m_seedCoverage,
			int*m_peakCoverage,
			vector<Read*>*m_myReads,
			bool*m_EXTENSION_currentRankIsDone,
	vector<vector<VERTEX_TYPE> >*m_FINISH_newFusions,
		int size,
	MyAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<int>*m_identifiers,
	bool*m_mode_sendDistribution,
	bool*m_alive,
	vector<VERTEX_TYPE>*m_SEEDING_receivedIngoingEdges,
	VERTEX_TYPE*m_SEEDING_receivedKey,
	int*m_SEEDING_i,
	bool*m_colorSpaceMode,
	bool*m_FINISH_fusionOccured,
	bool*m_Machine_getPaths_INITIALIZED,
	int*m_mode,
	vector<vector<VERTEX_TYPE> >*m_allPaths,
	bool*m_EXTENSION_VertexAssembled_received,
	int*m_EXTENSION_numberOfRanksDone,
	int*m_EXTENSION_currentPosition,
	int*m_last_value,
	vector<int>*m_EXTENSION_identifiers,
	int*m_ranksDoneAttachingReads,
	bool*m_SEEDING_edgesReceived,
	PairedRead*m_EXTENSION_pairedRead,
	bool*m_mode_EXTENSION,
	vector<VERTEX_TYPE>*m_SEEDING_receivedOutgoingEdges,
	int*m_DISTRIBUTE_n,
	vector<VERTEX_TYPE>*m_SEEDING_nodes,
	bool*m_EXTENSION_hasPairedReadReceived,
	int*m_numberOfRanksDoneSeeding,
	bool*m_SEEDING_vertexKeyAndCoverageReceived,
	int*m_SEEDING_receivedVertexCoverage,
	bool*m_EXTENSION_readLength_received,
	bool*m_Machine_getPaths_DONE,
	int*m_CLEAR_n,
	bool*m_FINISH_vertex_received,
	bool*m_EXTENSION_initiated,
	int*m_readyToSeed,
	bool*m_SEEDING_NodeInitiated,
	int*m_FINISH_n,
	bool*m_nextReductionOccured,
	bool*m_EXTENSION_hasPairedReadAnswer,
	MyAllocator*m_directionsAllocator,
	map<int,int>*m_FINISH_pathLengths,
	bool*m_EXTENSION_pairedSequenceReceived,
	int*m_EXTENSION_receivedLength,
	int*m_mode_send_coverage_iterator,
	map<int,VERTEX_TYPE>*m_coverageDistribution,
	VERTEX_TYPE*m_FINISH_received_vertex,
	bool*m_EXTENSION_read_vertex_received,
	int*m_sequence_ready_machines,
	bool*m_SEEDING_InedgesReceived,
	bool*m_EXTENSION_vertexIsAssembledResult,
	bool*m_SEEDING_vertexCoverageReceived,
	VERTEX_TYPE*m_EXTENSION_receivedReadVertex,
	int*m_numberOfMachinesReadyForEdgesDistribution,
	int*m_numberOfMachinesReadyToSendDistribution,
	bool*m_mode_send_outgoing_edges,
	int*m_mode_send_vertices_sequence_id,
	bool*m_mode_send_vertices,
	int*m_numberOfMachinesDoneSendingVertices,
	int*m_numberOfMachinesDoneSendingCoverage,
	bool*m_EXTENSION_reads_received,
				vector<Message>*m_outbox,
		map<int,int>*m_allIdentifiers,OpenAssemblerChooser*m_oa,
int*m_numberOfRanksWithCoverageData,
SeedExtender*seedExtender){

	this->ed=ed;
	this->m_numberOfRanksDoneDetectingDistances=m_numberOfRanksDoneDetectingDistances;
	this->m_numberOfRanksDoneSendingDistances=m_numberOfRanksDoneSendingDistances;
	this->parameters=parameters;
	this->m_libraryIterator=m_libraryIterator;
	this->m_libraryIndexInitiated=m_libraryIndexInitiated;
	this->m_subgraph=m_subgraph;
	this->m_outboxAllocator=m_outboxAllocator;
	this->rank=rank;
	this->m_EXTENSION_receivedReads=m_EXTENSION_receivedReads;
	this->m_numberOfMachinesDoneSendingEdges=m_numberOfMachinesDoneSendingEdges;
	this->m_fusionData=m_fusionData;
	this->m_EXTENSION_contigs=m_EXTENSION_contigs;
	this->m_wordSize=m_wordSize;
	this->m_minimumCoverage=m_minimumCoverage;
	this->m_seedCoverage=m_seedCoverage;
	this->m_peakCoverage=m_peakCoverage;
	this->m_myReads=m_myReads;
	this->m_EXTENSION_currentRankIsDone=m_EXTENSION_currentRankIsDone;
	this->m_FINISH_newFusions=m_FINISH_newFusions;
	this->size=size;
	this->m_inboxAllocator=m_inboxAllocator;
	this->m_persistentAllocator=m_persistentAllocator;
	this->m_identifiers=m_identifiers;
	this->m_mode_sendDistribution=m_mode_sendDistribution;
	this->m_alive=m_alive;
	this->m_SEEDING_receivedIngoingEdges=m_SEEDING_receivedIngoingEdges;
	this->m_SEEDING_receivedKey=m_SEEDING_receivedKey;
	this->m_SEEDING_i=m_SEEDING_i;
	this->m_colorSpaceMode=m_colorSpaceMode;
	this->m_FINISH_fusionOccured=m_FINISH_fusionOccured;
	this->m_Machine_getPaths_INITIALIZED=m_Machine_getPaths_INITIALIZED;
	this->m_mode=m_mode;
	this->m_allPaths=m_allPaths;
	this->m_EXTENSION_VertexAssembled_received=m_EXTENSION_VertexAssembled_received;
	this->m_EXTENSION_numberOfRanksDone=m_EXTENSION_numberOfRanksDone;
	this->m_EXTENSION_currentPosition=m_EXTENSION_currentPosition;
	this->m_last_value=m_last_value;
	this->m_EXTENSION_identifiers=m_EXTENSION_identifiers;
	this->m_ranksDoneAttachingReads=m_ranksDoneAttachingReads;
	this->m_SEEDING_edgesReceived=m_SEEDING_edgesReceived;
	this->m_EXTENSION_pairedRead=m_EXTENSION_pairedRead;
	this->m_mode_EXTENSION=m_mode_EXTENSION;
	this->m_SEEDING_receivedOutgoingEdges=m_SEEDING_receivedOutgoingEdges;
	this->m_DISTRIBUTE_n=m_DISTRIBUTE_n;
	this->m_SEEDING_nodes=m_SEEDING_nodes;
	this->m_EXTENSION_hasPairedReadReceived=m_EXTENSION_hasPairedReadReceived;
	this->m_numberOfRanksDoneSeeding=m_numberOfRanksDoneSeeding;
	this->m_SEEDING_vertexKeyAndCoverageReceived=m_SEEDING_vertexKeyAndCoverageReceived;
	this->m_SEEDING_receivedVertexCoverage=m_SEEDING_receivedVertexCoverage;
	this->m_EXTENSION_readLength_received=m_EXTENSION_readLength_received;
	this->m_Machine_getPaths_DONE=m_Machine_getPaths_DONE;
	this->m_CLEAR_n=m_CLEAR_n;
	this->m_FINISH_vertex_received=m_FINISH_vertex_received;
	this->m_EXTENSION_initiated=m_EXTENSION_initiated;
	this->m_readyToSeed=m_readyToSeed;
	this->m_SEEDING_NodeInitiated=m_SEEDING_NodeInitiated;
	this->m_FINISH_n=m_FINISH_n;
	this->m_nextReductionOccured=m_nextReductionOccured;
	this->m_EXTENSION_hasPairedReadAnswer=m_EXTENSION_hasPairedReadAnswer;
	this->m_directionsAllocator=m_directionsAllocator;
	this->m_FINISH_pathLengths=m_FINISH_pathLengths;
	this->m_EXTENSION_pairedSequenceReceived=m_EXTENSION_pairedSequenceReceived;
	this->m_EXTENSION_receivedLength=m_EXTENSION_receivedLength;
	this->m_mode_send_coverage_iterator=m_mode_send_coverage_iterator;
	this->m_coverageDistribution=m_coverageDistribution;
	this->m_FINISH_received_vertex=m_FINISH_received_vertex;
	this->m_EXTENSION_read_vertex_received=m_EXTENSION_read_vertex_received;
	this->m_sequence_ready_machines=m_sequence_ready_machines;
	this->m_SEEDING_InedgesReceived=m_SEEDING_InedgesReceived;
	this->m_EXTENSION_vertexIsAssembledResult=m_EXTENSION_vertexIsAssembledResult;
	this->m_SEEDING_vertexCoverageReceived=m_SEEDING_vertexCoverageReceived;
	this->m_EXTENSION_receivedReadVertex=m_EXTENSION_receivedReadVertex;
	this->m_numberOfMachinesReadyForEdgesDistribution=m_numberOfMachinesReadyForEdgesDistribution;
	this->m_numberOfMachinesReadyToSendDistribution=m_numberOfMachinesReadyToSendDistribution;
	this->m_mode_send_outgoing_edges=m_mode_send_outgoing_edges;
	this->m_mode_send_vertices_sequence_id=m_mode_send_vertices_sequence_id;
	this->m_mode_send_vertices=m_mode_send_vertices;
	this->m_numberOfMachinesDoneSendingVertices=m_numberOfMachinesDoneSendingVertices;
	this->m_numberOfMachinesDoneSendingCoverage=m_numberOfMachinesDoneSendingCoverage;
	this->m_EXTENSION_reads_received=m_EXTENSION_reads_received;
	this->m_outbox=m_outbox;
	this->m_allIdentifiers=m_allIdentifiers,
	this->m_oa=m_oa;
	this->m_numberOfRanksWithCoverageData=m_numberOfRanksWithCoverageData;
	this->seedExtender=seedExtender;
}
