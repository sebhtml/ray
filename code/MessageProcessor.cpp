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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include<string.h>
#include<assert.h>
#include<Read.h>
#include<MessageProcessor.h>
#include<StaticVector.h>
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

void MessageProcessor::call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_FORCE(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	uint64_t*out=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int validEdges=0;
	for(int i=0;i<count;i+=2){
		if(m_subgraph->find(incoming[i+0])!=NULL){
			out[2*validEdges+0]=incoming[i+0];
			out[2*validEdges+1]=incoming[i+1];
			validEdges++;
		}
	}

	Message aMessage(out,2*validEdges,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY_FORCE,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	for(int i=0;i<count;i++){
		uint64_t vertex=incoming[i];
		SplayNode<uint64_t,Vertex>*node=m_subgraph->find(vertex);
		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		// using ingoing edges, tell parents to delete the associated outgoing edge
		vector<uint64_t> ingoingEdges=node->getValue()->getIngoingEdges(vertex,*m_wordSize);
		for(int j=0;j<(int)ingoingEdges.size();j++){
			uint64_t prefix=ingoingEdges[j];
			uint64_t suffix=vertex;
			int rankToFlush=vertexRank(prefix,size);
			m_buffersForOutgoingEdgesToDelete.addAt(rankToFlush,prefix);
			m_buffersForOutgoingEdgesToDelete.addAt(rankToFlush,suffix);
			m_buffersForOutgoingEdgesToDelete.flush(rankToFlush,2,RAY_MPI_TAG_DELETE_OUTGOING_EDGE,m_outboxAllocator,m_outbox,rank,false);
		}

		// using outgoing edges, tell children to delete the associated ingoing edge
		vector<uint64_t> outgoingEdges=node->getValue()->getOutgoingEdges(vertex,*m_wordSize);
		for(int j=0;j<(int)outgoingEdges.size();j++){
			uint64_t prefix=vertex;
			uint64_t suffix=outgoingEdges[j];
			int rankToFlush=vertexRank(suffix,size);
			m_buffersForIngoingEdgesToDelete.addAt(rankToFlush,prefix);
			m_buffersForIngoingEdgesToDelete.addAt(rankToFlush,suffix);
			m_buffersForIngoingEdgesToDelete.flush(rankToFlush,2,RAY_MPI_TAG_DELETE_INGOING_EDGE,m_outboxAllocator,m_outbox,rank,false);
		}

		// delete the vertex
		m_subgraph->remove(vertex);
	}
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_DELETE_VERTEX_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_INGOING_EDGE(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	for(int i=0;i<count;i+=2){
		uint64_t prefix=incoming[i+0];
		uint64_t suffix=incoming[i+1];
		SplayNode<uint64_t,Vertex>*node=m_subgraph->find(suffix);

		if(node==NULL){ // node already deleted, don't need to delete the edges.
			continue;
		}

		#ifdef ASSERT
		int before=node->getValue()->getIngoingEdges(suffix,*m_wordSize).size();
		#endif

		node->getValue()->deleteIngoingEdge(prefix,*m_wordSize);

		#ifdef ASSERT
		int after=node->getValue()->getIngoingEdges(suffix,*m_wordSize).size();
		assert(after+1==before);
		#endif
	}
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_OUTGOING_EDGE(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	for(int i=0;i<count;i+=2){
		uint64_t prefix=incoming[i+0];
		uint64_t suffix=incoming[i+1];
		SplayNode<uint64_t,Vertex>*node=m_subgraph->find(prefix);

		if(node==NULL){ // node already deleted, don't need to delete the edges.
			continue;
		}
	
		#ifdef ASSERT
		int before=node->getValue()->getOutgoingEdges(prefix,*m_wordSize).size();
		#endif

		node->getValue()->deleteOutgoingEdge(suffix,*m_wordSize);

		#ifdef ASSERT
		int after=node->getValue()->getOutgoingEdges(prefix,*m_wordSize).size();
		assert(after+1==before);
		#endif

	}
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX_REPLY(Message*message){
	m_verticesExtractor->setReadiness(m_outbox,rank);
}

void MessageProcessor::call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY_FORCE(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	m_edgesExtractor->receiveIngoingEdges(incoming,count,true);
}

void MessageProcessor::call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_FORCE(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	uint64_t*out=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int validEdges=0;
	for(int i=0;i<count;i+=2){
		if(m_subgraph->find(incoming[i+1])!=NULL){
			out[2*validEdges+0]=incoming[i+0];
			out[2*validEdges+1]=incoming[i+1];
			validEdges++;
		}
	}
	Message aMessage(out,2*validEdges,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY_FORCE,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY_FORCE(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	m_edgesExtractor->receiveOutgoingEdges(incoming,count,true);
}

void MessageProcessor::call_RAY_MPI_TAG_VERIFY_INGOING_EDGES(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	uint64_t*out=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int validEdges=0;
	for(int i=0;i<count;i+=2){
		if(m_subgraph->find(incoming[i+0])!=NULL){
			out[2*validEdges+0]=incoming[i+0];
			out[2*validEdges+1]=incoming[i+1];
			validEdges++;
		}
	}
	Message aMessage(out,2*validEdges,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	m_edgesExtractor->receiveIngoingEdges(incoming,count,false);
}

void MessageProcessor::call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	uint64_t*out=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int validEdges=0;
	for(int i=0;i<count;i+=2){
		if(m_subgraph->find(incoming[i+1])!=NULL){
			out[2*validEdges+0]=incoming[i+0];
			out[2*validEdges+1]=incoming[i+1];
			validEdges++;
		}
	}
	Message aMessage(out,2*validEdges,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	m_edgesExtractor->receiveOutgoingEdges(incoming,count,false);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_BEGIN_REDUCTION_REPLY(Message*aMessage){
	m_verticesExtractor->incrementRanksReadyForReduction();
	if(m_verticesExtractor->readyForReduction()){
		(*m_master_mode)=RAY_MASTER_MODE_START_REDUCTION;
		m_verticesExtractor->resetRanksReadyForReduction();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_RESUME_VERTEX_DISTRIBUTION(Message*message){
	if(!m_verticesExtractor->finished()){
		cout<<"Rank "<<rank<<": "<<m_lastSize<<" -> "<<m_subgraph->size()<<endl;
		(*m_mode)=RAY_SLAVE_MODE_EXTRACT_VERTICES;
		m_verticesExtractor->removeTrigger();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_REDUCE_MEMORY_CONSUMPTION_DONE(Message*message){
	m_verticesExtractor->incrementRanksDoneWithReduction();
	if(m_verticesExtractor->reductionIsDone()){
		for(int i=0;i<size;i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_DELETE_VERTICES,rank);
			m_outbox->push_back(aMessage);
		}
		m_verticesExtractor->resetRanksDoneForReduction();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTICES(Message*message){
	m_lastSize=m_subgraph->size();
	(*m_mode)=RAY_SLAVE_MODE_DELETE_VERTICES;
	m_subgraph->unfreeze();
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTICES_DONE(Message*message){
	m_verticesExtractor->incrementRanksDoneWithReduction();
	if(m_verticesExtractor->reductionIsDone()){
		for(int i=0;i<size;i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_UPDATE_THRESHOLD,rank);
			m_outbox->push_back(aMessage);
		}
		m_verticesExtractor->resetRanksDoneForReduction();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_UPDATE_THRESHOLD(Message*message){
	m_verticesExtractor->updateThreshold(m_subgraph);

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY(Message*message){
	m_verticesExtractor->incrementRanksDoneWithReduction();
	if(m_verticesExtractor->reductionIsDone()){
		(*m_master_mode)=RAY_MASTER_MODE_RESUME_VERTEX_DISTRIBUTION;
		m_verticesExtractor->resetRanksDoneForReduction();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_START_REDUCTION(Message*message){
	#ifdef ASSERT
	m_verticesExtractor->assertBuffersAreEmpty();
	#endif
	(*m_mode)=RAY_SLAVE_MODE_REDUCE_MEMORY_CONSUMPTION;
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_BEGIN_REDUCTION(Message*message){
	m_verticesExtractor->flushAll(m_outboxAllocator,m_outbox,rank);
	m_verticesExtractor->scheduleReduction();
	(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
}

void MessageProcessor::call_RAY_MPI_TAG_MUST_RUN_REDUCER(Message*message){
	int rank=message->getSource();
	m_verticesExtractor->addRankForReduction(rank);
	if(m_verticesExtractor->mustRunReducer()){
		(*m_master_mode)=RAY_MASTER_MODE_ASK_BEGIN_REDUCTION;
		m_verticesExtractor->resetRanksForReduction();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_WELCOME(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_BARRIER(Message*message){
	MPI_Barrier(MPI_COMM_WORLD);
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_SEQUENCE_REGULATOR(Message*message){
	call_RAY_MPI_TAG_SEND_SEQUENCE(message);
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_SEND_SEQUENCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_INDEX_SEQUENCES;
	m_si->constructor(size);
}

/*
 * seq1.........\0 seq2.......\0 \0  <--------second \0 indicates end of stream
 *
 *
 *
 */
void MessageProcessor::call_RAY_MPI_TAG_SEND_SEQUENCE(Message*message){
	char*buffer=(char*)message->getBuffer();
	int currentPosition=0;

	#ifdef ASSERT
	int n=0;
	while(buffer[currentPosition]!=ASCII_END_OF_TRANSMISSION){
		currentPosition+=(strlen(buffer+currentPosition)+1);
		n++;
	}
	currentPosition=0;
	assert(n>0);
	#endif

	while(buffer[currentPosition]!=ASCII_END_OF_TRANSMISSION){
		Read myRead;
		myRead.copy(NULL,buffer+currentPosition,&(*m_persistentAllocator),false); // no trimming
		m_myReads->push_back(&myRead);
		if((*m_myReads).size()%100000==0){
			printf("Rank %i has %i sequence reads\n",rank,(int)(*m_myReads).size());
			fflush(stdout);
		}
		// move currentPosition after the first \0 encountered.
		currentPosition+=(strlen(buffer+currentPosition)+1);
	}
}

void MessageProcessor::call_RAY_MPI_TAG_SHOW_SEQUENCES(Message*message){
	printf("Rank %i has %i sequence reads\n",rank,(int)(*m_myReads).size());
	fflush(stdout);
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_SEQUENCE_REPLY(Message*message){
	m_sequencesLoader->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_SEQUENCES_READY(Message*message){
	(*m_sequence_ready_machines)++;
	if(*m_sequence_ready_machines==size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS(Message*message){
	int source=message->getSource();
	printf("Rank %i has %i sequence reads\n",rank,(int)(*m_myReads).size());
	fflush(stdout);
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_SEQUENCES_READY,rank);
	m_outbox->push_back(aMessage);
}

/*
 * receive vertices (data)
 */
void MessageProcessor::call_RAY_MPI_TAG_VERTICES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	int length=count;

	#ifdef ASSERT
	assert(!m_subgraph->frozen());
	#endif

	for(int i=0;i<length;i++){
		uint64_t l=incoming[i];

		if((*m_last_value)!=(int)m_subgraph->size() and (int)m_subgraph->size()%100000==0){
			(*m_last_value)=m_subgraph->size();
			printf("Rank %i has %i vertices\n",rank,(int)m_subgraph->size());
			fflush(stdout);
		}

		SplayNode<uint64_t,Vertex>*tmp=m_subgraph->insert(l);
		#ifdef ASSERT
		assert(tmp!=NULL);
		#endif
		if(m_subgraph->inserted()){
			tmp->getValue()->constructor(); 
		}
		tmp->getValue()->setCoverage(tmp->getValue()->getCoverage()+1);
		#ifdef ASSERT
		assert(tmp->getValue()->getCoverage()>0);
		#endif
	}
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_VERTICES_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);

	if(m_subgraph->size()>=m_verticesExtractor->getThreshold() && !m_verticesExtractor->isTriggered()){
		m_verticesExtractor->trigger();
		Message aMessage2(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_MUST_RUN_REDUCER,rank);
		m_outbox->push_back(aMessage2);
	}
}

void MessageProcessor::call_RAY_MPI_TAG_VERTICES_DATA_REPLY(Message*message){
	m_verticesExtractor->setReadiness(m_outbox,rank);
}

void MessageProcessor::call_RAY_MPI_TAG_VERTICES_DISTRIBUTED(Message*message){
	(*m_numberOfMachinesDoneSendingVertices)++;
	if((*m_numberOfMachinesDoneSendingVertices)==size){
		(*m_master_mode)=RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY(Message*message){
	m_verticesExtractor->setReadiness(m_outbox,rank);
}

void MessageProcessor::call_RAY_MPI_TAG_OUT_EDGES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	int length=count;

	for(int i=0;i<(int)length;i+=2){
		uint64_t prefix=incoming[i+0];
		uint64_t suffix=incoming[i+1];

		SplayNode<uint64_t,Vertex>*node=m_subgraph->find(prefix);
		if(node==NULL){
			continue;
		}

		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		node->getValue()->addOutgoingEdge(suffix,(*m_wordSize));
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_OUT_EDGES_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION(Message*message){
	// wait for everyone
	MPI_Barrier(MPI_COMM_WORLD);
	(*m_mode_send_vertices)=true;
	(*m_mode)=RAY_SLAVE_MODE_EXTRACT_VERTICES;
	m_verticesExtractor->constructor(size);

	(*m_mode_send_vertices_sequence_id)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_EDGES_DISTRIBUTED(Message*message){
	(*m_numberOfMachinesDoneSendingEdges)++;
	if((*m_numberOfMachinesDoneSendingEdges)==size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_INDEXING;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY(Message*message){
	m_verticesExtractor->setReadiness(m_outbox,rank);
}

void MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	int length=count;

	for(int i=0;i<(int)length;i+=2){
		uint64_t prefix=incoming[i+0];
		uint64_t suffix=incoming[i+1];
	
		SplayNode<uint64_t,Vertex>*node=m_subgraph->find(suffix);
		if(node==NULL){
			continue;
		}
		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		node->getValue()->addIngoingEdge(prefix,(*m_wordSize));
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_IN_EDGES_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION(Message*message){
	(*m_mode_send_outgoing_edges)=true;
	(*m_mode)=RAY_SLAVE_MODE_PROCESS_OUTGOING_EDGES;
	m_edgesExtractor->constructor(size);
}

void MessageProcessor::call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ASK(Message*message){
	int source=message->getSource();
	Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, source, RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ANSWER,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ANSWER(Message*message){
	(*m_numberOfMachinesReadyForEdgesDistribution)++;
	if(*m_numberOfMachinesReadyForEdgesDistribution==size){
		(*m_master_mode)=RAY_MASTER_MODE_START_EDGES_DISTRIBUTION;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message){
	// freeze the forest. icy winter ahead.
	m_subgraph->freeze();
	//m_subgraph->show(rank,parameters->getPrefix().c_str());
	int source=message->getSource();
	printf("Rank %i has %i vertices (completed)\n",rank,(int)m_subgraph->size());
	fflush(stdout);

	Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, source, RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER(Message*message){
	(*m_numberOfMachinesReadyToSendDistribution)++;
	if((*m_numberOfMachinesReadyToSendDistribution)==size){
		(*m_master_mode)=RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION(Message*message){
	(*m_mode_send_coverage_iterator)=0;
	(*m_mode_sendDistribution)=true;
	(*m_mode)=RAY_SLAVE_MODE_SEND_DISTRIBUTION;
}

void MessageProcessor::call_RAY_MPI_TAG_COVERAGE_DATA(Message*message){
	void*buffer=message->getBuffer();
	int*incoming=(int*)buffer;
	int count=incoming[0];

	for(int i=0;i<count;i++){
		int coverage=incoming[1+2*i+0];
		uint64_t count=incoming[1+2*i+1];
		(*m_coverageDistribution)[coverage]+=count;
	}
	call_RAY_MPI_TAG_COVERAGE_END(message);
}

void MessageProcessor::call_RAY_MPI_TAG_COVERAGE_END(Message*message){
	(*m_numberOfMachinesDoneSendingCoverage)++;
	if((*m_numberOfMachinesDoneSendingCoverage)==size){
		(*m_master_mode)=RAY_MASTER_MODE_SEND_COVERAGE_VALUES;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_COVERAGE_VALUES(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(*m_minimumCoverage)=incoming[0];
	(*m_seedCoverage)=incoming[1];
	(*m_peakCoverage)=incoming[2];
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_RECEIVED_COVERAGE_INFORMATION,rank);
	m_outbox->push_back(aMessage);
	m_oa->constructor((*m_peakCoverage));
	parameters->setPeakCoverage(*m_peakCoverage);
}

void MessageProcessor::call_RAY_MPI_TAG_READY_TO_SEED(Message*message){
	(*m_readyToSeed)++;
	if((*m_readyToSeed)==size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_SEEDING;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_START_SEEDING(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_START_SEEDING;
	map<int,map<int,int> > edgesDistribution;
	
	#ifdef ASSERT
	assert(m_subgraph!=NULL);
	#endif

	int size=0;
	for(int i=0;i<m_subgraph->getNumberOfTrees();i++){
		SplayTreeIterator<uint64_t,Vertex> seedingIterator(m_subgraph->getTree(i));
		while(seedingIterator.hasNext()){
			size++;
			SplayNode<uint64_t,Vertex>*node=seedingIterator.next();
			edgesDistribution[node->getValue()->getIngoingEdges(node->getKey(),(*m_wordSize)).size()][node->getValue()->getOutgoingEdges(node->getKey(),(*m_wordSize)).size()]++;
			//(m_seedingData->m_SEEDING_nodes).push_back(node->getKey());
		}
	}
	#ifdef ASSERT
	//assert((int)m_subgraph->size()==size);
	//cout<<"Ingoing and outgoing edges."<<endl;
	for(map<int,map<int,int> >::iterator i=edgesDistribution.begin();i!=edgesDistribution.end();++i){
		for(map<int,int>::iterator j=i->second.begin();j!=i->second.end();++j){
			//cout<<i->first<<" "<<j->first<<" "<<j->second<<endl;
		}
	}
	#endif
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	SplayNode<uint64_t,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef ASSERT
	if(node==NULL){
		cout<<idToWord(incoming[0],(*m_wordSize))<<endl;
	}
	assert(node!=NULL);
	#endif
	uint64_t coverage=node->getValue()->getCoverage();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
	message2[0]=coverage;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedVertexCoverage)=incoming[0];
	(m_seedingData->m_SEEDING_vertexCoverageReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	SplayNode<uint64_t,Vertex>*node=(SplayNode<uint64_t,Vertex>*)incoming[0];
	uint64_t key=node->getKey();
	uint64_t coverage=node->getValue()->getCoverage();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message2[0]=key;
	message2[1]=coverage;
	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedKey)=incoming[0];
	(m_seedingData->m_SEEDING_receivedVertexCoverage)=incoming[1];
	(m_seedingData->m_SEEDING_vertexKeyAndCoverageReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	vector<uint64_t> outgoingEdges=m_subgraph->find(incoming[0])->getValue()->getOutgoingEdges(incoming[0],*m_wordSize);
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(outgoingEdges.size()*sizeof(uint64_t));
	for(int i=0;i<(int)outgoingEdges.size();i++){
		message2[i]=outgoingEdges[i];
	}
	Message aMessage(message2,outgoingEdges.size(),MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	SplayNode<uint64_t,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef ASSERT
	assert(node!=NULL);
	#endif
	vector<uint64_t> outgoingEdges=node->getValue()->getOutgoingEdges(incoming[0],*m_wordSize);
	vector<uint64_t> ingoingEdges=node->getValue()->getIngoingEdges(incoming[0],*m_wordSize);
	int toAllocate=(2+outgoingEdges.size()+ingoingEdges.size());
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(toAllocate*sizeof(uint64_t));
	int k=0;
	message2[k++]=outgoingEdges.size();
	for(int i=0;i<(int)outgoingEdges.size();i++){
		message2[k++]=outgoingEdges[i];
	}
	message2[k++]=ingoingEdges.size();
	for(int i=0;i<(int)ingoingEdges.size();i++){
		message2[k++]=ingoingEdges[i];
	}

	#ifdef ASSERT
	assert(k==(int)(outgoingEdges.size()+ingoingEdges.size()+2));
	#endif

	Message aMessage(message2,k,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedOutgoingEdges).clear();
	for(int i=0;i<count;i++){
		(m_seedingData->m_SEEDING_receivedOutgoingEdges).push_back(incoming[i]);
	}
	(m_seedingData->m_SEEDING_edgesReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_SEEDING_IS_OVER(Message*message){
	(*m_numberOfRanksDoneSeeding)++;
	if((*m_numberOfRanksDoneSeeding)==size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_DETECTION;
	}
}


void MessageProcessor::call_RAY_MPI_TAG_RECEIVED_MESSAGES_REPLY(Message*message){
	if(rank!=MASTER_RANK){
		(*m_alive)=false; // Rest In Peace.
	}
}

void MessageProcessor::call_RAY_MPI_TAG_RECEIVED_MESSAGES(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	for(int i=0;i<count;i++){
		m_messagesHandler->addCount(message->getSource(),incoming[i]);
	}
	if(message->getSource()!=MASTER_RANK && m_messagesHandler->isFinished(message->getSource())){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_RECEIVED_MESSAGES_REPLY,rank);
		m_outbox->push_back(aMessage);
	}
	if(m_messagesHandler->isFinished()){
		(*m_alive)=false;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON(Message*message){
	// send stats to master
	int i=0;
	while(i<size){
		uint64_t*data=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int j=0;
		int maxToProcess=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);
		while(i+j<size &&j<maxToProcess){
			data[j]=m_messagesHandler->getReceivedMessages()[i+j];
			j++;
		}
		Message aMessage(data,j,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_RECEIVED_MESSAGES,rank);
		m_outbox->push_back(aMessage);
		i+=maxToProcess;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_I_GO_NOW(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_SET_WORD_SIZE(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(*m_wordSize)=incoming[0];
}

void MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS(Message*message){
	int source=message->getSource();
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY(Message*message){
	(*m_ranksDoneAttachingReads)++;
	if((*m_ranksDoneAttachingReads)==size){
		(*m_master_mode)=RAY_MASTER_MODE_PREPARE_SEEDING;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	SplayNode<uint64_t,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef ASSERT
	assert(node!=NULL);
	#endif
	vector<uint64_t> ingoingEdges=node->getValue()->getIngoingEdges(incoming[0],*m_wordSize);
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(ingoingEdges.size()*sizeof(uint64_t));
	for(int i=0;i<(int)ingoingEdges.size();i++){
		message2[i]=ingoingEdges[i];
	}
	Message aMessage(message2,ingoingEdges.size(),MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedIngoingEdges).clear();
	for(int i=0;i<count;i++){
		(m_seedingData->m_SEEDING_receivedIngoingEdges).push_back(incoming[i]);
	}
	(m_seedingData->m_SEEDING_InedgesReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_IS_DONE(Message*message){
	(m_ed->m_EXTENSION_numberOfRanksDone)++;
	(m_ed->m_EXTENSION_currentRankIsDone)=true;
	if((m_ed->m_EXTENSION_numberOfRanksDone)==size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_FUSIONS;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_EXTENSION(Message*message){
	(m_ed->m_EXTENSION_initiated)=false;
	(*m_mode)=RAY_SLAVE_MODE_EXTENSION;
	(*m_last_value)=-1;
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	SplayNode<uint64_t,Vertex>*node=m_subgraph->find(incoming[0]);
	int offset=incoming[1];
	#ifdef ASSERT
	assert(node!=NULL);
	#endif
	vector<Direction> directions=node->getValue()->getDirections();
	//cout<<"directions="<<directions.size()<<endl;
	int maxSize=directions.size();
	//cout<<"source="<<source<<" self="<<rank<<" MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED directions="<<maxSize<<endl;

	// each one of them takes 2 elements., this is 4000/8/2-1 = 249
	int maxToProcess=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)/2-1; // -1 because we need to track the offset and the vertex too
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	message2[0]=incoming[0];
	int p=2; // 0 is vertex, 1 is offset
	int processed=0;
	while(processed<maxToProcess && offset+processed<maxSize){
		message2[p++]=directions[offset+processed].getWave();
		message2[p++]=directions[offset+processed].getProgression();
		processed++;
	}

	int nextOffset=offset+processed;
	message2[1]=nextOffset;

	#ifdef ASSERT
	if(directions.size()==0){
		assert(nextOffset==0);
	}
	#endif

	//cout<<"processed "<<processed<<endl;

	if(nextOffset==maxSize){
		Message aMessage(message2,2*processed+2,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY_END,rank);
		m_outbox->push_back(aMessage);
	}else{
		Message aMessage(message2,2*processed+2,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY,rank);
		m_outbox->push_back(aMessage);
	}
}

void MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY_END(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	for(int i=2;i<count;i+=2){
		int wave=incoming[i+0];
		int progression=incoming[i+1];
		Direction a;
		a.constructor(wave,progression);
		seedExtender->getDirections()->push_back(a);
	}

	(m_ed->m_EXTENSION_VertexAssembled_received)=true;
	(m_ed->m_EXTENSION_vertexIsAssembledResult)=seedExtender->getDirections()->size()>0;
	//cout<<"source="<<message->getSource()<<" self="<<rank<<" MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY_END directions="<<seedExtender->getDirections()->size()<<endl;
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	for(int i=2;i<count;i+=2){
		int wave=incoming[i+0];
		int progression=incoming[i+1];
		Direction a;
		a.constructor(wave,progression);
		seedExtender->getDirections()->push_back(a);
	}
	
	// ask for the next data chunk
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message2[0]=incoming[0];
	message2[1]=incoming[1];
	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_IS_ASSEMBLED,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_MARK_AS_ASSEMBLED(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_EXTENSION_DATA(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_SEND_EXTENSION_DATA;
	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA_REPLY(Message*message){
	(*m_ready)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	for(int i=0;i<count;i++){
		(*m_allPaths)[(*m_allPaths).size()-1].push_back(incoming[i+0]);
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_EXTENSION_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_END(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA_END(Message*message){
	(m_ed->m_EXTENSION_currentRankIsDone)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_ATTACH_SEQUENCE(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	for(int i=0;i<count;i+=4){
		uint64_t vertex=incoming[i+0];
		int rank=incoming[i+1];
		int sequenceIdOnDestination=(int)incoming[i+2];
		char strand=(char)incoming[i+3];
		SplayNode<uint64_t,Vertex>*node=m_subgraph->find(vertex);
		if(node==NULL){
			continue;
		}
		node->getValue()->addRead(rank,sequenceIdOnDestination,strand,&(*m_persistentAllocator));
	}
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READS(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	SplayNode<uint64_t,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef ASSERT
	assert(node!=NULL);
	#endif
	Vertex*theVertex=node->getValue();
	#ifdef ASSERT
	assert(theVertex!=NULL);
	#endif
	ReadAnnotation*e=theVertex->getReads();
	int maxToProcess=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)-3;
	maxToProcess=maxToProcess-maxToProcess%3;
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
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

		Message aMessage(message2,j,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_READS_REPLY,rank);
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
			Message aMessage(message2,j,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_READS_REPLY,rank);
			m_outbox->push_back(aMessage);
			// if more reads are to be sent
			if(e!=NULL){
				//allocate another chunk
				message2=(uint64_t*)m_outboxAllocator->allocate(maxToProcess*sizeof(uint64_t));
				j=0;
			}
		}
	}
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READS_REPLY(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	for(int i=0;i<count;i+=3){
		// beginning of transmission, s,0,s
		if(incoming[i]==m_sentinelValue 
		&& incoming[i+1]==0
		&& incoming[i+2]==m_sentinelValue){
			m_ed->m_EXTENSION_receivedReads.clear();
		// end of transmission, s,s,s
		}else if(incoming[i]==m_sentinelValue 
		&& incoming[i+1]==m_sentinelValue
		&& incoming[i+2]==m_sentinelValue){
			(m_ed->m_EXTENSION_reads_received)=true;
		}else{
			int rank=incoming[i];
			int index=incoming[i+1];
			char strand=incoming[i+2];
			ReadAnnotation e;
			e.constructor(rank,index,strand);
			m_ed->m_EXTENSION_receivedReads.push_back(e);
		}
	}
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	char strand=incoming[2];
	uint64_t vertex=(*m_myReads)[incoming[0]]->getVertex(incoming[1],(*m_wordSize),strand,(*m_colorSpaceMode));
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
	message2[0]=vertex;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY(Message*message){
	void*buffer=message->getBuffer();
	(m_ed->m_EXTENSION_read_vertex_received)=true;
	(m_ed->m_EXTENSION_receivedReadVertex)=((uint64_t*)buffer)[0];
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_LENGTH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int length=(*m_myReads)[incoming[0]]->length();
	
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
	message2[0]=length;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_ed->m_EXTENSION_readLength_received)=true;
	(m_ed->m_EXTENSION_receivedLength)=incoming[0];
}

void MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY(Message*message){
	call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION(message);
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	for(int i=0;i<count;i+=3){
		SplayNode<uint64_t,Vertex>*node=m_subgraph->find(incoming[i+0]);
		#ifdef ASSERT
		assert(node!=NULL);
		#endif
		int wave=incoming[i+1];
		int progression=incoming[i+2];
		node->getValue()->addDirection(wave,progression,&(*m_directionsAllocator));
	}
	
}

void MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY(Message*message){
	m_fusionData->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_ASSEMBLE_WAVES(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_ASSEMBLE_WAVES;
	(m_seedingData->m_SEEDING_i)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REVERSE(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE(Message*message){
	(m_ed->m_EXTENSION_currentRankIsDone)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_START_FUSION(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_FUSION;
	(m_seedingData->m_SEEDING_i)=0;

	m_fusionData->m_FUSION_direct_fusionDone=false;
	m_fusionData->m_FUSION_first_done=false;
	m_fusionData->m_FUSION_paths_requested=false;
}

void MessageProcessor::call_RAY_MPI_TAG_FUSION_DONE(Message*message){
	m_fusionData->m_FUSION_numberOfRanksDone++;
	if(m_fusionData->m_FUSION_numberOfRanksDone==size && !(*m_isFinalFusion)){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	SplayNode<uint64_t,Vertex>*node=m_subgraph->find(incoming[0]);
	#ifdef ASSERT
	assert(node!=NULL);
	#endif
	vector<Direction> paths=node->getValue()->getDirections();
	m_fusionData->m_FUSION_cachedDirections[source]=paths;
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
	message2[0]=paths.size();
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FUSION_paths_received=true;
	m_fusionData->m_FUSION_receivedPaths.clear();
	m_fusionData->m_FUSION_numberOfPaths=incoming[0];
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int id=incoming[0];
	int length=0;
	#ifdef ASSERT
	assert(m_fusionData->m_FUSION_identifier_map.count(id)>0);
	#endif
	if(m_fusionData->m_FUSION_identifier_map.count(id)>0){
		length=(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size();
	}

	#ifdef ASSERT
	assert(length>0);
	#endif
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
	message2[0]=length;
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FUSION_receivedLength=incoming[0];
	m_fusionData->m_FUSION_pathLengthReceived=true;
}

void MessageProcessor::call_RAY_MPI_TAG_CALIBRATION_MESSAGE(Message*message){
}
void MessageProcessor::call_RAY_MPI_TAG_BEGIN_CALIBRATION(Message*message){
}
void MessageProcessor::call_RAY_MPI_TAG_END_CALIBRATION(Message*message){
}
void MessageProcessor::call_RAY_MPI_TAG_COMMUNICATION_STABILITY_MESSAGE(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int i=incoming[0];
	Direction d=m_fusionData->m_FUSION_cachedDirections[source][i];
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message2[0]=d.getWave();
	message2[1]=d.getProgression();
	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FUSION_path_received=true;
	int pathId=incoming[0];
	int position=incoming[1];
	m_fusionData->m_FUSION_receivedPath.constructor(pathId,position);
}

void MessageProcessor::call_RAY_MPI_TAG_INDEX_PAIRED_SEQUENCE_REPLY(Message*message){
	m_sequencesLoader->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_INDEX_PAIRED_SEQUENCE(Message*message){
	int count=message->getCount();
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	for(int i=0;i<count;i+=4){
		int currentReadId=incoming[i+0];
		int otherRank=incoming[i+1];
		int otherId=incoming[i+2];
		int library=incoming[i+3];
		PairedRead*t=(PairedRead*)(*m_persistentAllocator).allocate(sizeof(PairedRead));
		#ifdef ASSERT
		assert(otherRank<size);
		#endif

		t->constructor(otherRank,otherId,library);

		#ifdef ASSERT
		if(currentReadId>=(int)m_myReads->size()){
			cout<<"currentReadId="<<currentReadId<<" size="<<m_myReads->size()<<endl;
		}
		assert(currentReadId<(int)m_myReads->size());
		#endif

		(*m_myReads)[currentReadId]->setPairedRead(t);
	}
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_INDEX_PAIRED_SEQUENCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_HAS_PAIRED_READ(Message*message){
	int source=message->getSource();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	int index=incoming[0];
	#ifdef ASSERT
	assert(index<(int)m_myReads->size());
	#endif
	message2[0]=(*m_myReads)[index]->hasPairedRead();
	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_ed->m_EXTENSION_hasPairedReadAnswer)=incoming[0];
	(m_ed->m_EXTENSION_hasPairedReadReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PAIRED_READ(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int index=incoming[0];
	#ifdef ASSERT
	assert(index<(int)m_myReads->size());
	#endif

	PairedRead*t=(*m_myReads)[index]->getPairedRead();
	PairedRead dummy;
	dummy.constructor(0,0,DUMMY_LIBRARY);
	if(t==NULL){
		t=&dummy;
	}
	#ifdef ASSERT
	assert(t!=NULL);
	#endif
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(3*sizeof(uint64_t));
	message2[0]=t->getRank();
	message2[1]=t->getId();
	message2[2]=t->getLibrary();
	Message aMessage(message2,3,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_GET_PAIRED_READ_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PAIRED_READ_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_ed->m_EXTENSION_pairedRead).constructor(incoming[0],incoming[1],incoming[2]);
	(m_ed->m_EXTENSION_pairedSequenceReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_CLEAR_DIRECTIONS(Message*message){
	int source=message->getSource();
	// clearing old data too!.
	m_fusionData->m_FINISH_pathLengths.clear();

	//cout<<"Rank "<<rank<<" is clearing its directions"<<endl;
	// clear graph
	for(int i=0;i<m_subgraph->getNumberOfTrees();i++){
		SplayTreeIterator<uint64_t,Vertex> iterator(m_subgraph->getTree(i));
		while(iterator.hasNext()){
			iterator.next()->getValue()->clearDirections();
		}
	}
	(*m_directionsAllocator).clear();
	(*m_directionsAllocator).constructor(PERSISTENT_ALLOCATOR_CHUNK_SIZE);


	// add the FINISHING bits
	for(int i=0;i<(int)m_fusionData->m_FINISH_newFusions.size();i++){
		#ifdef SHOW_PROGRESS
		#endif
		(m_ed->m_EXTENSION_contigs).push_back((m_fusionData->m_FINISH_newFusions)[i]);
	}

	m_fusionData->m_FINISH_newFusions.clear();


	vector<vector<uint64_t> > fusions;
	for(int i=0;i<(int)(m_ed->m_EXTENSION_contigs).size();i++){
		int id=(m_ed->m_EXTENSION_identifiers)[i];
		if(m_fusionData->m_FUSION_eliminated.count(id)==0){
			fusions.push_back((m_ed->m_EXTENSION_contigs)[i]);
			vector<uint64_t> rc;
			for(int j=(m_ed->m_EXTENSION_contigs)[i].size()-1;j>=0;j--){
				rc.push_back(complementVertex((m_ed->m_EXTENSION_contigs)[i][j],*m_wordSize,(*m_colorSpaceMode)));
			}
			fusions.push_back(rc);
		}
	}

	(m_ed->m_EXTENSION_identifiers).clear();
	m_fusionData->m_FUSION_eliminated.clear();
	for(int i=0;i<(int)fusions.size();i++){
		int id=i*MAX_NUMBER_OF_MPI_PROCESSES+rank;
		#ifdef ASSERT
		assert(rank<size);
		assert(rank>=0);
		assert(size>=1);
		assert((id%MAX_NUMBER_OF_MPI_PROCESSES)<size);
		#endif
		(m_ed->m_EXTENSION_identifiers).push_back(id);
	}

	for(int i=0;i<(int)(m_ed->m_EXTENSION_identifiers).size();i++){
		int id=(m_ed->m_EXTENSION_identifiers)[i];
		m_fusionData->m_FUSION_identifier_map[id]=i;
	}

	(m_ed->m_EXTENSION_contigs).clear();
	(m_ed->m_EXTENSION_contigs)=fusions;

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY(Message*message){
	(*m_CLEAR_n)++;
}

void MessageProcessor::call_RAY_MPI_TAG_FINISH_FUSIONS(Message*message){
	//cout<<"Rank "<<rank<<" call_RAY_MPI_TAG_FINISH_FUSIONS"<<endl;
	(*m_mode)=RAY_SLAVE_MODE_FINISH_FUSIONS;
	m_fusionData->m_FINISH_fusionOccured=false;
	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
	m_fusionData->m_FUSION_first_done=false;
	m_fusionData->m_Machine_getPaths_INITIALIZED=false;
	m_fusionData->m_Machine_getPaths_DONE=false;
}

void MessageProcessor::call_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(*m_FINISH_n)++;
	if(incoming[0]){
		(*m_nextReductionOccured)=true;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS;
	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
}

void MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY(Message*message){
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED(Message*message){
	(*m_DISTRIBUTE_n)++;
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_START(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	vector<uint64_t> a;
	(*m_allPaths).push_back(a);
	int id=incoming[0];
	#ifdef ASSERT
	int rank=id%MAX_NUMBER_OF_MPI_PROCESSES;
	assert(rank<size);
	#endif
	(*m_identifiers).push_back(id);
	(*m_allIdentifiers)[id]=m_identifiers->size()-1;

}

void MessageProcessor::call_RAY_MPI_TAG_ELIMINATE_PATH(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FUSION_eliminated.insert(incoming[0]);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_VERTEX(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int id=incoming[0];
	int position=incoming[1];
	#ifdef ASSERT
	assert(m_fusionData->m_FUSION_identifier_map.count(id)>0);
	#endif
	#ifdef ASSERT
	if(position>=(int)(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size()){
		cout<<"Pos="<<position<<" Length="<<(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size()<<endl;
	}
	assert(position<(int)(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size());
	#endif
	uint64_t*messageBytes=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
	messageBytes[0]=(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]][position];
	Message aMessage(messageBytes,1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FINISH_vertex_received=true;
	m_fusionData->m_FINISH_received_vertex=incoming[0];
}

void MessageProcessor::call_RAY_MPI_TAG_SET_COLOR_MODE(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(*m_colorSpaceMode)=incoming[0];
}

void MessageProcessor::call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION;
	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
	m_ed->m_EXTENSION_reads_requested=false;
}

void MessageProcessor::call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE(Message*message){
	(*m_numberOfRanksDoneDetectingDistances)++;
	if((*m_numberOfRanksDoneDetectingDistances)==size){
		(*m_master_mode)=RAY_MASTER_MODE_ASK_DISTANCES;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY(Message*message){
	m_library->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_LIBRARY_DISTANCE(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	for(int i=0;i<count;i+=3){
		parameters->addDistance(incoming[i+0],incoming[i+1],incoming[i+2]);
		//cout<<"SourceSays "<<message->getSource()<<" "<<incoming[i+0]<<" "<<incoming[i+1]<<" "<<incoming[i+2]<<endl;
		//fflush(stdout);
	}
	//cout<<"Received "<<count/3<<" lengths from "<<message->getSource()<<endl;

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES;
	m_library->allocateBuffers();
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED(Message*message){
	(*m_numberOfRanksDoneSendingDistances)++;
	//cout<<"RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED "<<(*m_numberOfRanksDoneSendingDistances)<<endl;
	if((*m_numberOfRanksDoneSendingDistances)==size){
		(*m_master_mode)=RAY_MASTER_MODE_START_UPDATING_DISTANCES;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	int*data=(int*)incoming;
	int libraries=data[0];
	for(int i=0;i<libraries;i++){
		int average=data[2*i+1+0];
		int deviation=data[2*i+1+1];
		parameters->addLibraryData(i,average,deviation);
	}
}

void MessageProcessor::call_RAY_MPI_TAG_RECEIVED_COVERAGE_INFORMATION(Message*message){
	(*m_numberOfRanksWithCoverageData)++;
	if((*m_numberOfRanksWithCoverageData)==size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_EDGES;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int index=incoming[0];
	#ifdef ASSERT
	assert(index<(int)m_myReads->size());
	#endif
	PairedRead*t=(*m_myReads)[index]->getPairedRead();
	PairedRead dummy;
	dummy.constructor(0,0,DUMMY_LIBRARY);
	if(t==NULL){
		t=&dummy;
	}
	#ifdef ASSERT
	assert(t!=NULL);
	#endif
	char*seq=m_myReads->at(index)->getSeq();

	int beforeRounding=3*sizeof(uint64_t)+strlen(seq)+1;
	int toAllocate=roundNumber(beforeRounding,sizeof(uint64_t));
	//cout<<" seq is "<<strlen(seq)<<" +1 +4*8="<<beforeRounding<<", rounded: "<<toAllocate<<endl;

	uint64_t*messageBytes=(uint64_t*)m_outboxAllocator->allocate(toAllocate);
	messageBytes[0]=t->getRank();
	messageBytes[1]=t->getId();
	messageBytes[2]=t->getLibrary();
	char*dest=(char*)(messageBytes+3);
	strcpy(dest,seq);
	//cout<<"dest="<<dest<<endl;
	Message aMessage(messageBytes,toAllocate/sizeof(uint64_t),MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_ed->m_EXTENSION_pairedRead).constructor(incoming[0],incoming[1],incoming[2]);
	(m_ed->m_EXTENSION_pairedSequenceReceived)=true;

	seedExtender->m_receivedString=(char*)(incoming+3);
	seedExtender->m_sequenceReceived=true;
}

void MessageProcessor::constructor(
MessagesHandler*m_messagesHandler,
SeedingData*seedingData,
Library*m_library,
bool*m_ready,
VerticesExtractor*m_verticesExtractor,
EdgesExtractor*m_edgesExtractor,
SequencesLoader*sequencesLoader,ExtensionData*ed,
			int*m_numberOfRanksDoneDetectingDistances,
			int*m_numberOfRanksDoneSendingDistances,
			Parameters*parameters,
			MyForest*m_subgraph,
			RingAllocator*m_outboxAllocator,
				int rank,
			int*m_numberOfMachinesDoneSendingEdges,
			FusionData*m_fusionData,
			int*m_wordSize,
			int*m_minimumCoverage,
			int*m_seedCoverage,
			int*m_peakCoverage,
			ArrayOfReads*m_myReads,
		int size,
	RingAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<int>*m_identifiers,
	bool*m_mode_sendDistribution,
	bool*m_alive,
	bool*m_colorSpaceMode,
	int*m_mode,
	vector<vector<uint64_t> >*m_allPaths,
	int*m_last_value,
	int*m_ranksDoneAttachingReads,
	int*m_DISTRIBUTE_n,
	int*m_numberOfRanksDoneSeeding,
	int*m_CLEAR_n,
	int*m_readyToSeed,
	int*m_FINISH_n,
	bool*m_nextReductionOccured,
	MyAllocator*m_directionsAllocator,
	int*m_mode_send_coverage_iterator,
	map<int,uint64_t>*m_coverageDistribution,
	int*m_sequence_ready_machines,
	int*m_numberOfMachinesReadyForEdgesDistribution,
	int*m_numberOfMachinesReadyToSendDistribution,
	bool*m_mode_send_outgoing_edges,
	int*m_mode_send_vertices_sequence_id,
	bool*m_mode_send_vertices,
	int*m_numberOfMachinesDoneSendingVertices,
	int*m_numberOfMachinesDoneSendingCoverage,
				StaticVector*m_outbox,
		map<int,int>*m_allIdentifiers,OpenAssemblerChooser*m_oa,
int*m_numberOfRanksWithCoverageData,
SeedExtender*seedExtender,int*m_master_mode,
bool*m_isFinalFusion,
SequencesIndexer*m_si){
	this->m_sequencesLoader=sequencesLoader;
	this->m_verticesExtractor=m_verticesExtractor;
	this->m_ed=ed;
	this->m_numberOfRanksDoneDetectingDistances=m_numberOfRanksDoneDetectingDistances;
	this->m_numberOfRanksDoneSendingDistances=m_numberOfRanksDoneSendingDistances;
	this->parameters=parameters;
	this->m_library=m_library;
	this->m_subgraph=m_subgraph;
	this->m_edgesExtractor=m_edgesExtractor;
	this->m_outboxAllocator=m_outboxAllocator;
	this->rank=rank;
	this->m_numberOfMachinesDoneSendingEdges=m_numberOfMachinesDoneSendingEdges;
	this->m_fusionData=m_fusionData;
	this->m_wordSize=m_wordSize;
	this->m_minimumCoverage=m_minimumCoverage;
	this->m_seedCoverage=m_seedCoverage;
	this->m_peakCoverage=m_peakCoverage;
	this->m_myReads=m_myReads;
	this->size=size;
	this->m_inboxAllocator=m_inboxAllocator;
	this->m_si=m_si;
	this->m_persistentAllocator=m_persistentAllocator;
	this->m_identifiers=m_identifiers;
	this->m_mode_sendDistribution=m_mode_sendDistribution;
	this->m_alive=m_alive;
	this->m_colorSpaceMode=m_colorSpaceMode;
	this->m_messagesHandler=m_messagesHandler;
	this->m_mode=m_mode;
	this->m_allPaths=m_allPaths;
	this->m_last_value=m_last_value;
	this->m_ranksDoneAttachingReads=m_ranksDoneAttachingReads;
	this->m_DISTRIBUTE_n=m_DISTRIBUTE_n;
	this->m_numberOfRanksDoneSeeding=m_numberOfRanksDoneSeeding;
	this->m_CLEAR_n=m_CLEAR_n;
	this->m_readyToSeed=m_readyToSeed;
	this->m_FINISH_n=m_FINISH_n;
	this->m_nextReductionOccured=m_nextReductionOccured;
	this->m_directionsAllocator=m_directionsAllocator;
	this->m_mode_send_coverage_iterator=m_mode_send_coverage_iterator;
	this->m_coverageDistribution=m_coverageDistribution;
	this->m_sequence_ready_machines=m_sequence_ready_machines;
	this->m_numberOfMachinesReadyForEdgesDistribution=m_numberOfMachinesReadyForEdgesDistribution;
	this->m_numberOfMachinesReadyToSendDistribution=m_numberOfMachinesReadyToSendDistribution;
	this->m_mode_send_outgoing_edges=m_mode_send_outgoing_edges;
	this->m_mode_send_vertices_sequence_id=m_mode_send_vertices_sequence_id;
	this->m_mode_send_vertices=m_mode_send_vertices;
	this->m_numberOfMachinesDoneSendingVertices=m_numberOfMachinesDoneSendingVertices;
	this->m_numberOfMachinesDoneSendingCoverage=m_numberOfMachinesDoneSendingCoverage;
	this->m_outbox=m_outbox;
	this->m_allIdentifiers=m_allIdentifiers,
	this->m_oa=m_oa;
	this->m_numberOfRanksWithCoverageData=m_numberOfRanksWithCoverageData;
	this->seedExtender=seedExtender;
	this->m_master_mode=m_master_mode;
	this->m_isFinalFusion=m_isFinalFusion;
	this->m_ready=m_ready;
	m_seedingData=seedingData;

	m_buffersForOutgoingEdgesToDelete.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_buffersForIngoingEdgesToDelete.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
}

void MessageProcessor::flushBuffers(){
	m_buffersForIngoingEdgesToDelete.flushAll(RAY_MPI_TAG_DELETE_INGOING_EDGE,m_outboxAllocator,m_outbox,rank);
	m_buffersForOutgoingEdgesToDelete.flushAll(RAY_MPI_TAG_DELETE_OUTGOING_EDGE,m_outboxAllocator,m_outbox,rank);
}

MessageProcessor::MessageProcessor(){
	m_last=time(NULL);
	m_consumed=0;
	m_sentinelValue=0;
	m_sentinelValue--;// overflow it in an obvious manner
	
	assignHandlers();
}

void MessageProcessor::assignHandlers(){
	m_methods[RAY_MPI_TAG_WELCOME]=&MessageProcessor::call_RAY_MPI_TAG_WELCOME;
	m_methods[RAY_MPI_TAG_SEND_SEQUENCE]=&MessageProcessor::call_RAY_MPI_TAG_SEND_SEQUENCE;
	m_methods[RAY_MPI_TAG_SEND_SEQUENCE_REGULATOR]=&MessageProcessor::call_RAY_MPI_TAG_SEND_SEQUENCE_REGULATOR;
	m_methods[RAY_MPI_TAG_SEND_SEQUENCE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_SEND_SEQUENCE_REPLY;
	m_methods[RAY_MPI_TAG_SEQUENCES_READY]=&MessageProcessor::call_RAY_MPI_TAG_SEQUENCES_READY;
	m_methods[RAY_MPI_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS]=&MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS;
	m_methods[RAY_MPI_TAG_VERTICES_DATA]=&MessageProcessor::call_RAY_MPI_TAG_VERTICES_DATA;
	m_methods[RAY_MPI_TAG_VERTICES_DISTRIBUTED]=&MessageProcessor::call_RAY_MPI_TAG_VERTICES_DISTRIBUTED;
	m_methods[RAY_MPI_TAG_OUT_EDGES_DATA]=&MessageProcessor::call_RAY_MPI_TAG_OUT_EDGES_DATA;
	m_methods[RAY_MPI_TAG_START_VERTICES_DISTRIBUTION]=&MessageProcessor::call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION;
	m_methods[RAY_MPI_TAG_EDGES_DISTRIBUTED]=&MessageProcessor::call_RAY_MPI_TAG_EDGES_DISTRIBUTED;
	m_methods[RAY_MPI_TAG_IN_EDGES_DATA]=&MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA;
	m_methods[RAY_MPI_TAG_START_EDGES_DISTRIBUTION]=&MessageProcessor::call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION;
	m_methods[RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ASK]=&MessageProcessor::call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ASK;
	m_methods[RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ANSWER]=&MessageProcessor::call_RAY_MPI_TAG_START_EDGES_DISTRIBUTION_ANSWER;
	m_methods[RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION]=&MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION;
	m_methods[RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER]=&MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER;
	m_methods[RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION]=&MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION;
	m_methods[RAY_MPI_TAG_COVERAGE_DATA]=&MessageProcessor::call_RAY_MPI_TAG_COVERAGE_DATA;
	m_methods[RAY_MPI_TAG_COVERAGE_END]=&MessageProcessor::call_RAY_MPI_TAG_COVERAGE_END;
	m_methods[RAY_MPI_TAG_SEND_COVERAGE_VALUES]=&MessageProcessor::call_RAY_MPI_TAG_SEND_COVERAGE_VALUES;
	m_methods[RAY_MPI_TAG_READY_TO_SEED]=&MessageProcessor::call_RAY_MPI_TAG_READY_TO_SEED;
	m_methods[RAY_MPI_TAG_START_SEEDING]=&MessageProcessor::call_RAY_MPI_TAG_START_SEEDING;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_KEY_AND_COVERAGE_REPLY;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY;
	m_methods[RAY_MPI_TAG_SEEDING_IS_OVER]=&MessageProcessor::call_RAY_MPI_TAG_SEEDING_IS_OVER;
	m_methods[RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON]=&MessageProcessor::call_RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON;
	m_methods[RAY_MPI_TAG_I_GO_NOW]=&MessageProcessor::call_RAY_MPI_TAG_I_GO_NOW;
	m_methods[RAY_MPI_TAG_SET_WORD_SIZE]=&MessageProcessor::call_RAY_MPI_TAG_SET_WORD_SIZE;
	m_methods[RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS]=&MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS;
	m_methods[RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY;
	m_methods[RAY_MPI_TAG_EXTENSION_IS_DONE]=&MessageProcessor::call_RAY_MPI_TAG_EXTENSION_IS_DONE;
	m_methods[RAY_MPI_TAG_ASK_EXTENSION]=&MessageProcessor::call_RAY_MPI_TAG_ASK_EXTENSION;
	m_methods[RAY_MPI_TAG_ASK_IS_ASSEMBLED]=&MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED;
	m_methods[RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY;
	m_methods[RAY_MPI_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY_END]=&MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY_END;
	m_methods[RAY_MPI_TAG_MARK_AS_ASSEMBLED]=&MessageProcessor::call_RAY_MPI_TAG_MARK_AS_ASSEMBLED;
	m_methods[RAY_MPI_TAG_ASK_EXTENSION_DATA]=&MessageProcessor::call_RAY_MPI_TAG_ASK_EXTENSION_DATA;
	m_methods[RAY_MPI_TAG_EXTENSION_DATA]=&MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA;
	m_methods[RAY_MPI_TAG_EXTENSION_DATA_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA_REPLY;
	m_methods[RAY_MPI_TAG_EXTENSION_END]=&MessageProcessor::call_RAY_MPI_TAG_EXTENSION_END;
	m_methods[RAY_MPI_TAG_EXTENSION_DATA_END]=&MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA_END;
	m_methods[RAY_MPI_TAG_ATTACH_SEQUENCE]=&MessageProcessor::call_RAY_MPI_TAG_ATTACH_SEQUENCE;
	m_methods[RAY_MPI_TAG_REQUEST_READS]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_READS;
	m_methods[RAY_MPI_TAG_REQUEST_READS_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_READS_REPLY;
	m_methods[RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION]=&MessageProcessor::call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION;
	m_methods[RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY;
	m_methods[RAY_MPI_TAG_ASK_READ_LENGTH]=&MessageProcessor::call_RAY_MPI_TAG_ASK_READ_LENGTH;
	m_methods[RAY_MPI_TAG_ASK_READ_LENGTH_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY;
	m_methods[RAY_MPI_TAG_SAVE_WAVE_PROGRESSION]=&MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION;
	m_methods[RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY;
	m_methods[RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY;
	m_methods[RAY_MPI_TAG_ASSEMBLE_WAVES]=&MessageProcessor::call_RAY_MPI_TAG_ASSEMBLE_WAVES;
	m_methods[RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REVERSE]=&MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REVERSE;
	m_methods[RAY_MPI_TAG_ASSEMBLE_WAVES_DONE]=&MessageProcessor::call_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE;
	m_methods[RAY_MPI_TAG_START_FUSION]=&MessageProcessor::call_RAY_MPI_TAG_START_FUSION;
	m_methods[RAY_MPI_TAG_FUSION_DONE]=&MessageProcessor::call_RAY_MPI_TAG_FUSION_DONE;
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	m_methods[RAY_MPI_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY]=&MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY;
	m_methods[RAY_MPI_TAG_GET_PATH_LENGTH]=&MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH;
	m_methods[RAY_MPI_TAG_VERTICES_DATA_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_VERTICES_DATA_REPLY;
	m_methods[RAY_MPI_TAG_GET_PATH_LENGTH_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY;
	m_methods[RAY_MPI_TAG_CALIBRATION_MESSAGE]=&MessageProcessor::call_RAY_MPI_TAG_CALIBRATION_MESSAGE;
	m_methods[RAY_MPI_TAG_BEGIN_CALIBRATION]=&MessageProcessor::call_RAY_MPI_TAG_BEGIN_CALIBRATION;
	m_methods[RAY_MPI_TAG_END_CALIBRATION]=&MessageProcessor::call_RAY_MPI_TAG_END_CALIBRATION;
	m_methods[RAY_MPI_TAG_COMMUNICATION_STABILITY_MESSAGE]=&MessageProcessor::call_RAY_MPI_TAG_COMMUNICATION_STABILITY_MESSAGE;
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATH]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH;
	m_methods[RAY_MPI_TAG_SHOW_SEQUENCES]=&MessageProcessor::call_RAY_MPI_TAG_SHOW_SEQUENCES;
	m_methods[RAY_MPI_TAG_BARRIER]=&MessageProcessor::call_RAY_MPI_TAG_BARRIER;
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY;
	m_methods[RAY_MPI_TAG_INDEX_PAIRED_SEQUENCE]=&MessageProcessor::call_RAY_MPI_TAG_INDEX_PAIRED_SEQUENCE;
	m_methods[RAY_MPI_TAG_INDEX_PAIRED_SEQUENCE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_INDEX_PAIRED_SEQUENCE_REPLY;
	m_methods[RAY_MPI_TAG_HAS_PAIRED_READ]=&MessageProcessor::call_RAY_MPI_TAG_HAS_PAIRED_READ;
	m_methods[RAY_MPI_TAG_HAS_PAIRED_READ_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_HAS_PAIRED_READ_REPLY;
	m_methods[RAY_MPI_TAG_GET_PAIRED_READ]=&MessageProcessor::call_RAY_MPI_TAG_GET_PAIRED_READ;
	m_methods[RAY_MPI_TAG_GET_PAIRED_READ_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_GET_PAIRED_READ_REPLY;
	m_methods[RAY_MPI_TAG_START_INDEXING_SEQUENCES]=&MessageProcessor::call_RAY_MPI_TAG_START_INDEXING_SEQUENCES;
	m_methods[RAY_MPI_TAG_CLEAR_DIRECTIONS]=&MessageProcessor::call_RAY_MPI_TAG_CLEAR_DIRECTIONS;
	m_methods[RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY;
	m_methods[RAY_MPI_TAG_FINISH_FUSIONS]=&MessageProcessor::call_RAY_MPI_TAG_FINISH_FUSIONS;
	m_methods[RAY_MPI_TAG_FINISH_FUSIONS_FINISHED]=&MessageProcessor::call_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED;
	m_methods[RAY_MPI_TAG_DISTRIBUTE_FUSIONS]=&MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS;
	m_methods[RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED]=&MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED;
	m_methods[RAY_MPI_TAG_EXTENSION_START]=&MessageProcessor::call_RAY_MPI_TAG_EXTENSION_START;
	m_methods[RAY_MPI_TAG_ELIMINATE_PATH]=&MessageProcessor::call_RAY_MPI_TAG_ELIMINATE_PATH;
	m_methods[RAY_MPI_TAG_GET_PATH_VERTEX]=&MessageProcessor::call_RAY_MPI_TAG_GET_PATH_VERTEX;
	m_methods[RAY_MPI_TAG_GET_PATH_VERTEX_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY;
	m_methods[RAY_MPI_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY]=&MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY;
	m_methods[RAY_MPI_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY]=&MessageProcessor::call_RAY_MPI_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY;
	m_methods[RAY_MPI_TAG_SET_COLOR_MODE]=&MessageProcessor::call_RAY_MPI_TAG_SET_COLOR_MODE;
	m_methods[RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION]=&MessageProcessor::call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION;
	m_methods[RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE]=&MessageProcessor::call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE;
	m_methods[RAY_MPI_TAG_LIBRARY_DISTANCE]=&MessageProcessor::call_RAY_MPI_TAG_LIBRARY_DISTANCE;
	m_methods[RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY;
	m_methods[RAY_MPI_TAG_ASK_LIBRARY_DISTANCES]=&MessageProcessor::call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES;
	m_methods[RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED]=&MessageProcessor::call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED;
	m_methods[RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION]=&MessageProcessor::call_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION;
	m_methods[RAY_MPI_TAG_RECEIVED_COVERAGE_INFORMATION]=&MessageProcessor::call_RAY_MPI_TAG_RECEIVED_COVERAGE_INFORMATION;
	m_methods[RAY_MPI_TAG_REQUEST_READ_SEQUENCE]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE;
	m_methods[RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY;
	m_methods[RAY_MPI_TAG_IN_EDGES_DATA_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY;
	m_methods[RAY_MPI_TAG_OUT_EDGES_DATA_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY;
	m_methods[RAY_MPI_TAG_RECEIVED_MESSAGES]=&MessageProcessor::call_RAY_MPI_TAG_RECEIVED_MESSAGES;
	m_methods[RAY_MPI_TAG_RECEIVED_MESSAGES_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_RECEIVED_MESSAGES_REPLY;
	m_methods[RAY_MPI_TAG_MUST_RUN_REDUCER]=&MessageProcessor::call_RAY_MPI_TAG_MUST_RUN_REDUCER;
	m_methods[RAY_MPI_TAG_ASK_BEGIN_REDUCTION_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_BEGIN_REDUCTION_REPLY;
	m_methods[RAY_MPI_TAG_ASK_BEGIN_REDUCTION]=&MessageProcessor::call_RAY_MPI_TAG_ASK_BEGIN_REDUCTION;
	m_methods[RAY_MPI_TAG_RESUME_VERTEX_DISTRIBUTION]=&MessageProcessor::call_RAY_MPI_TAG_RESUME_VERTEX_DISTRIBUTION;
	m_methods[RAY_MPI_TAG_REDUCE_MEMORY_CONSUMPTION_DONE]=&MessageProcessor::call_RAY_MPI_TAG_REDUCE_MEMORY_CONSUMPTION_DONE;
	m_methods[RAY_MPI_TAG_START_REDUCTION]=&MessageProcessor::call_RAY_MPI_TAG_START_REDUCTION;
	m_methods[RAY_MPI_TAG_VERIFY_INGOING_EDGES]=&MessageProcessor::call_RAY_MPI_TAG_VERIFY_INGOING_EDGES;
	m_methods[RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY;
	m_methods[RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY;
	m_methods[RAY_MPI_TAG_VERIFY_OUTGOING_EDGES]=&MessageProcessor::call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES;
	m_methods[RAY_MPI_TAG_VERIFY_INGOING_EDGES_FORCE]=&MessageProcessor::call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_FORCE;
	m_methods[RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY_FORCE]=&MessageProcessor::call_RAY_MPI_TAG_VERIFY_INGOING_EDGES_REPLY_FORCE;
	m_methods[RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY_FORCE]=&MessageProcessor::call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_REPLY_FORCE;
	m_methods[RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_FORCE]=&MessageProcessor::call_RAY_MPI_TAG_VERIFY_OUTGOING_EDGES_FORCE;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_EDGES]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES;
	m_methods[RAY_MPI_TAG_DELETE_VERTICES]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTICES;
	m_methods[RAY_MPI_TAG_DELETE_VERTICES_DONE]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTICES_DONE;
	m_methods[RAY_MPI_TAG_UPDATE_THRESHOLD]=&MessageProcessor::call_RAY_MPI_TAG_UPDATE_THRESHOLD;
	m_methods[RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY;
	m_methods[RAY_MPI_TAG_DELETE_VERTEX]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX;
	m_methods[RAY_MPI_TAG_DELETE_INGOING_EDGE]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_INGOING_EDGE;
	m_methods[RAY_MPI_TAG_DELETE_OUTGOING_EDGE]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_OUTGOING_EDGE;
	m_methods[RAY_MPI_TAG_DELETE_VERTEX_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX_REPLY;
}


