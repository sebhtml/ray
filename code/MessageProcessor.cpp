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

void MessageProcessor::call_RAY_MPI_TAG_LOAD_SEQUENCES(Message*message){
	uint32_t*incoming=(uint32_t*)message->getBuffer();
	for(int i=0;i<(int)incoming[0];i++){
		parameters->setNumberOfSequences(incoming[1+i]);
	}
	(*m_mode)=RAY_SLAVE_MODE_LOAD_SEQUENCES;
}

void MessageProcessor::processMessage(Message*message){
	int tag=message->getTag();
	FNMETHOD f=m_methods[tag];
	(this->*f)(message);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_READ_MATE_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_GET_READ_MATE(Message*message){
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int j=0;
	uint64_t*buffer=(uint64_t*)message->getBuffer();
	for(int i=0;i<message->getCount();i++){
		int readId=buffer[i];
		Read*read=m_myReads->at(readId);
		int readLength=read->length();
		outgoingMessage[j++]=readLength;
		if(!read->hasPairedRead()){
			outgoingMessage[j++]=-1;
			outgoingMessage[j++]=-1;
			outgoingMessage[j++]=-1;
		}else{
			PairedRead*mate=read->getPairedRead();
			outgoingMessage[j++]=mate->getRank();
			outgoingMessage[j++]=mate->getId();
			outgoingMessage[j++]=mate->getLibrary();
		}
	}

	Message aMessage(outgoingMessage,j,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_GET_READ_MATE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_READS(Message*message){
	//int count=message->getCount();
	uint64_t*buffer=(uint64_t*)message->getBuffer();
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

	int j=0;
	for(int i=0;i<message->getCount();i+=5){
		uint64_t vertex=buffer[i];
		ReadAnnotation*ptr=(ReadAnnotation*)buffer[i+1];
		if(ptr==NULL){
			ptr=m_subgraph->getReads(vertex);
		}
	
		if(ptr==NULL){
			outgoingMessage[j+1]=INVALID_RANK;
		}else{
			int rank=ptr->getRank();
			#ifdef ASSERT
			assert(rank>=0&&rank<parameters->getSize());
			#endif
			outgoingMessage[j+1]=rank;
			outgoingMessage[j+2]=ptr->getReadIndex();
			outgoingMessage[j+3]=ptr->getPositionOnStrand();
			outgoingMessage[j+4]=ptr->getStrand();
			ptr=ptr->getNext();
		}
		outgoingMessage[j]=(uint64_t)ptr;
		j+=5;
	}
	Message aMessage(outgoingMessage,j,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_SET_WORD_SIZE(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(*m_wordSize)=incoming[0];
	(*m_colorSpaceMode)=incoming[1];
	m_subgraph->setWordSize(*m_wordSize);
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX(Message*message){
	#ifdef ASSERT
	assert(!m_subgraph->frozen());
	#endif

	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	for(int i=0;i<count;i++){
		uint64_t vertex=incoming[i];
		Vertex*node=m_subgraph->find(vertex);

		if(node==NULL){
			continue;
		}

		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		// delete the vertex
		m_subgraph->remove(vertex);

		#ifdef ASSERT
		assert(m_subgraph->find(vertex)==NULL);
		#endif
	}
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_DELETE_VERTEX_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	for(int i=0;i<count;i+=2){
		Vertex*node=m_subgraph->find(incoming[i]);
		#ifdef ASSERT
		if(node==NULL){
			cout<<__func__<<" does not exist: "<<idToWord(incoming[i],*m_wordSize)<<endl;
		}
		assert(node!=NULL);
		#endif
		outgoingMessage[i]=node->getEdges(incoming[i]);
		outgoingMessage[i+1]=node->getCoverage(incoming[i]);
	}
	
	Message aMessage(outgoingMessage,count,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_CHECK_VERTEX(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	
	int outgoingCount=0;
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	for(int i=0;i<count;i+=2){
		int task=incoming[i+0];
		uint64_t vertex=incoming[i+1];
		Vertex*node=m_subgraph->find(vertex);
		if(node!=NULL){
			int parents=node->getIngoingEdges(vertex,(*m_wordSize)).size();
			int children=node->getOutgoingEdges(vertex,(*m_wordSize)).size();
			int coverage=node->getCoverage(vertex);
			if(parents>0&&children>0&&coverage>3){
				outgoingMessage[outgoingCount++]=task;
				outgoingMessage[outgoingCount++]=coverage;
			}
		}
	}

	Message aMessage(outgoingMessage,outgoingCount,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_CHECK_VERTEX_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_CHECK_VERTEX_REPLY(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	m_reducer->processConfetti(incoming,count);
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_INGOING_EDGE(Message*message){
	#ifdef ASSERT
	assert(!m_subgraph->frozen());
	#endif

	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	for(int i=0;i<count;i+=2){
		uint64_t prefix=incoming[i+0];
		uint64_t suffix=incoming[i+1];

		Vertex*node=m_subgraph->find(suffix);

		if(node==NULL){ // node already deleted, don't need to delete the edges.
			continue;
		}

		#ifdef ASSERT
		assert(node!=NULL);
		int before=node->getOutgoingEdges(suffix,*m_wordSize).size();
		#endif

		/* the edge might already be deleted if the tip is within another tip. */

		node->deleteIngoingEdge(suffix,prefix,(*m_wordSize));

		#ifdef ASSERT
		int after=node->getOutgoingEdges(suffix,*m_wordSize).size();
		assert(after<=before);
		#endif
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_DELETE_INGOING_EDGE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_OUTGOING_EDGE_REPLY(Message*message){
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_INGOING_EDGE_REPLY(Message*message){
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_OUTGOING_EDGE(Message*message){

	#ifdef ASSERT
	assert(!m_subgraph->frozen());
	#endif

	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	for(int i=0;i<count;i+=2){
		uint64_t prefix=incoming[i+0];
		uint64_t suffix=incoming[i+1];
		Vertex*node=m_subgraph->find(prefix);

		if(node==NULL){ // node already deleted, don't need to delete the edges.
			continue;
		}
	
		#ifdef ASSERT
		assert(node!=NULL);
		int before=node->getOutgoingEdges(suffix,*m_wordSize).size();
		#endif

		/* the edge might already be deleted if the tip is within another tip. */
		node->deleteOutgoingEdge(prefix,suffix,(*m_wordSize));
		
		#ifdef ASSERT
		int after=node->getOutgoingEdges(suffix,*m_wordSize).size();
		assert(after<=before);
		#endif
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_DELETE_OUTGOING_EDGE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX_REPLY(Message*message){
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_BEGIN_REDUCTION_REPLY(Message*aMessage){
	m_verticesExtractor->incrementRanksReadyForReduction();
	if(m_verticesExtractor->readyForReduction()){
		(*m_master_mode)=RAY_MASTER_MODE_START_REDUCTION;
		m_verticesExtractor->resetRanksReadyForReduction();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_RESUME_VERTEX_DISTRIBUTION(Message*message){
	if(parameters->runReducer()){
		m_verticesExtractor->updateThreshold(m_subgraph);
		printf("Rank %i: %lu -> %lu\n",rank,m_lastSize,m_subgraph->size());
	}
	if(!m_verticesExtractor->finished()){
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
	#ifdef ASSERT
	assert(m_subgraph->frozen());
	#endif

	m_lastSize=m_subgraph->size();
	(*m_mode)=RAY_SLAVE_MODE_DELETE_VERTICES;
	m_subgraph->unfreeze();

	#ifdef ASSERT
	assert(!m_subgraph->frozen());
	#endif
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTICES_DONE(Message*message){
	m_verticesExtractor->incrementRanksDoneWithReduction();

	if(m_verticesExtractor->reductionIsDone()){
		printf("\n");
		fflush(stdout);
		for(int i=0;i<size;i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_UPDATE_THRESHOLD,rank);
			m_outbox->push_back(aMessage);
		}
		m_verticesExtractor->resetRanksDoneForReduction();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_UPDATE_THRESHOLD(Message*message){
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY(Message*message){
	m_verticesExtractor->incrementRanksDoneWithReduction();
	if(m_verticesExtractor->reductionIsDone()){
		if(m_verticesExtractor->isDistributionCompleted()){
			(*m_master_mode)=RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS;
		}else{
			(*m_master_mode)=RAY_MASTER_MODE_RESUME_VERTEX_DISTRIBUTION;
			m_verticesExtractor->resetRanksDoneForReduction();
		}
	}
}

void MessageProcessor::call_RAY_MPI_TAG_START_REDUCTION(Message*message){
	m_subgraph->freeze();

	#ifdef ASSERT
	m_verticesExtractor->assertBuffersAreEmpty();
	assert(m_subgraph->frozen());
	#endif
	(*m_mode)=RAY_SLAVE_MODE_REDUCE_MEMORY_CONSUMPTION;
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_BEGIN_REDUCTION(Message*message){
	//cout<<"Source="<<message->getSource()<<" Destination="<<rank<<" RAY_MPI_TAG_ASK_BEGIN_REDUCTION"<<endl;
	m_verticesExtractor->scheduleReduction(m_outbox,rank);
}

void MessageProcessor::call_RAY_MPI_TAG_MUST_RUN_REDUCER(Message*message){
	int rank=message->getSource();
	//cout<<"Source="<<rank<<" Destination="<<rank<<" RAY_MPI_TAG_MUST_RUN_REDUCER"<<endl;
	m_verticesExtractor->addRankForReduction(rank);
	if(m_verticesExtractor->mustRunReducer()){
		(*m_master_mode)=RAY_MASTER_MODE_ASK_BEGIN_REDUCTION;
		m_verticesExtractor->resetRanksForReduction();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_WELCOME(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_INDEX_SEQUENCES;
	m_si->constructor(parameters,m_outboxAllocator,m_inbox,m_outbox);
	void constructor(int rank,int size,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox);
}

void MessageProcessor::call_RAY_MPI_TAG_SEQUENCES_READY(Message*message){
	(*m_sequence_ready_machines)++;
	if(*m_sequence_ready_machines==size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION;
	}
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
			showMemoryUsage(rank);
		}

		Vertex*tmp=m_subgraph->insert(l);
		#ifdef ASSERT
		assert(tmp!=NULL);
		#endif
		if(m_subgraph->inserted()){
			tmp->constructor(); 
		}
		tmp->setCoverage(l,tmp->getCoverage(l)+1);
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
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_VERTICES_DISTRIBUTED(Message*message){
	(*m_numberOfMachinesDoneSendingVertices)++;
	if((*m_numberOfMachinesDoneSendingVertices)==size){
		m_verticesExtractor->setDistributionAsCompleted();
		if(parameters->runReducer()){
			for(int i=0;i<size;i++){
				Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_MUST_RUN_REDUCER_FROM_MASTER,rank);
				m_outbox->push_back(aMessage);
			}
		}else{
			(*m_master_mode)=RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS;
		}
	}
}

void MessageProcessor::call_RAY_MPI_TAG_MUST_RUN_REDUCER_FROM_MASTER(Message*message){
	m_verticesExtractor->trigger();
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_MUST_RUN_REDUCER,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_OUT_EDGES_DATA_REPLY(Message*message){
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_OUT_EDGES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	int length=count;

	for(int i=0;i<(int)length;i+=2){
		uint64_t prefix=incoming[i+0];
		uint64_t suffix=incoming[i+1];

		Vertex*node=m_subgraph->find(prefix);

		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		node->addOutgoingEdge(prefix,suffix,(*m_wordSize));
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_OUT_EDGES_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION(Message*message){
	// wait for everyone
	MPI_Barrier(MPI_COMM_WORLD);
	(*m_mode_send_vertices)=true;
	(*m_mode)=RAY_SLAVE_MODE_EXTRACT_VERTICES;
	m_verticesExtractor->constructor(size,parameters);
	if(parameters->runReducer()){
		m_verticesExtractor->enableReducer();
	}

	(*m_mode_send_vertices_sequence_id)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_EDGES_DISTRIBUTED(Message*message){
	(*m_numberOfMachinesDoneSendingEdges)++;
	if((*m_numberOfMachinesDoneSendingEdges)==size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_INDEXING;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY(Message*message){
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	int length=count;

	for(int i=0;i<(int)length;i+=2){
		uint64_t prefix=incoming[i+0];
		uint64_t suffix=incoming[i+1];
	
		Vertex*node=m_subgraph->find(suffix);

		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		node->addIngoingEdge(suffix,prefix,(*m_wordSize));
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_IN_EDGES_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message){
	#ifdef ASSERT
	m_verticesExtractor->assertBuffersAreEmpty();
	#endif

	if(parameters->runReducer()){
		m_verticesExtractor->updateThreshold(m_subgraph);
		printf("Rank %i: %lu -> %lu\n",rank,m_lastSize,m_subgraph->size());
	}

	// freeze the forest. icy winter ahead.
	m_subgraph->freeze();
	//m_subgraph->show(rank,parameters->getPrefix().c_str());
	int source=message->getSource();

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
	printf("Rank %i has %i vertices (completed)\n",rank,(int)m_subgraph->size());
	fflush(stdout);
	showMemoryUsage(rank);
	fflush(stdout);

	int chunks=m_subgraph->getAllocator()->getNumberOfChunks();
	int chunkSize=m_subgraph->getAllocator()->getChunkSize();
	uint64_t totalBytes=chunks*chunkSize;
	uint64_t kibibytes=totalBytes/1024;
	printf("Rank %i: memory usage for vertices topology is %lu KiB\n",rank,kibibytes);
	fflush(stdout);

	(*m_mode_send_coverage_iterator)=0;
	(*m_mode_sendDistribution)=true;
	(*m_mode)=RAY_SLAVE_MODE_SEND_DISTRIBUTION;
}

void MessageProcessor::call_RAY_MPI_TAG_COVERAGE_DATA_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_COVERAGE_DATA(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();

	for(int i=0;i<count;i+=2){
		int coverage=incoming[i];
		uint64_t count=incoming[i+1];
		(*m_coverageDistribution)[coverage]+=count;
	}
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_COVERAGE_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_COVERAGE_END(Message*message){
	//cout<<__func__<<" "<<message->getSource()<<endl;
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
	parameters->setSeedCoverage(*m_seedCoverage);
	parameters->setMinimumCoverage(*m_minimumCoverage);
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
	GridTableIterator seedingIterator;
	//MyForestIterator seedingIterator;
	seedingIterator.constructor(m_subgraph,*m_wordSize);
	while(seedingIterator.hasNext()){
		size++;
		Vertex*node=seedingIterator.next();
		//SplayNode<uint64_t,Vertex>*node=seedingIterator.next();
		edgesDistribution[node->getIngoingEdges(node->m_lowerKey,(*m_wordSize)).size()][node->getOutgoingEdges(node->m_lowerKey,(*m_wordSize)).size()]++;
		//(m_seedingData->m_SEEDING_nodes).push_back(node->getKey());
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

	int chunks=m_subgraph->getSecondAllocator()->getNumberOfChunks();
	int chunkSize=m_subgraph->getSecondAllocator()->getChunkSize();
	uint64_t totalBytes=chunks*chunkSize;
	uint64_t kibibytes=totalBytes/1024;
	printf("Rank %i: memory usage for vertices data is %lu KiB\n",rank,kibibytes);
	fflush(stdout);

	chunks=m_si->getAllocator()->getNumberOfChunks();
	chunkSize=m_si->getAllocator()->getChunkSize();
	totalBytes=chunks*chunkSize;
	kibibytes=totalBytes/1024;
	printf("Rank %i: memory usage for read markers is %lu KiB\n",rank,kibibytes);
	fflush(stdout);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*1*sizeof(uint64_t));
	for(int i=0;i<count;i++){
		Vertex*node=m_subgraph->find(incoming[i+0]);
		#ifdef ASSERT
		if(node==NULL){
			cout<<"Rank="<<rank<<" "<<__func__<<" "<<idToWord(incoming[i+0],(*m_wordSize))<<" does not exist"<<endl;
		}
		assert(node!=NULL);
		#endif
		uint64_t coverage=node->getCoverage(incoming[i]);
		message2[i+0]=coverage;
	}
	Message aMessage(message2,count*1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedVertexCoverage)=incoming[0];
	(m_seedingData->m_SEEDING_vertexCoverageReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*5*sizeof(uint64_t));
	for(int i=0;i<count;i++){
		vector<uint64_t> outgoingEdges=m_subgraph->find(incoming[i+0])->getOutgoingEdges(incoming[i+0],*m_wordSize);
		message2[5*i+0]=outgoingEdges.size();
		for(int j=0;j<(int)outgoingEdges.size();j++){
			message2[5*i+1+j]=outgoingEdges[j];
		}
	}
	Message aMessage(message2,count*5,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*10*sizeof(uint64_t));

	for(int j=0;j<count;j++){
		uint64_t vertex=incoming[j];
		Vertex*node=m_subgraph->find(vertex);

		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		vector<uint64_t> outgoingEdges=node->getOutgoingEdges(vertex,*m_wordSize);
		vector<uint64_t> ingoingEdges=node->getIngoingEdges(vertex,*m_wordSize);
		int k=j*10;
		
		message2[k+0]=outgoingEdges.size();
		for(int i=0;i<(int)outgoingEdges.size();i++){
			message2[k+1+i]=outgoingEdges[i];
		}
		message2[k+5]=ingoingEdges.size();
		for(int i=0;i<(int)ingoingEdges.size();i++){
			message2[k+5+1+i]=ingoingEdges[i];
		}
	}
	Message aMessage(message2,count*10,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;

	int numberOfOutgoingEdges=incoming[0];
	(m_seedingData->m_SEEDING_receivedOutgoingEdges).clear();

	for(int i=0;i<numberOfOutgoingEdges;i++){
		(m_seedingData->m_SEEDING_receivedOutgoingEdges).push_back(incoming[1+i]);
	}

	int numberOfIngoingEdges=incoming[5];
	m_seedingData->m_SEEDING_receivedIngoingEdges.clear();

	for(int i=0;i<numberOfIngoingEdges;i++){
		uint64_t nextVertex=incoming[5+1+i];
		m_seedingData->m_SEEDING_receivedIngoingEdges.push_back(nextVertex);
	}

	(m_seedingData->m_SEEDING_edgesReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedOutgoingEdges).clear();
	for(int i=0;i<(int)incoming[0];i++){
		uint64_t nextVertex=incoming[1+i];
		(m_seedingData->m_SEEDING_receivedOutgoingEdges).push_back(nextVertex);
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
	#ifdef COUNT_MESSAGES
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	for(int i=0;i<count;i++){
		m_messagesHandler->addCount(message->getSource(),incoming[i]);
	}
	#endif

	bool isFinished=true;
	#ifdef COUNT_MESSAGES
	isFinished=m_messagesHandler->isFinished(message->getSource());
	#endif
	if(message->getSource()!=MASTER_RANK && isFinished){
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_RECEIVED_MESSAGES_REPLY,rank);
		m_outbox->push_back(aMessage);
	}
	bool finished=true;
	#ifdef COUNT_MESSAGES
	finished=m_messagesHandler->isFinished()
	#endif
	if(finished){
		(*m_alive)=false;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON(Message*message){
	#ifdef COUNT_MESSAGES
	// send stats to master
	int i=0;
	while(i<size){
		uint64_t*data=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		int j=0;
		int maxToProcess=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);
		while(i+j<size &&j<maxToProcess){
			#ifdef COUNT_MESSAGES
			data[j]=m_messagesHandler->getReceivedMessages()[i+j];
			#endif
			j++;
		}
		Message aMessage(data,j,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_RECEIVED_MESSAGES,rank);
		m_outbox->push_back(aMessage);
		i+=maxToProcess;
	}
	#else
	*m_alive=false;
	#endif
}

void MessageProcessor::call_RAY_MPI_TAG_I_GO_NOW(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS(Message*message){
	int source=message->getSource();
	uint64_t*dummy=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(dummy,5,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,rank);
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
	int count=message->getCount();
	#ifdef ASSERT
	if(count*5*sizeof(uint64_t)>MAXIMUM_MESSAGE_SIZE_IN_BYTES){
		cout<<"Count="<<count<<endl;
	}
	assert(count*5*sizeof(uint64_t)<=MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*5*sizeof(uint64_t));
	for(int i=0;i<count;i++){
		Vertex*node=m_subgraph->find(incoming[0+i]);
		#ifdef ASSERT
		if(node==NULL){
			cout<<"Rank="<<rank<<" "<<idToWord(incoming[i+0],*m_wordSize)<<" does not exist."<<endl;
		}
		assert(node!=NULL);
		#endif 
		vector<uint64_t> ingoingEdges=node->getIngoingEdges(incoming[i+0],*m_wordSize);
		message2[i*5+0]=ingoingEdges.size();
		for(int j=0;j<(int)ingoingEdges.size();j++){
			message2[i*5+1+j]=ingoingEdges[j];
		}
	}
	Message aMessage(message2,count*5,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedIngoingEdges).clear();
	for(int i=0;i<(int)incoming[0];i++){
		(m_seedingData->m_SEEDING_receivedIngoingEdges).push_back(incoming[1+i]);
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
	m_verticesExtractor->showBuffers();

}

void MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	#ifdef ASSERT
	Vertex*node=m_subgraph->find(incoming[0]);
	assert(node!=NULL);
	#endif
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
	message2[0]=m_subgraph->isAssembled(incoming[0]);

	Message aMessage(message2,1,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;

	(m_ed->m_EXTENSION_VertexAssembled_received)=true;
	(m_ed->m_EXTENSION_vertexIsAssembledResult)=(bool)incoming[0];
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
	for(int i=0;i<count;i+=5){
		m_count++;
		uint64_t vertex=incoming[i+0];
/*
		int coverage=m_subgraph->find(vertex)->getCoverage(vertex);
		if(coverage==1){
			continue;
		}
*/
		uint64_t complement=complementVertex_normal(vertex,*m_wordSize);
		bool lower=vertex<complement;
		int rank=incoming[i+1];
		int sequenceIdOnDestination=(int)incoming[i+2];
		int positionOnStrand=incoming[i+3];
		char strand=(char)incoming[i+4];
		//cout<<__func__<<" Vertex="<<idToWord(vertex,*m_wordSize)<<" ReadRank="<<rank<<" ReadIndexOnRank="<<sequenceIdOnDestination<<" ReadStrandOnVertex="<<strand<<" StrandPositionInRead="<<positionOnStrand<<endl;
		Vertex*node=m_subgraph->find(vertex);
		if(node==NULL){
			continue;
		}
		ReadAnnotation*e=(ReadAnnotation*)m_si->getAllocator()->allocate(sizeof(ReadAnnotation));
		#ifdef ASSERT
		assert(e!=NULL);
		#endif
		e->constructor(rank,sequenceIdOnDestination,positionOnStrand,strand,lower);

		m_subgraph->addRead(vertex,e);
	}
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(message2,count,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY(Message*message){
	m_si->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READS(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int source=message->getSource();
	int count=message->getCount();

	// keep 3 for the sentinels or the pointer. + 1 for the vertex
	int maxToProcess=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)-4-1;
	maxToProcess=maxToProcess-maxToProcess%4;
	
	#ifdef ASSERT
	assert(maxToProcess%4==0);
	#endif

	ReadAnnotation*e=NULL;

	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int j=0;

	uint64_t vertex=0;
	// start from the beginning
	if(count==1){
		//cout<<__func__<<" from key "<<incoming[0]<<endl;
		cout.flush();
		vertex=incoming[0];

		#ifdef ASSERT
		Vertex*node=m_subgraph->find(vertex);
		assert(node!=NULL);
		#endif

		e=m_subgraph->getReads(vertex);
		/*
		int n=0;
		while(e!=NULL){
			n++;
			e=e->getNext();
		}
		cout<<"Reads: "<<n<<endl;
		*/
		e=m_subgraph->getReads(vertex);

		#ifdef ASSERT
		assert(maxToProcess%4==0);
		#endif

		// send a maximum of maxToProcess individually

		// pad the message with a sentinel value  sentinel/0/sentinel
		message2[j++]=m_sentinelValue;
		message2[j++]=0;
		message2[j++]=m_sentinelValue;
		message2[j++]=m_sentinelValue;
		
		#ifdef ASSERT
		assert(j==4);
		#endif

	// use the pointer provided, count is 2, but only the first element is good.
	}else{
		//cout<<__func__<<" from pointer "<<incoming[0]<<endl; 
		cout.flush();
		e=(ReadAnnotation*)incoming[0];
		vertex=incoming[1];

		#ifdef ASSERT
		assert(e!=NULL);
		assert(j==0);
		#endif
	}

	uint64_t complement=complementVertex_normal(vertex,*m_wordSize);
	bool lower=vertex<complement;

	while(e!=NULL&&j!=maxToProcess){
		#ifdef ASSERT
		assert(e!=NULL);
		assert(e->getRank()>=0);
		if(e->getRank()>=size){
			cout<<"Error : rank="<<e->getRank()<<" Strand="<<e->getStrand()<<endl;
		}
		assert(e->getRank()<size);
		assert(j<=(maxToProcess-4));
		assert(e->getStrand()=='F'||e->getStrand()=='R');
		assert((uint64_t)e->getReadIndex()!=m_sentinelValue);
		#endif

		if(e->isLower()==lower){
			//cout<<"appending"<<endl;
			message2[j++]=e->getRank();
			message2[j++]=e->getReadIndex();
			message2[j++]=e->getPositionOnStrand();
			message2[j++]=e->getStrand();
		}
		//cout<<"next"<<endl;
		e=e->getNext();

		#ifdef ASSERT
		assert(j%4==0);
		assert(j<=maxToProcess+4);
		#endif
	}
	if(e==NULL){
		// end is sentinel/sentinel/sentinel
		message2[j++]=m_sentinelValue;
		message2[j++]=m_sentinelValue;
		message2[j++]=m_sentinelValue;
		message2[j++]=m_sentinelValue;
		//cout<<"DONE "<<endl;
		cout.flush();
		#ifdef ASSERT
		assert(j%4==0);
		#endif
		message2[j++]=vertex;
	}else{
		// pad with the pointer
		message2[j++]=(uint64_t)e;
		//cout<<"SET PTR="<<(uint64_t)e<<endl;
		cout.flush();
		#ifdef ASSERT
		assert(j%4==1);
		#endif
		message2[j++]=vertex;
	}

	Message aMessage(message2,j,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_READS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READS_REPLY(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	int count3=count-(count%4);
	uint64_t vertex=incoming[count-1];
	uint64_t complement=complementVertex_normal(vertex,*m_wordSize);
	bool lower=vertex<complement;

	#ifdef ASSERT
	assert(count3%4==0);
	assert(incoming!=NULL);
	#endif

	for(int i=0;i<count3;i+=4){
		// beginning of transmission, s,0,s
		if(incoming[i+0]==m_sentinelValue 
		&& incoming[i+1]==0
		&& incoming[i+2]==m_sentinelValue){
			#ifdef ASSERT
			assert(m_ed->m_EXTENSION_reads_received==false);
			#endif

			m_ed->m_EXTENSION_receivedReads.clear();
		// end of transmission, s,s,s
		}else if(incoming[i+0]==m_sentinelValue 
		&& incoming[i+1]==m_sentinelValue
		&& incoming[i+2]==m_sentinelValue){
			(m_ed->m_EXTENSION_reads_received)=true;
			//cout<<m_ed->m_EXTENSION_receivedReads.size()<<" Reads."<<endl;
		}else{
			#ifdef ASSERT
			assert(m_ed->m_EXTENSION_reads_received==false);
			assert(incoming[i+0]!=m_sentinelValue);
			assert(incoming[i+1]!=m_sentinelValue);
			assert(incoming[i+2]!=m_sentinelValue);
			#endif

			int theRank=incoming[i];
			int index=incoming[i+1];
			int strandPosition=incoming[i+2];
			char strand=incoming[i+3];

			#ifdef ASSERT
			assert(theRank>=0);
			assert(theRank<size);
			assert(strand=='F'||strand=='R');
			#endif

			ReadAnnotation e;
			e.constructor(theRank,index,strandPosition,strand,lower);
			m_ed->m_EXTENSION_receivedReads.push_back(e);
		}
	}
	if(!m_ed->m_EXTENSION_reads_received){
		// ask for more, give the pointer to the correct location.
		// pointer is 64 bits, assuming 64-bit architecture
		uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
		void*ptr=(void*)incoming[count-2];
		#ifdef ASSERT
		assert(ptr!=NULL);
		#endif

		message2[0]=(uint64_t)ptr;
		message2[1]=vertex;
		Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_REQUEST_READS,rank);
		m_outbox->push_back(aMessage);
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
		uint64_t vertex=incoming[i+0];
		uint64_t rc=complementVertex_normal(vertex,*m_wordSize);
		bool lower=vertex<rc;
		#ifdef ASSERT
		Vertex*node=m_subgraph->find(vertex);
		assert(node!=NULL);
		#endif
		uint64_t wave=incoming[i+1];
		int progression=incoming[i+2];
		Direction*e=(Direction*)m_directionsAllocator->allocate(sizeof(Direction));
		e->constructor(wave,progression,lower);

		m_subgraph->addDirection(incoming[i],e);
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
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	Vertex*node=m_subgraph->find(incoming[0]);

	#ifdef ASSERT
	if(node==NULL){
		cout<<"Source="<<message->getSource()<<" Destination="<<rank<<" "<<idToWord(incoming[0],*m_wordSize)<<" does not exist, aborting"<<endl;
		cout.flush();
	}
	assert(node!=NULL);
	#endif

	vector<Direction> paths=m_subgraph->getDirections(incoming[0]);
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message2[0]=paths.size();
	message2[1]=node->getCoverage(incoming[0]);
	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FUSION_paths_received=true;
	m_fusionData->m_FUSION_receivedPaths.clear();
	m_fusionData->m_FUSION_numberOfPaths=incoming[0];
	m_seedingData->m_SEEDING_receivedVertexCoverage=incoming[1];
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	uint64_t id=incoming[0];
	int length=0;
	if(m_fusionData->m_FUSION_identifier_map.count(id)>0){
		length=(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size();
	}

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

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	uint64_t vertex=incoming[0];
	int firstPathId=incoming[1];
	vector<Direction> paths=m_subgraph->getDirections(vertex);

	int availableElements=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int j=2;
	message2[0]=vertex;
	while(firstPathId<(int)paths.size() && j<availableElements){
		#ifdef ASSERT
		assert(firstPathId<(int)paths.size());
		#endif
		uint64_t pathId=paths[firstPathId].getWave();
		#ifdef ASSERT
		assert(getRankFromPathUniqueId(pathId)<size);
		#endif
		message2[j++]=pathId;
		message2[j++]=paths[firstPathId].getProgression();
		firstPathId++;
	}
	message2[1]=firstPathId;

	#ifdef ASSERT
	assert(j/2-1<=(int)paths.size());
	assert(j<=availableElements);
	assert(source<size);
	#endif

	if(firstPathId==(int)paths.size()){
		Message aMessage(message2,j,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END,rank);
		m_outbox->push_back(aMessage);
	}else{
		Message aMessage(message2,j,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY,rank);
		m_outbox->push_back(aMessage);
	}
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	uint64_t vertex=incoming[0];
	int count=message->getCount();
	uint64_t complement=complementVertex_normal(vertex,*m_wordSize);
	bool lower=vertex<complement;
	for(int i=2;i<count;i+=2){
		uint64_t pathId=incoming[i];
		#ifdef ASSERT
		assert(getRankFromPathUniqueId(pathId)<size);
		#endif
		int position=incoming[i+1];
		m_fusionData->m_FUSION_receivedPath.constructor(pathId,position,lower);
		m_fusionData->m_Machine_getPaths_result.push_back(m_fusionData->m_FUSION_receivedPath);
	}

	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message2[0]=vertex;
	message2[1]=incoming[1];
	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_ASK_VERTEX_PATHS,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	uint64_t vertex=incoming[0];
	int count=message->getCount();
	uint64_t complement=complementVertex_normal(vertex,*m_wordSize);
	bool lower=vertex<complement;
	for(int i=2;i<count;i+=2){
		uint64_t pathId=incoming[i];
		#ifdef ASSERT
		assert(getRankFromPathUniqueId(pathId)<size);
		#endif
		int position=incoming[i+1];
		m_fusionData->m_FUSION_receivedPath.constructor(pathId,position,lower);
		m_fusionData->m_Machine_getPaths_result.push_back(m_fusionData->m_FUSION_receivedPath);
	}

	m_fusionData->m_FUSION_paths_received=true;
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	#ifdef ASSERT
	Vertex*node=m_subgraph->find(incoming[0]);
	assert(node!=NULL);
	#endif
	vector<Direction> paths=m_subgraph->getDirections(incoming[0]);
	int i=incoming[1];
	Direction d=paths[i];
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(3*sizeof(uint64_t));
	message2[0]=incoming[0];
	message2[1]=d.getWave();
	#ifdef ASSERT
	assert(getRankFromPathUniqueId(message2[1])<size);
	#endif
	message2[2]=d.getProgression();
	Message aMessage(message2,3,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FUSION_path_received=true;
	uint64_t vertex=incoming[0];
	uint64_t complement=complementVertex_normal(vertex,*m_wordSize);
	bool lower=vertex<complement;
	uint64_t pathId=incoming[1];
	#ifdef ASSERT
	assert(getRankFromPathUniqueId(pathId)<size);
	#endif
	int position=incoming[2];
	m_fusionData->m_FUSION_receivedPath.constructor(pathId,position,lower);
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
	GridTableIterator iterator;
	//MyForestIterator iterator;
	iterator.constructor(m_subgraph,*m_wordSize);
	while(iterator.hasNext()){
		iterator.next();
		uint64_t key=iterator.getKey();
		m_subgraph->clearDirections(key);
	}

	m_directionsAllocator->reset();

	// add the FINISHING bits
	for(int i=0;i<(int)m_fusionData->m_FINISH_newFusions.size();i++){
		#ifdef SHOW_PROGRESS
		#endif
		(m_ed->m_EXTENSION_contigs).push_back((m_fusionData->m_FINISH_newFusions)[i]);
	}

	m_fusionData->m_FINISH_newFusions.clear();


	vector<vector<uint64_t> > fusions;
	for(int i=0;i<(int)(m_ed->m_EXTENSION_contigs).size();i++){
		uint64_t id=(m_ed->m_EXTENSION_identifiers)[i];
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
		uint64_t id=getPathUniqueId(rank,i);
		#ifdef ASSERT
		assert(rank<size);
		assert(rank>=0);
		assert(size>=1);
		assert((int)getRankFromPathUniqueId(id)<size);
		#endif
		(m_ed->m_EXTENSION_identifiers).push_back(id);
	}

	for(int i=0;i<(int)(m_ed->m_EXTENSION_identifiers).size();i++){
		uint64_t id=(m_ed->m_EXTENSION_identifiers)[i];
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
	m_fusionData->readyBuffers();
	(*m_mode)=RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS;
	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY(Message*message){
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED(Message*message){
	(*m_DISTRIBUTE_n)++;
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_START(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	vector<uint64_t> a;
	(*m_allPaths).push_back(a);
	uint64_t id=incoming[0];
	#ifdef ASSERT
	int rank=getRankFromPathUniqueId(id);
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
	uint64_t id=incoming[0];
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

void MessageProcessor::call_RAY_MPI_TAG_WRITE_AMOS(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_AMOS;
	m_ed->m_EXTENSION_initiated=false;
	m_ed->m_EXTENSION_currentPosition=((uint64_t*)message->getBuffer())[0];
}

void MessageProcessor::call_RAY_MPI_TAG_WRITE_AMOS_REPLY(Message*message){
	m_ed->m_EXTENSION_currentRankIsDone=true;
	m_ed->m_EXTENSION_currentPosition=((uint64_t*)message->getBuffer())[0];
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
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_INDEXING;
	}
}

void MessageProcessor::setReducer(MemoryConsumptionReducer*a){
	m_reducer=a;
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

	int beforeRounding=5*sizeof(uint64_t)+m_myReads->at(index)->getRequiredBytes();
	int toAllocate=roundNumber(beforeRounding,sizeof(uint64_t));
	//cout<<" seq is "<<strlen(seq)<<" +1 +4*8="<<beforeRounding<<", rounded: "<<toAllocate<<endl;

	uint64_t*messageBytes=(uint64_t*)m_outboxAllocator->allocate(toAllocate);
	messageBytes[0]=t->getRank();
	messageBytes[1]=t->getId();
	messageBytes[2]=t->getLibrary();
	messageBytes[3]=(*m_myReads)[index]->getType();
	messageBytes[4]=m_myReads->at(index)->length();
	char*dest=(char*)(messageBytes+5);
	memcpy(dest,m_myReads->at(index)->getRawSequence(),m_myReads->at(index)->getRequiredBytes());
	//cout<<"dest="<<dest<<endl;
	Message aMessage(messageBytes,toAllocate/sizeof(uint64_t),MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_ed->m_EXTENSION_pairedRead).constructor(incoming[0],incoming[1],incoming[2]);
	(m_ed->m_EXTENSION_pairedSequenceReceived)=true;
	m_ed->m_readType=incoming[3];
	int length=incoming[4];
	uint8_t*sequence=(uint8_t*)(incoming+5);
	Read tmp;
	tmp.setRawSequence(sequence,length);
	char buffer2[4000];
	tmp.getSeq(buffer2);
	seedExtender->m_receivedString=buffer2;
	seedExtender->m_sequenceReceived=true;
}

void MessageProcessor::constructor(
MessagesHandler*m_messagesHandler,
SeedingData*seedingData,
Library*m_library,
bool*m_ready,
VerticesExtractor*m_verticesExtractor,
SequencesLoader*sequencesLoader,ExtensionData*ed,
			int*m_numberOfRanksDoneDetectingDistances,
			int*m_numberOfRanksDoneSendingDistances,
			Parameters*parameters,
			GridTable*m_subgraph,
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
	vector<uint64_t>*m_identifiers,
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
				StaticVector*m_inbox,
		map<int,int>*m_allIdentifiers,OpenAssemblerChooser*m_oa,
int*m_numberOfRanksWithCoverageData,
SeedExtender*seedExtender,int*m_master_mode,
bool*m_isFinalFusion,
SequencesIndexer*m_si){
	m_count=0;

	this->m_sequencesLoader=sequencesLoader;
	this->m_verticesExtractor=m_verticesExtractor;
	this->m_ed=ed;
	this->m_numberOfRanksDoneDetectingDistances=m_numberOfRanksDoneDetectingDistances;
	this->m_numberOfRanksDoneSendingDistances=m_numberOfRanksDoneSendingDistances;
	this->parameters=parameters;
	this->m_library=m_library;
	this->m_subgraph=m_subgraph;
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
	this->m_inbox=m_inbox;
	this->m_allIdentifiers=m_allIdentifiers,
	this->m_oa=m_oa;
	this->m_numberOfRanksWithCoverageData=m_numberOfRanksWithCoverageData;
	this->seedExtender=seedExtender;
	this->m_master_mode=m_master_mode;
	this->m_isFinalFusion=m_isFinalFusion;
	this->m_ready=m_ready;
	m_seedingData=seedingData;

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
	m_methods[RAY_MPI_TAG_SEQUENCES_READY]=&MessageProcessor::call_RAY_MPI_TAG_SEQUENCES_READY;
	m_methods[RAY_MPI_TAG_VERTICES_DATA]=&MessageProcessor::call_RAY_MPI_TAG_VERTICES_DATA;
	m_methods[RAY_MPI_TAG_VERTICES_DISTRIBUTED]=&MessageProcessor::call_RAY_MPI_TAG_VERTICES_DISTRIBUTED;
	m_methods[RAY_MPI_TAG_OUT_EDGES_DATA]=&MessageProcessor::call_RAY_MPI_TAG_OUT_EDGES_DATA;
	m_methods[RAY_MPI_TAG_START_VERTICES_DISTRIBUTION]=&MessageProcessor::call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION;
	m_methods[RAY_MPI_TAG_IN_EDGES_DATA]=&MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA;
	m_methods[RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION]=&MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION;
	m_methods[RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER]=&MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER;
	m_methods[RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION]=&MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION;
	m_methods[RAY_MPI_TAG_COVERAGE_DATA]=&MessageProcessor::call_RAY_MPI_TAG_COVERAGE_DATA;
	m_methods[RAY_MPI_TAG_COVERAGE_DATA_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_COVERAGE_DATA_REPLY;
	m_methods[RAY_MPI_TAG_COVERAGE_END]=&MessageProcessor::call_RAY_MPI_TAG_COVERAGE_END;
	m_methods[RAY_MPI_TAG_SEND_COVERAGE_VALUES]=&MessageProcessor::call_RAY_MPI_TAG_SEND_COVERAGE_VALUES;
	m_methods[RAY_MPI_TAG_READY_TO_SEED]=&MessageProcessor::call_RAY_MPI_TAG_READY_TO_SEED;
	m_methods[RAY_MPI_TAG_START_SEEDING]=&MessageProcessor::call_RAY_MPI_TAG_START_SEEDING;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY;
	m_methods[RAY_MPI_TAG_SEEDING_IS_OVER]=&MessageProcessor::call_RAY_MPI_TAG_SEEDING_IS_OVER;
	m_methods[RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON]=&MessageProcessor::call_RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON;
	m_methods[RAY_MPI_TAG_I_GO_NOW]=&MessageProcessor::call_RAY_MPI_TAG_I_GO_NOW;
	m_methods[RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS]=&MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS;
	m_methods[RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY;
	m_methods[RAY_MPI_TAG_EXTENSION_IS_DONE]=&MessageProcessor::call_RAY_MPI_TAG_EXTENSION_IS_DONE;
	m_methods[RAY_MPI_TAG_ASK_EXTENSION]=&MessageProcessor::call_RAY_MPI_TAG_ASK_EXTENSION;
	m_methods[RAY_MPI_TAG_ASK_IS_ASSEMBLED]=&MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED;
	m_methods[RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY;
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
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY;
	m_methods[RAY_MPI_TAG_GET_PATH_LENGTH]=&MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH;
	m_methods[RAY_MPI_TAG_VERTICES_DATA_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_VERTICES_DATA_REPLY;
	m_methods[RAY_MPI_TAG_GET_PATH_LENGTH_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY;
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATH]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH;
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY;
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
	m_methods[RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY;
	m_methods[RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY;
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
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_EDGES]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY;
	m_methods[RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT]=&MessageProcessor::call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	m_methods[RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY;
	m_methods[RAY_MPI_TAG_DELETE_VERTICES]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTICES;
	m_methods[RAY_MPI_TAG_DELETE_VERTICES_DONE]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTICES_DONE;
	m_methods[RAY_MPI_TAG_UPDATE_THRESHOLD]=&MessageProcessor::call_RAY_MPI_TAG_UPDATE_THRESHOLD;
	m_methods[RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_UPDATE_THRESHOLD_REPLY;
	m_methods[RAY_MPI_TAG_DELETE_VERTEX]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX;
	m_methods[RAY_MPI_TAG_DELETE_INGOING_EDGE]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_INGOING_EDGE;
	m_methods[RAY_MPI_TAG_DELETE_OUTGOING_EDGE]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_OUTGOING_EDGE;
	m_methods[RAY_MPI_TAG_DELETE_VERTEX_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX_REPLY;
	m_methods[RAY_MPI_TAG_DELETE_OUTGOING_EDGE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_OUTGOING_EDGE_REPLY;
	m_methods[RAY_MPI_TAG_DELETE_INGOING_EDGE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_DELETE_INGOING_EDGE_REPLY;
	m_methods[RAY_MPI_TAG_CHECK_VERTEX]=&MessageProcessor::call_RAY_MPI_TAG_CHECK_VERTEX;
	m_methods[RAY_MPI_TAG_CHECK_VERTEX_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_CHECK_VERTEX_REPLY;
	m_methods[RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY;
	m_methods[RAY_MPI_TAG_SET_WORD_SIZE]=&MessageProcessor::call_RAY_MPI_TAG_SET_WORD_SIZE;
	m_methods[RAY_MPI_TAG_MUST_RUN_REDUCER_FROM_MASTER]=&MessageProcessor::call_RAY_MPI_TAG_MUST_RUN_REDUCER_FROM_MASTER;
	m_methods[RAY_MPI_TAG_LOAD_SEQUENCES]=&MessageProcessor::call_RAY_MPI_TAG_LOAD_SEQUENCES;
	m_methods[RAY_MPI_TAG_WRITE_AMOS]=&MessageProcessor::call_RAY_MPI_TAG_WRITE_AMOS;
	m_methods[RAY_MPI_TAG_WRITE_AMOS_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_WRITE_AMOS_REPLY;
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END;
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY;
	m_methods[RAY_MPI_TAG_ASK_VERTEX_PATHS]=&MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_READS]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_READS;
	m_methods[RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY;
	m_methods[RAY_MPI_TAG_GET_READ_MATE]=&MessageProcessor::call_RAY_MPI_TAG_GET_READ_MATE;
	m_methods[RAY_MPI_TAG_GET_READ_MATE_REPLY]=&MessageProcessor::call_RAY_MPI_TAG_GET_READ_MATE_REPLY;
}


