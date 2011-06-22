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

#include <core/constants.h>
#include <string.h>
#include <assert.h>
#include <structures/Read.h>
#include <communication/MessageProcessor.h>
#include <structures/StaticVector.h>
#include <core/common_functions.h>
#include <structures/ReadAnnotation.h>
#include <structures/SplayTree.h>
#include <structures/Direction.h>
#include <structures/SplayNode.h>
#include <structures/MyForest.h>
#include <structures/SplayTreeIterator.h>
#include <assembler/FusionData.h>
#include <core/Parameters.h>

void MessageProcessor::call_RAY_MPI_TAG_LOAD_SEQUENCES(Message*message){
	uint32_t*incoming=(uint32_t*)message->getBuffer();
	for(int i=0;i<(int)incoming[0];i++){
		m_parameters->setNumberOfSequences(incoming[1+i]);
	}
	(*m_mode)=RAY_SLAVE_MODE_LOAD_SEQUENCES;
}

void MessageProcessor::call_RAY_MPI_TAG_CONTIG_INFO(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	m_scaffolder->addMasterContig(incoming[0],incoming[1]);
	
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(outgoingMessage,message->getCount(),
		MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_CONTIG_INFO_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_CONTIG_INFO_REPLY(Message*message){
}

void MessageProcessor::processMessage(Message*message){
	int tag=message->getTag();
	FNMETHOD f=m_methods[tag];
	(this->*f)(message);
}

void MessageProcessor::call_RAY_MPI_TAG_SCAFFOLDING_LINKS(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	uint64_t leftContig=incoming[0];
	char leftStrand=incoming[1];
	uint64_t rightContig=incoming[2];
	char rightStrand=incoming[3];
	int average=incoming[4];
	int number=incoming[5];
	if(rightContig<leftContig){
		uint64_t t=leftContig;
		leftContig=rightContig;
		rightContig=t;
		char t2=leftStrand;
		leftStrand=rightStrand;
		rightStrand=t2;
		if(leftStrand=='F')
			leftStrand='R';
		else
			leftStrand='F';
		if(rightStrand=='F')
			rightStrand='R';
		else
			rightStrand='F';
	}

	//cout<<__func__<<" "<<leftContig<<" "<<leftStrand<<" "<<rightContig<<" "<<rightStrand<<" "<<average<<" "<<number<<endl;
	vector<uint64_t> link;
	link.push_back(leftContig);
	link.push_back(leftStrand);
	link.push_back(rightContig);
	link.push_back(rightStrand);
	link.push_back(average);
	link.push_back(number);
	m_scaffolder->addMasterLink(&link);

	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(outgoingMessage,message->getCount(),
		MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_GET_READ_MATE_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_GET_READ_MARKERS_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_GET_READ_MARKERS(Message*message){
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int outputPosition=0;
	for(int i=0;i<count;i++){
		int readId=incoming[i];
		Read*read=m_myReads->at(readId);
		int readLength=read->length();
		outgoingMessage[outputPosition++]=readLength;
		Kmer forwardMarker;
		if(read->getForwardOffset()<=readLength-m_parameters->getWordSize()){
			forwardMarker=read->getVertex(read->getForwardOffset(),m_parameters->getWordSize(),'F',m_parameters->getColorSpaceMode());
		}
		Kmer reverseMarker;
		if(read->getReverseOffset()<=readLength-m_parameters->getWordSize()){
			reverseMarker=read->getVertex(read->getReverseOffset(),m_parameters->getWordSize(),'R',m_parameters->getColorSpaceMode());
		}
		forwardMarker.pack(outgoingMessage,&outputPosition);
		reverseMarker.pack(outgoingMessage,&outputPosition);
		outgoingMessage[outputPosition++]=read->getForwardOffset();
		outgoingMessage[outputPosition++]=read->getReverseOffset();
	}

	Message aMessage(outgoingMessage,outputPosition,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_GET_READ_MARKERS_REPLY,rank);
	m_outbox->push_back(aMessage);
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

/*
 * input: list of Kmer+ReadAnnotation*
 * output:
 *        list of ReadAnnotation
 */
void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_READS(Message*message){
	uint64_t*buffer=(uint64_t*)message->getBuffer();
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

	int j=0;

	for(int i=0;i<message->getCount();i+=KMER_U64_ARRAY_SIZE+1){
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(buffer,&bufferPosition);
		ReadAnnotation*ptr=(ReadAnnotation*)buffer[bufferPosition++];
		Kmer complement=m_parameters->_complementVertex(&vertex);
		bool isLower=vertex<complement;
		if(ptr==NULL){
			ptr=m_subgraph->getReads(&vertex);
		}
	
		bool gotOne=false;
		while(ptr!=NULL&&!gotOne){
			int rank=ptr->getRank();
			#ifdef ASSERT
			assert(rank>=0&&rank<m_parameters->getSize());
			#endif
			if(ptr->isLower()==isLower){
				outgoingMessage[j+1]=rank;
				outgoingMessage[j+2]=ptr->getReadIndex();
				outgoingMessage[j+3]=ptr->getPositionOnStrand();
				outgoingMessage[j+4]=ptr->getStrand();
				gotOne=true;
			}
			ptr=ptr->getNext();
		}
		if(!gotOne){
			outgoingMessage[j+1]=INVALID_RANK;
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
	m_subgraph->setWordSize(*m_wordSize);
}

void MessageProcessor::call_RAY_MPI_TAG_VERTEX_INFO_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_VERTEX_READS_REPLY(Message*message){
}

/*
 * <- k-mer -><- pointer ->
 */
void MessageProcessor::call_RAY_MPI_TAG_VERTEX_READS(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	Kmer vertex;
	int pos=0;
	vertex.unpack(incoming,&pos);
	Kmer complement=m_parameters->_complementVertex(&vertex);
	bool lower=vertex<complement;
	ReadAnnotation*e=(ReadAnnotation*)incoming[pos++];
	ReadAnnotation*origin=e;
	#ifdef ASSERT
	assert(e!=NULL);
	#endif
	int maximumToReturn=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)/4-1;
	int processed=0;
	while(e!=NULL&&processed<maximumToReturn){
		if(e->isLower()==lower){
			processed++;
		}
		e=e->getNext();
	}
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate((processed+1)*4*sizeof(uint64_t));
	int outputPosition=0;
	outgoingMessage[outputPosition++]=processed;
	processed=0;
	e=origin;
	while(e!=NULL&&processed<maximumToReturn){
		if(e->isLower()==lower){
			outgoingMessage[outputPosition++]=e->getRank();
			outgoingMessage[outputPosition++]=e->getReadIndex();
			outgoingMessage[outputPosition++]=e->getPositionOnStrand();
			outgoingMessage[outputPosition++]=e->getStrand();
			processed++;
		}

		e=e->getNext();
	}
	outgoingMessage[outputPosition++]=(uint64_t)e;
	Message aMessage(outgoingMessage,outputPosition,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_VERTEX_READS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_VERTEX_INFO(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int bufferPosition=0;
	Kmer vertex;
	vertex.unpack(incoming,&bufferPosition);
	Kmer complement=m_parameters->_complementVertex(&vertex);
	bool lower=vertex<complement;
	Vertex*node=m_subgraph->find(&vertex);
	#ifdef ASSERT
	assert(node!=NULL);
	#endif

	ReadAnnotation*e=m_subgraph->getReads(&vertex);
	int n=0;
	while(e!=NULL){
		if(e->isLower()==lower){
			n++;
		}
		e=e->getNext();
	}
	e=m_subgraph->getReads(&vertex);

	uint64_t wave=incoming[bufferPosition++];
	int progression=incoming[bufferPosition++];
	// add direction in the graph
	Direction*d=(Direction*)m_directionsAllocator->allocate(sizeof(Direction));
	d->constructor(wave,progression,lower);
	m_subgraph->addDirection(&vertex,d);

	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	outgoingMessage[0]=node->getCoverage(&vertex);
	outgoingMessage[1]=node->getEdges(&vertex);
	outgoingMessage[2]=n;
	int pos=5;
	int processed=0;
	int maximumToReturn=(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t)-5)/4;
	e=m_subgraph->getReads(&vertex);
	while(e!=NULL&&processed<maximumToReturn){
		if(e->isLower()==lower){
			outgoingMessage[pos++]=e->getRank();
			outgoingMessage[pos++]=e->getReadIndex();
			outgoingMessage[pos++]=e->getPositionOnStrand();
			outgoingMessage[pos++]=e->getStrand();
			processed++;
		}

		e=e->getNext();
	}

	outgoingMessage[3]=(uint64_t)e;
	outgoingMessage[4]=processed;
	Message aMessage(outgoingMessage,pos,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_VERTEX_INFO_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_DELETE_VERTEX(Message*message){
	#ifdef ASSERT
	assert(!m_subgraph->frozen());
	#endif

	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(incoming,&bufferPosition);

		Vertex*node=m_subgraph->find(&vertex);

		if(node==NULL){
			continue;
		}

		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		// delete the vertex
		m_subgraph->remove(&vertex);

		#ifdef ASSERT
		assert(m_subgraph->find(&vertex)==NULL);
		#endif
	}
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_DELETE_VERTEX_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);

	for(int i=0;i<count;i+=period){
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(incoming,&bufferPosition);
		Vertex*node=m_subgraph->find(&vertex);
		#ifdef ASSERT
		if(node==NULL){
			cout<<__func__<<" does not exist: "<<idToWord(&vertex,*m_wordSize,m_parameters->getColorSpaceMode())<<endl;
		}
		assert(node!=NULL);
		#endif
		outgoingMessage[i]=node->getEdges(&vertex);
		outgoingMessage[i+1]=node->getCoverage(&vertex);
	}
	
	Message aMessage(outgoingMessage,count,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_CHECK_VERTEX(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	
	int outgoingCount=0;
	int period=1+KMER_U64_ARRAY_SIZE;
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	for(int i=0;i<count;i+=period){
		int task=incoming[i+0];
		Kmer vertex;
		int bufferPosition=i+1;
		vertex.unpack(incoming,&bufferPosition);
		Vertex*node=m_subgraph->find(&vertex);
		if(node!=NULL){
			int parents=node->getIngoingEdges(&vertex,(*m_wordSize)).size();
			int children=node->getOutgoingEdges(&vertex,(*m_wordSize)).size();
			int coverage=node->getCoverage(&vertex);
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
	for(int i=0;i<count;i+=2*KMER_U64_ARRAY_SIZE){
		int bufferPosition=i;
		Kmer prefix;
		prefix.unpack(incoming,&bufferPosition);
		Kmer suffix;
		suffix.unpack(incoming,&bufferPosition);

		Vertex*node=m_subgraph->find(&suffix);

		if(node==NULL){ // node already deleted, don't need to delete the edges.
			continue;
		}

		#ifdef ASSERT
		assert(node!=NULL);
		int before=node->getOutgoingEdges(&suffix,*m_wordSize).size();
		#endif

		/* the edge might already be deleted if the tip is within another tip. */

		node->deleteIngoingEdge(&suffix,&prefix,(*m_wordSize));

		#ifdef ASSERT
		int after=node->getOutgoingEdges(&suffix,*m_wordSize).size();
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
	for(int i=0;i<count;i+=2*KMER_U64_ARRAY_SIZE){
		int bufferPosition=i;
		Kmer prefix;
		prefix.unpack(incoming,&bufferPosition);
		Kmer suffix;
		suffix.unpack(incoming,&bufferPosition);

		Vertex*node=m_subgraph->find(&prefix);

		if(node==NULL){ // node already deleted, don't need to delete the edges.
			continue;
		}
	
		#ifdef ASSERT
		assert(node!=NULL);
		int before=node->getOutgoingEdges(&suffix,*m_wordSize).size();
		#endif

		/* the edge might already be deleted if the tip is within another tip. */
		node->deleteOutgoingEdge(&prefix,&suffix,(*m_wordSize));
		
		#ifdef ASSERT
		int after=node->getOutgoingEdges(&suffix,*m_wordSize).size();
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
	if(m_parameters->runReducer()){
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

void MessageProcessor::call_RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY(Message*message){}


/*
 * <--vertex--><--pointer--><--numberOfMates--><--mates -->
 */
void MessageProcessor::call_RAY_MPI_TAG_VERTEX_READS_FROM_LIST(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	Kmer vertex;
	for(int i=0;i<vertex.getNumberOfU64();i++){
		vertex.setU64(i,incoming[i]);
	}
	Kmer complement=m_parameters->_complementVertex(&vertex);
	int numberOfMates=incoming[KMER_U64_ARRAY_SIZE+1];
	bool lower=vertex<complement;
	ReadAnnotation*e=(ReadAnnotation*)incoming[KMER_U64_ARRAY_SIZE+0];
	#ifdef ASSERT
	assert(e!=NULL);
	#endif
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int processed=0;
	int pos=1;
	set<uint64_t> localIndex;
	for(int i=0;i<numberOfMates;i++){
		localIndex.insert(incoming[KMER_U64_ARRAY_SIZE+2+i]);
	}

	while(e!=NULL){
		if(e->isLower()==lower){
			uint64_t uniqueId=getPathUniqueId(e->getRank(),e->getReadIndex());
			if(localIndex.count(uniqueId)>0){
				outgoingMessage[pos++]=e->getRank();
				outgoingMessage[pos++]=e->getReadIndex();
				outgoingMessage[pos++]=e->getPositionOnStrand();
				outgoingMessage[pos++]=e->getStrand();
				processed++;
			}
		}

		e=e->getNext();
	}
	outgoingMessage[0]=processed;
	Message aMessage(outgoingMessage,1+processed*4,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY,rank);
	m_outbox->push_back(aMessage);
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
	m_verticesExtractor->scheduleReduction(m_outbox,rank);
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

void MessageProcessor::call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_INDEX_SEQUENCES;
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

	#ifdef ASSERT
	assert(!m_subgraph->frozen());
	#endif
	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		Kmer l;
		int pos=i;
		l.unpack(incoming,&pos);

		if((*m_last_value)!=(int)m_subgraph->size() && (int)m_subgraph->size()%100000==0){
			(*m_last_value)=m_subgraph->size();
			printf("Rank %i has %i vertices\n",rank,(int)m_subgraph->size());
			fflush(stdout);

			if(m_parameters->showMemoryUsage()){
				showMemoryUsage(rank);
			}
		}

		Vertex*tmp=m_subgraph->insert(&l);
		#ifdef ASSERT
		assert(tmp!=NULL);
		#endif
		if(m_subgraph->inserted()){
			tmp->constructor(); 
		}
		tmp->setCoverage(&l,tmp->getCoverage(&l)+1);
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
		if(m_parameters->runReducer()){
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

	for(int i=0;i<(int)length;i+=2*KMER_U64_ARRAY_SIZE){
		int pos=i;
		Kmer prefix;
		prefix.unpack(incoming,&pos);
		Kmer suffix;
		suffix.unpack(incoming,&pos);

		Vertex*node=m_subgraph->find(&prefix);

		#ifdef ASSERT
		if(node==NULL){
			cout<<"Rank="<<rank<<" "<<__func__<<" "<<idToWord(&prefix,(*m_wordSize),m_parameters->getColorSpaceMode())<<" does not exist"<<endl;
		}
		assert(node!=NULL);
		#endif

		node->addOutgoingEdge(&prefix,&suffix,(*m_wordSize));
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_OUT_EDGES_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION(Message*message){
	// wait for everyone
	m_messagesHandler->barrier();
	(*m_mode_send_vertices)=true;
	(*m_mode)=RAY_SLAVE_MODE_EXTRACT_VERTICES;
	m_verticesExtractor->constructor(size,m_parameters);
	if(m_parameters->runReducer()){
		m_verticesExtractor->enableReducer();
	}

	(*m_mode_send_vertices_sequence_id)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY(Message*message){
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;
	int length=count;

	for(int i=0;i<(int)length;i+=2*KMER_U64_ARRAY_SIZE){
		int bufferPosition=i;
		Kmer prefix;
		prefix.unpack(incoming,&bufferPosition);
		Kmer suffix;
		suffix.unpack(incoming,&bufferPosition);

		Vertex*node=m_subgraph->find(&suffix);

		#ifdef ASSERT
		assert(node!=NULL);
		#endif

		node->addIngoingEdge(&suffix,&prefix,(*m_wordSize));

		#ifdef ASSERT
		vector<Kmer>inEdges=node->getIngoingEdges(&suffix,m_parameters->getWordSize());
		bool found=false;
		for(int j=0;j<(int)inEdges.size();j++){
			if(inEdges[j]==prefix){
				found=true;
				break;
			}
		}
		assert(found);
		#endif
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_IN_EDGES_DATA_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message){
	#ifdef ASSERT
	m_verticesExtractor->assertBuffersAreEmpty();
	#endif

	if(m_parameters->runReducer()){
		m_verticesExtractor->updateThreshold(m_subgraph);
		printf("Rank %i: %lu -> %lu\n",rank,m_lastSize,m_subgraph->size());
	}

	// freeze the forest. icy winter ahead.
	m_subgraph->freeze();
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

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(rank);
		fflush(stdout);
	}

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
	(*m_numberOfMachinesDoneSendingCoverage)++;
	if((*m_numberOfMachinesDoneSendingCoverage)==size){
		(*m_master_mode)=RAY_MASTER_MODE_SEND_COVERAGE_VALUES;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_COVERAGE_VALUES(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(*m_minimumCoverage)=incoming[0];
	(*m_peakCoverage)=incoming[1];
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_RECEIVED_COVERAGE_INFORMATION,rank);
	m_outbox->push_back(aMessage);
	m_oa->constructor((*m_peakCoverage));
	m_parameters->setPeakCoverage(*m_peakCoverage);
	m_parameters->setMinimumCoverage(*m_minimumCoverage);
	m_parameters->setRepeatCoverage(incoming[2]);
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
	seedingIterator.constructor(m_subgraph,*m_wordSize,m_parameters);
	while(seedingIterator.hasNext()){
		size++;
		Vertex*node=seedingIterator.next();
		edgesDistribution[node->getIngoingEdges(&(node->m_lowerKey),(*m_wordSize)).size()][node->getOutgoingEdges(&(node->m_lowerKey),(*m_wordSize)).size()]++;
	}
	#ifdef ASSERT
	for(map<int,map<int,int> >::iterator i=edgesDistribution.begin();i!=edgesDistribution.end();++i){
		for(map<int,int>::iterator j=i->second.begin();j!=i->second.end();++j){
		}
	}
	#endif
}

void MessageProcessor::call_RAY_MPI_TAG_GET_COVERAGE_AND_MARK(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	Kmer vertex;
	int bufferPosition=0;
	vertex.unpack(incoming,&bufferPosition);
	Vertex*node=m_subgraph->find(&vertex);
	#ifdef ASSERT
	assert(node!=NULL);
	#endif
	uint64_t coverage=node->getCoverage(&vertex);
	message2[0]=coverage;
	message2[1]=node->getEdges(&vertex);
	Kmer rc=m_parameters->_complementVertex(&vertex);
	bool lower=vertex<rc;
	uint64_t wave=incoming[1];
	int progression=incoming[2];
	// mark direction in the graph
	Direction*e=(Direction*)m_directionsAllocator->allocate(sizeof(Direction));
	e->constructor(wave,progression,lower);
	m_subgraph->addDirection(&vertex,e);

	Message aMessage(message2,2,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*sizeof(uint64_t));
	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(incoming,&bufferPosition);

		string kmerStr=idToWord(&vertex,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
		Vertex*node=m_subgraph->find(&vertex);
		#ifdef ASSERT
		if(node==NULL){
			cout<<"Rank="<<rank<<" "<<__func__<<" "<<idToWord(&vertex,(*m_wordSize),m_parameters->getColorSpaceMode())<<" does not exist"<<endl;
		}
		assert(node!=NULL);
		#endif
		uint64_t coverage=node->getCoverage(&vertex);
		message2[i]=coverage;
	}
	Message aMessage(message2,count,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY,rank);
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

	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(incoming,&bufferPosition);
		vector<Kmer> outgoingEdges=m_subgraph->find(&vertex)->getOutgoingEdges(&vertex,*m_wordSize);
		int outputPosition=(1+4*KMER_U64_ARRAY_SIZE)*i;
		message2[outputPosition++]=outgoingEdges.size();
		for(int j=0;j<(int)outgoingEdges.size();j++){
			outgoingEdges[j].pack(message2,&outputPosition);
		}
	}
	Message aMessage(message2,(count/KMER_U64_ARRAY_SIZE)*(1+4*KMER_U64_ARRAY_SIZE),MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*10*sizeof(uint64_t));
	int outputPosition=0;
	Kmer vertex;
	int bufferPosition=0;
	vertex.unpack(incoming,&bufferPosition);

	Vertex*node=m_subgraph->find(&vertex);

	#ifdef ASSERT
	assert(node!=NULL);
	#endif

	vector<Kmer> outgoingEdges=node->getOutgoingEdges(&vertex,*m_wordSize);
	vector<Kmer> ingoingEdges=node->getIngoingEdges(&vertex,*m_wordSize);
	
	message2[outputPosition++]=outgoingEdges.size();
	for(int i=0;i<(int)outgoingEdges.size();i++){
		outgoingEdges[i].pack(message2,&outputPosition);
	}
	message2[outputPosition++]=ingoingEdges.size();
	for(int i=0;i<(int)ingoingEdges.size();i++){
		ingoingEdges[i].pack(message2,&outputPosition);
	}
	Message aMessage(message2,outputPosition,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;

	int bufferPosition=0;
	int numberOfOutgoingEdges=incoming[bufferPosition++];
	(m_seedingData->m_SEEDING_receivedOutgoingEdges).clear();

	for(int i=0;i<numberOfOutgoingEdges;i++){
		Kmer a;
		a.unpack(incoming,&bufferPosition);
		(m_seedingData->m_SEEDING_receivedOutgoingEdges).push_back(a);
	}

	int numberOfIngoingEdges=incoming[bufferPosition++];
	m_seedingData->m_SEEDING_receivedIngoingEdges.clear();

	for(int i=0;i<numberOfIngoingEdges;i++){
		Kmer a;
		a.unpack(incoming,&bufferPosition);
		m_seedingData->m_SEEDING_receivedIngoingEdges.push_back(a);
	}

	(m_seedingData->m_SEEDING_edgesReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedOutgoingEdges).clear();
	int bufferPosition=0;
	int count=incoming[bufferPosition++];
	for(int i=0;i<(int)count;i++){
		Kmer nextVertex;
		nextVertex.unpack(incoming,&bufferPosition);
		(m_seedingData->m_SEEDING_receivedOutgoingEdges).push_back(nextVertex);
	}

	(m_seedingData->m_SEEDING_edgesReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_SEEDING_IS_OVER(Message*message){
	(*m_numberOfRanksDoneSeeding)++;
	if((*m_numberOfRanksDoneSeeding)==size){
		(*m_numberOfRanksDoneSeeding)=0;
		(*m_master_mode)=RAY_MASTER_MODE_DO_NOTHING;
		for(int i=0;i<size;i++){
			Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,i,RAY_MPI_TAG_REQUEST_SEED_LENGTHS,rank);
			m_outbox->push_back(aMessage);
		}
	}
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_SEED_LENGTHS(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_SEND_SEED_LENGTHS;
	m_seedingData->m_initialized=false;
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_SEED_LENGTHS(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();
	for(int i=0;i<count;i+=2){
		int seedLength=incoming[i];
		int number=incoming[i+1];
		m_seedingData->m_masterSeedLengths[seedLength]+=number;
	}
	
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS(Message*message){
	(*m_numberOfRanksDoneSeeding)++;

	if((*m_numberOfRanksDoneSeeding)==size){
		(*m_numberOfRanksDoneSeeding)=0;
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_DETECTION;
		m_seedingData->writeSeedStatistics();
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
		Kmer vertex;
		int pos=i;
		vertex.unpack(incoming,&pos);
		Vertex*node=m_subgraph->find(&vertex);
		#ifdef ASSERT
		if(node==NULL){
			cout<<"Rank="<<rank<<" "<<idToWord(&vertex,*m_wordSize,m_parameters->getColorSpaceMode())<<" does not exist."<<endl;
		}
		assert(node!=NULL);
		#endif 
		vector<Kmer> ingoingEdges=node->getIngoingEdges(&vertex,*m_wordSize);
		int outputPosition=i*5;
		message2[outputPosition++]=ingoingEdges.size();
		for(int j=0;j<(int)ingoingEdges.size();j++){
			ingoingEdges[j].unpack(incoming,&outputPosition);
		}
	}
	Message aMessage(message2,count*5,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(m_seedingData->m_SEEDING_receivedIngoingEdges).clear();
	int bufferPosition=0;
	int numberOfElements=incoming[bufferPosition++];
	for(int i=0;i<(int)numberOfElements;i++){
		Kmer a;
		a.unpack(incoming,&bufferPosition);
		(m_seedingData->m_SEEDING_receivedIngoingEdges).push_back(a);
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
	Kmer vertex;
	int pos=0;
	vertex.unpack(incoming,&pos);
	#ifdef ASSERT
	Vertex*node=m_subgraph->find(&vertex);
	assert(node!=NULL);
	#endif
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
	message2[0]=m_subgraph->isAssembled(&vertex);

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
	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE+4){
		m_count++;
		Kmer vertex;
		for(int j=0;j<vertex.getNumberOfU64();j++){
			vertex.setU64(j,incoming[i+j]);
		}
		Kmer complement=m_parameters->_complementVertex(&vertex);
		bool lower=vertex.isLower(&complement);
		int rank=incoming[i+vertex.getNumberOfU64()];
		int sequenceIdOnDestination=(int)incoming[i+vertex.getNumberOfU64()+1];
		int positionOnStrand=incoming[i+vertex.getNumberOfU64()+2];
		char strand=(char)incoming[i+vertex.getNumberOfU64()+3];
		Vertex*node=m_subgraph->find(&vertex);

		int coverage=node->getCoverage(&vertex);
		if(node==NULL){
			continue;
		}

		if(coverage==1){
			continue;
		}
		
		ReadAnnotation*e=(ReadAnnotation*)m_si->getAllocator()->allocate(sizeof(ReadAnnotation));
		#ifdef ASSERT
		assert(e!=NULL);
		#endif
		e->constructor(rank,sequenceIdOnDestination,positionOnStrand,strand,lower);

		m_subgraph->addRead(&vertex,e);
	}
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(message2,count,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY(Message*message){
	m_si->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	char strand=incoming[2];
	Kmer vertex=(*m_myReads)[incoming[0]]->getVertex(incoming[1],(*m_wordSize),strand,m_parameters->getColorSpaceMode());
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
	int bufferPosition=0;
	vertex.pack(message2,&bufferPosition);
	Message aMessage(message2,bufferPosition,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY(Message*message){
	uint64_t*buffer=(uint64_t*)message->getBuffer();
	(m_ed->m_EXTENSION_read_vertex_received)=true;
	int bufferPosition=0;
	m_ed->m_EXTENSION_receivedReadVertex.unpack(buffer,&bufferPosition);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_LENGTH(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	int index=incoming[0];
	#ifdef ASSERT
	assert(index<(int)m_myReads->size());
	#endif
	Read*read=(*m_myReads)[index];
	#ifdef ASSERT
	assert(read!=NULL);
	#endif
	int length=read->length();
	
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(3*sizeof(uint64_t));
	int pos=0;
	message2[pos++]=length;
	message2[pos++]=read->getForwardOffset();
	message2[pos++]=read->getReverseOffset();

	Message aMessage(message2,pos,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY,rank);
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
	for(int i=0;i<count;i+=(KMER_U64_ARRAY_SIZE+2)){
		Kmer vertex;
		int pos=i;
		vertex.unpack(incoming,&pos);
		Kmer rc=m_parameters->_complementVertex(&vertex);
		bool lower=vertex<rc;
		#ifdef ASSERT
		Vertex*node=m_subgraph->find(&vertex);
		assert(node!=NULL);
		#endif
		uint64_t wave=incoming[pos++];
		
		#ifdef ASSERT
		if(getRankFromPathUniqueId(wave)>=size){
			cout<<"Invalid rank: "<<getRankFromPathUniqueId(wave)<<" maximum is "<<size-1<<endl;
		}
		assert(getRankFromPathUniqueId(wave)<size);
		#endif

		int progression=incoming[pos++];
		Direction*e=(Direction*)m_directionsAllocator->allocate(sizeof(Direction));
		e->constructor(wave,progression,lower);

		m_subgraph->addDirection(&vertex,e);
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
	m_fusionData->initialise();
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
	int pos=0;
	Kmer vertex;
	vertex.unpack(incoming,&pos);
	Vertex*node=m_subgraph->find(&vertex);

	#ifdef ASSERT
	if(node==NULL){
		cout<<"Source="<<message->getSource()<<" Destination="<<rank<<" "<<idToWord(&vertex,*m_wordSize,m_parameters->getColorSpaceMode())<<" does not exist, aborting"<<endl;
		cout.flush();
	}
	assert(node!=NULL);
	#endif

	vector<Direction> paths=m_subgraph->getDirections(&vertex);
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message2[0]=paths.size();
	message2[1]=node->getCoverage(&vertex);
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
	int count=message->getCount();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	for(int i=0;i<count;i++){
		uint64_t id=incoming[i];
		int length=0;
		if(m_fusionData->m_FUSION_identifier_map.count(id)>0){
			length=(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size();
		}

		message2[i]=length;
	}
	Message aMessage(message2,count,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		Kmer vertex;
		int pos=i;
		vertex.unpack(incoming,&pos);
		Vertex*node=m_subgraph->find(&vertex);
		int coverage=node->getCoverage(&vertex);
		vector<Direction> paths=m_subgraph->getDirections(&vertex);
		message2[i*4+0]=coverage;
		message2[i*4+1]=(paths.size()==1);
		if(paths.size()==1){
			message2[4*i+2]=paths[0].getWave();
			message2[4*i+3]=paths[0].getProgression();
		}
	}

	Message aMessage(message2,(count/KMER_U64_ARRAY_SIZE)*4,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FUSION_receivedLength=incoming[0];
	m_fusionData->m_FUSION_pathLengthReceived=true;
}

/*
input: Kmer ; index
output: Kmer ; index ; list
*/
void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)buffer;
	Kmer vertex;
	int pos=0;
	vertex.unpack(incoming,&pos);

	int firstPathId=incoming[pos];
	vector<Direction> paths=m_subgraph->getDirections(&vertex);

	int availableElements=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int outputPosition=0;
	vertex.pack(message2,&outputPosition);
	int origin=outputPosition;
	outputPosition++;
	while(firstPathId<(int)paths.size() && (outputPosition+2)<availableElements){
		#ifdef ASSERT
		assert(firstPathId<(int)paths.size());
		#endif
		uint64_t pathId=paths[firstPathId].getWave();
		int progression=paths[firstPathId].getProgression();
		#ifdef ASSERT
		if(getRankFromPathUniqueId(pathId)>=size){
			cout<<"Invalid rank: "<<getRankFromPathUniqueId(pathId)<<" maximum is "<<size-1<<" Index: "<<firstPathId<<" Total: "<<paths.size()<<" Progression: "<<progression<<endl;
		}
		assert(getRankFromPathUniqueId(pathId)<size);
		#endif
		message2[outputPosition++]=pathId;
		message2[outputPosition++]=progression;
		firstPathId++;
	}
	message2[origin]=firstPathId;

	#ifdef ASSERT
	assert(outputPosition<=availableElements);
	assert(source<size);
	#endif

	if(firstPathId==(int)paths.size()){
		Message aMessage(message2,outputPosition,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END,rank);
		m_outbox->push_back(aMessage);
	}else{
		Message aMessage(message2,outputPosition,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY,rank);
		m_outbox->push_back(aMessage);
	}
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	Kmer vertex;
	int pos=0;
	vertex.unpack(incoming,&pos);
	int count=message->getCount();
	Kmer complement=m_parameters->_complementVertex(&vertex);
	bool lower=vertex<complement;
	int origin=pos;
	pos++;
	for(int i=pos;i<count;i+=2){
		uint64_t pathId=incoming[i];
		#ifdef ASSERT
		assert(getRankFromPathUniqueId(pathId)<size);
		#endif
		int position=incoming[i+1];
		m_fusionData->m_FUSION_receivedPath.constructor(pathId,position,lower);
		m_fusionData->m_Machine_getPaths_result.push_back(m_fusionData->m_FUSION_receivedPath);
	}

	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	int outputPosition=0;
	vertex.pack(message2,&outputPosition);
	message2[outputPosition++]=incoming[origin];
	Message aMessage(message2,outputPosition,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_ASK_VERTEX_PATHS,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	Kmer vertex;
	int bufferPosition=0;
	vertex.unpack(incoming,&bufferPosition);
	int count=message->getCount();
	Kmer complement=m_parameters->_complementVertex(&vertex);
	bufferPosition++;
	bool lower=vertex<complement;
	for(int i=bufferPosition;i<count;i+=2){
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
	Kmer kmer;
	int pos=0;
	kmer.unpack(incoming,&pos);
	#ifdef ASSERT
	Vertex*node=m_subgraph->find(&kmer);
	assert(node!=NULL);
	#endif
	vector<Direction> paths=m_subgraph->getDirections(&kmer);
	int i=incoming[1];
	Direction d=paths[i];
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(3*sizeof(uint64_t));
	int outputPosition=0;
	kmer.pack(message2,&outputPosition);
	message2[outputPosition++]=d.getWave();
	#ifdef ASSERT
	assert(getRankFromPathUniqueId(message2[1])<size);
	#endif
	message2[outputPosition++]=d.getProgression();
	Message aMessage(message2,outputPosition,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FUSION_path_received=true;
	Kmer vertex;
	int pos=0;
	vertex.unpack(incoming,&pos);
	Kmer complement=m_parameters->_complementVertex(&vertex);
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
	int count=message->getCount();
	for(int i=0;i<count;i++){
		int index=incoming[i];
		#ifdef ASSERT
		assert(index<(int)m_myReads->size());
		#endif
		message2[i]=(*m_myReads)[index]->hasPairedRead();
	}
	Message aMessage(message2,count,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY,rank);
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
	bool appendReverseComplement=(message->getCount()==0);
	// clearing old data too!.
	m_fusionData->m_FINISH_pathLengths.clear();

	// clear graph
	GridTableIterator iterator;
	iterator.constructor(m_subgraph,*m_wordSize,m_parameters);
	while(iterator.hasNext()){
		iterator.next();
		Kmer key=*(iterator.getKey());
		m_subgraph->clearDirections(&key);
	}

	m_directionsAllocator->clear();

	// add the FINISHING bits
	for(int i=0;i<(int)m_fusionData->m_FINISH_newFusions.size();i++){
		(m_ed->m_EXTENSION_contigs).push_back((m_fusionData->m_FINISH_newFusions)[i]);
	}

	m_fusionData->m_FINISH_newFusions.clear();

	vector<vector<Kmer> > fusions;
	for(int i=0;i<(int)(m_ed->m_EXTENSION_contigs).size();i++){
		uint64_t id=(m_ed->m_EXTENSION_identifiers)[i];
		if(m_fusionData->m_FUSION_eliminated.count(id)==0){
			fusions.push_back((m_ed->m_EXTENSION_contigs)[i]);
			if(!appendReverseComplement)
				continue;

			vector<Kmer> rc;
			for(int j=(m_ed->m_EXTENSION_contigs)[i].size()-1;j>=0;j--){
				rc.push_back(complementVertex(&((m_ed->m_EXTENSION_contigs)[i][j]),*m_wordSize,
					m_parameters->getColorSpaceMode()));
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
	Kmer a=(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]][position];
	int pos=0;
	a.pack(messageBytes,&pos);
	Message aMessage(messageBytes,pos,MPI_UNSIGNED_LONG_LONG,source,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_VERTEX_REPLY(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_fusionData->m_FINISH_vertex_received=true;
	int pos=0;
	m_fusionData->m_FINISH_received_vertex.unpack(incoming,&pos);
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
		m_parameters->addDistance(incoming[i+0],incoming[i+1],incoming[i+2]);
	}

	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY,rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES;
	m_library->allocateBuffers();
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED(Message*message){
	(*m_numberOfRanksDoneSendingDistances)++;
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
		m_parameters->addLibraryData(i,average,deviation);
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

	uint64_t*messageBytes=(uint64_t*)m_outboxAllocator->allocate(toAllocate);
	messageBytes[0]=t->getRank();
	messageBytes[1]=t->getId();
	messageBytes[2]=t->getLibrary();
	messageBytes[3]=(*m_myReads)[index]->getType();
	messageBytes[4]=m_myReads->at(index)->length();
	char*dest=(char*)(messageBytes+5);
	memcpy(dest,m_myReads->at(index)->getRawSequence(),m_myReads->at(index)->getRequiredBytes());
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
	tmp.getSeq(buffer2,m_parameters->getColorSpaceMode(),false);
	seedExtender->m_receivedString=buffer2;
	seedExtender->m_sequenceReceived=true;
}

void MessageProcessor::call_RAY_MPI_TAG_I_FINISHED_SCAFFOLDING(Message*message){
	m_scaffolder->m_numberOfRanksFinished++;
	if(m_scaffolder->m_numberOfRanksFinished==m_parameters->getSize()){
		m_scaffolder->solve();
		(*m_master_mode)=RAY_MASTER_MODE_WRITE_SCAFFOLDS;
		m_scaffolder->m_initialised=false;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_GET_CONTIG_CHUNK(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	uint64_t contigId=incoming[0];
	int position=incoming[1];
	int index=m_fusionData->m_FUSION_identifier_map[contigId];
	int length=m_ed->m_EXTENSION_contigs[index].size();
	int outputPosition=0;
	uint64_t*messageContent=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	while(position<length
	 && outputPosition<(int)(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t))){
		m_ed->m_EXTENSION_contigs[index][position++].pack(messageContent,&outputPosition);
	}
	
	Message aMessage(messageContent,outputPosition,
		MPI_UNSIGNED_LONG_LONG,message->getSource(),RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY,
		m_parameters->getRank());
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_SCAFFOLDER(Message*message){
	(*m_mode)=RAY_SLAVE_MODE_SCAFFOLDER;
}

void MessageProcessor::setScaffolder(Scaffolder*a){
	m_scaffolder=a;
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
		OpenAssemblerChooser*m_oa,
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
	this->m_parameters=parameters;
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
	#define MACRO_LIST_ITEM(x) m_methods[x]=&MessageProcessor::call_ ## x ;
	#include <communication/mpi_tag_macros.h>
	#undef MACRO_LIST_ITEM
}

void MessageProcessor::setVirtualCommunicator(VirtualCommunicator*a){
	m_virtualCommunicator=a;
}
