/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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
#include <core/OperatingSystem.h>
#include <assert.h>
#include <structures/Read.h>
#include <communication/MessageProcessor.h>
#include <structures/StaticVector.h>
#include <core/common_functions.h>
#include <structures/ReadAnnotation.h>
#include <structures/SplayTree.h>
#include <structures/Direction.h>
#include <structures/SplayNode.h>
#include <structures/SplayTreeIterator.h>
#include <assembler/FusionData.h>
#include <core/Parameters.h>


void MessageProcessor::call_RAY_MPI_TAG_LOAD_SEQUENCES(Message*message){
	uint32_t*incoming=(uint32_t*)message->getBuffer();
	for(int i=0;i<(int)incoming[0];i++){
		if(m_parameters->hasOption("-debug-partitioner"))
			cout<<"Rank "<<m_parameters->getRank()<<" RAY_MPI_TAG_LOAD_SEQUENCES File "<<i<<" "<<incoming[i+1]<<endl;
		m_parameters->setNumberOfSequences(i,incoming[1+i]);
	}
}

void MessageProcessor::call_RAY_MPI_TAG_CONTIG_INFO(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	m_scaffolder->addMasterContig(incoming[0],incoming[1]);
	
	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(outgoingMessage,message->getCount(),
		message->getSource(),RAY_MPI_TAG_CONTIG_INFO_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_CONTIG_INFO_REPLY(Message*message){
}

void MessageProcessor::processMessage(Message*message){
	int tag=message->getTag();
	MessageProcessorHandler f=m_methods[tag];
	(this->*f)(message);
}

void MessageProcessor::call_RAY_MPI_TAG_SCAFFOLDING_LINKS(Message*message){
	uint64_t*incoming=(uint64_t*)message->getBuffer();

	int position=0;

	uint64_t leftContig=incoming[position++];
	char leftStrand=incoming[position++];
	uint64_t rightContig=incoming[position++];
	char rightStrand=incoming[position++];
	int number=incoming[position++];
	int average=incoming[position++];
	int standardDeviation=incoming[position++];

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
	
	SummarizedLink link(leftContig,leftStrand,rightContig,rightStrand,average,number,standardDeviation);
	m_scaffolder->addMasterLink(&link);

	uint64_t*outgoingMessage=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(outgoingMessage,message->getCount(),
		message->getSource(),RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY,m_rank);
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
		#ifdef ASSERT
		if(readId>=(int)m_myReads->size())
			cout<<"Fatal Error: ReadIndex: "<<readId<<" but Reads: "<<m_myReads->size()<<endl;
		assert(readId<(int)m_myReads->size());
		#endif
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

	Message aMessage(outgoingMessage,outputPosition,message->getSource(),RAY_MPI_TAG_GET_READ_MARKERS_REPLY,m_rank);
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
		/** data: readLength, rank, id, library */
		outgoingMessage[j++]=readLength;
		if(!read->hasPairedRead()){
			outgoingMessage[j++]=MAX_NUMBER_OF_MPI_PROCESSES;
			outgoingMessage[j++]=MAX_NUMBER_OF_MPI_PROCESSES;
			outgoingMessage[j++]=MAX_NUMBER_OF_MPI_PROCESSES;
		}else{
			PairedRead*mate=read->getPairedRead();
			outgoingMessage[j++]=mate->getRank();
			outgoingMessage[j++]=mate->getId();
			outgoingMessage[j++]=mate->getLibrary();
		}
	}

	Message aMessage(outgoingMessage,j,message->getSource(),RAY_MPI_TAG_GET_READ_MATE_REPLY,m_rank);
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
	Message aMessage(outgoingMessage,j,message->getSource(),RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_SET_WORD_SIZE(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	(*m_wordSize)=incoming[0];
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
	Message aMessage(outgoingMessage,outputPosition,message->getSource(),RAY_MPI_TAG_VERTEX_READS_REPLY,m_rank);
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

/*
	uint64_t wave=incoming[bufferPosition++];
	int progression=incoming[bufferPosition++];
	// add direction in the graph
	Direction*d=(Direction*)m_directionsAllocator->allocate(sizeof(Direction));
	d->constructor(wave,progression,lower);
	m_subgraph->addDirection(&vertex,d);
*/
	m_subgraph->find(&vertex)->assemble();

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
	Message aMessage(outgoingMessage,pos,message->getSource(),RAY_MPI_TAG_VERTEX_INFO_REPLY,m_rank);
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
		if(node==NULL){
			outgoingMessage[i]=0;
			outgoingMessage[i+1]=1;
		}else{
			outgoingMessage[i]=node->getEdges(&vertex);
			outgoingMessage[i+1]=node->getCoverage(&vertex);
		}
	}
	
	Message aMessage(outgoingMessage,count,message->getSource(),RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_BUILD_GRAPH(Message*message){
	m_verticesExtractor->constructor(m_parameters->getSize(),m_parameters,m_subgraph);
	*m_mode_send_vertices_sequence_id=0;
	*m_mode=RAY_SLAVE_MODE_EXTRACT_VERTICES;
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
	Message aMessage(outgoingMessage,1+processed*4,message->getSource(),RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_WELCOME(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(Message*message){

	/* read the Graph checkpoint here */
	if(m_parameters->hasCheckpoint("GenomeGraph")){
		cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint GenomeGraph"<<endl;
		ifstream f(m_parameters->getCheckpointFile("GenomeGraph").c_str());
		uint64_t n=0;
		f.read((char*)&n,sizeof(uint64_t));
		for(uint64_t  i=0;i<n;i++){
			if(i%100000==0){
				cout<<"Rank "<<m_parameters->getRank()<<" loading checkpoint GenomeGraph ["<<i<<"/"<<n<<"]"<<endl;
			}
			Kmer kmer;
			kmer.read(&f);
			int coverage=0;
			f.read((char*)&coverage,sizeof(int));
			Vertex*tmp=m_subgraph->insert(&kmer);

			/* we only want to construct it once. */
			if(m_subgraph->inserted()){
				tmp->constructor();
				tmp->setCoverage(&kmer,coverage);
			}
			int parents=0;
			f.read((char*)&parents,sizeof(int));
			for(int j=0;j<parents;j++){
				Kmer parent;
				parent.read(&f);
				tmp->addIngoingEdge(&kmer,&parent,m_parameters->getWordSize());
			}
			int children=0;
			f.read((char*)&children,sizeof(int));

			for(int j=0;j<children;j++){
				Kmer child;
				child.read(&f);
				tmp->addOutgoingEdge(&kmer,&child,m_parameters->getWordSize());
			}

			#ifdef ASSERT
			vector<Kmer> parentKmers=tmp->getIngoingEdges(&kmer,m_parameters->getWordSize());
			vector<Kmer> childKmers=tmp->getOutgoingEdges(&kmer,m_parameters->getWordSize());
			if((int)parentKmers.size()!=parents){
				cout<<"Expected: "<<parents<<" Actual: "<<parentKmers.size()<<endl;
			}
			assert((int)parentKmers.size()==parents);
			assert((int)childKmers.size()==children);
			#endif
		}
		f.close();

		/* complete the resizing ... */
		/* Otherwise, the code will fail later on */
		m_subgraph->completeResizing();

		#ifdef ASSERT
		assert(m_subgraph->size()==n);
		#endif

		cout<<"Rank "<<m_parameters->getRank()<<" loading checkpoint GenomeGraph ["<<n<<"/"<<n<<"]"<<endl;
		cout<<"Rank "<<m_parameters->getRank()<<" loaded "<<n<<" vertices from checkpoint GenomeGraph"<<endl;
	}

	/* write checkpoint if necessary */
	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("GenomeGraph")){
		/* announce the user that we are writing a checkpoint */
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint GenomeGraph"<<endl;
		cout.flush();

		ofstream f(m_parameters->getCheckpointFile("GenomeGraph").c_str());

		GridTableIterator iterator;
		iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);
		uint64_t theSize=m_subgraph->size();

		f.write((char*)&theSize,sizeof(uint64_t));

		while(iterator.hasNext()){
			Vertex*node=iterator.next();
			Kmer key=*(iterator.getKey());
			node->write(&key,&f,m_parameters->getWordSize());
		}

		f.close();
	}

}

void MessageProcessor::call_RAY_MPI_TAG_SEQUENCES_READY(Message*message){
	(*m_sequence_ready_machines)++;
	if(*m_sequence_ready_machines==m_size){
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

	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		Kmer l;
		int pos=i;
		l.unpack(incoming,&pos);

		/* TODO: remove call to reverseComplement, this if should never be picked up because only the lowest k-mers are sent
 *
 * I am not sure it would work but if it does that would reduces the number of sent messages */
		Kmer reverseComplement=l.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
		if(reverseComplement<l)
			continue;

		if((*m_last_value)!=(int)m_subgraph->size() && (int)m_subgraph->size()%100000==0){
			(*m_last_value)=m_subgraph->size();
			printf("Rank %i has %i vertices\n",m_rank,(int)m_subgraph->size());
			fflush(stdout);

			if(m_parameters->showMemoryUsage()){
				showMemoryUsage(m_rank);
			}
		}
		
		KmerCandidate*candidate=m_subgraph->getKmerAcademy()->find(&l);

		/** the candidate remained in the Bloom filter ! , good.*/
		if(candidate == NULL){
			continue;
		}

		#ifdef ASSERT
		assert(candidate!=NULL);
		#endif

		if(candidate->m_count<m_parameters->getMinimumCoverageToStore())
			continue;

		Vertex*tmp=m_subgraph->insert(&l);
		#ifdef ASSERT
		assert(tmp!=NULL);
		#endif
		if(m_subgraph->inserted()){
			tmp->constructor(); 
		}
		tmp->setCoverage(&l,tmp->getCoverage(&l)+1);
	}
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_VERTICES_DATA_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_VERTICES_DATA_REPLY(Message*message){
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_WRITE_KMERS_REPLY(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_WRITE_KMERS(Message*message){
	*m_mode=RAY_SLAVE_MODE_WRITE_KMERS;
}

void MessageProcessor::call_RAY_MPI_TAG_PURGE_NULL_EDGES(Message*message){
	m_subgraph->getKmerAcademy()->destructor();
	m_bloomFilter.destructor();

	m_subgraph->completeResizing();

	#ifdef ASSERT
	GridTableIterator iterator;
	iterator.constructor(m_subgraph,*m_wordSize,m_parameters);

	uint64_t n=0;

	while(iterator.hasNext()){
		iterator.next();
		n++;
	}

	if(n!= m_subgraph->size()){
		cout<<"Hilarious error: counted, expected: "<<m_subgraph->size()<<", actual: "<<n<<endl;
	}

	assert(n== m_subgraph->size());
	#endif


	printf("Rank %i has %i vertices (completed)\n",m_rank,(int)m_subgraph->size());
	fflush(stdout);

	m_subgraph->printStatistics();

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_rank);
		fflush(stdout);
	}

	*m_mode=RAY_SLAVE_MODE_PURGE_NULL_EDGES;
}

void MessageProcessor::call_RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_VERTICES_DISTRIBUTED(Message*message){
	(*m_numberOfMachinesDoneSendingVertices)++;
	if((*m_numberOfMachinesDoneSendingVertices)==m_size){
		m_verticesExtractor->setDistributionAsCompleted();
		(*m_master_mode)=RAY_MASTER_MODE_PURGE_NULL_EDGES;
	}
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

		if(node==NULL){
			continue; /* NULL because coverage is too low */
		}

		node->addOutgoingEdge(&prefix,&suffix,(*m_wordSize));
	}

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_OUT_EDGES_DATA_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_VERTICES_DISTRIBUTION(Message*message){
	m_bloomFilter.constructor();
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

		if(node==NULL){
			continue;
		}

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

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_IN_EDGES_DATA_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION(Message*message){
	int source=message->getSource();

	Message aMessage(NULL, 0, source, RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER(Message*message){
	(*m_numberOfMachinesReadyToSendDistribution)++;
	if((*m_numberOfMachinesReadyToSendDistribution)==m_size){
		(*m_master_mode)=RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS;
	}
}

/* we reply with an empty message */
void MessageProcessor::call_RAY_MPI_TAG_TEST_NETWORK_MESSAGE(Message*message){
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_TEST_NETWORK(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION(Message*message){
	m_subgraph->getKmerAcademy()->completeResizing();

	printf("Rank %i has %i k-mers (completed)\n",m_rank,(int)m_subgraph->getKmerAcademy()->size());
	fflush(stdout);

	m_subgraph->getKmerAcademy()->printStatistics();

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_rank);
		fflush(stdout);
	}

	(*m_mode_send_coverage_iterator)=0;
	(*m_mode_sendDistribution)=true;

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
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_COVERAGE_DATA_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_COVERAGE_END(Message*message){
	(*m_numberOfMachinesDoneSendingCoverage)++;
	if((*m_numberOfMachinesDoneSendingCoverage)==m_size){
		(*m_master_mode)=RAY_MASTER_MODE_SEND_COVERAGE_VALUES;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_COVERAGE_VALUES(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	m_parameters->setMinimumCoverage(incoming[0]);
	m_parameters->setPeakCoverage(incoming[1]);
	m_parameters->setRepeatCoverage(incoming[2]);
	m_oa->constructor();

	Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_READY_TO_SEED(Message*message){
	(*m_readyToSeed)++;
	if((*m_readyToSeed)==m_size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_SEEDING;
	}
}

// TODO: move checkpointing code in checkpointing.
void MessageProcessor::call_RAY_MPI_TAG_START_SEEDING(Message*message){
	/* read checkpoints ReadOffsets and OptimalMarkers */
	
	if(m_parameters->hasCheckpoint("OptimalMarkers") && m_parameters->hasCheckpoint("ReadOffsets")){
		cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint ReadOffsets"<<endl;
		ifstream f(m_parameters->getCheckpointFile("ReadOffsets").c_str());
		uint64_t n=0;
		f.read((char*)&n,sizeof(uint64_t));
		for(uint64_t i=0;i<n;i++){
			m_myReads->at(i)->readOffsets(&f);
		}
		f.close();

		cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint OptimalMarkers"<<endl;
		ifstream f2(m_parameters->getCheckpointFile("OptimalMarkers").c_str());

		n=0;
		f2.read((char*)&n,sizeof(uint64_t));
		uint64_t loaded=0;
		for(uint64_t i=0;i<n;i++){
			Kmer kmer;
			kmer.read(&f2);

			Kmer complement=kmer.complementVertex(m_parameters->getWordSize(),
				m_parameters->getColorSpaceMode());
			bool isLower=kmer<complement;
			int markers=0;
			f2.read((char*)&markers,sizeof(int));
			for(int j=0;j<markers;j++){
				loaded++;
				ReadAnnotation*marker=
					(ReadAnnotation*)m_si->getAllocator()->allocate(sizeof(ReadAnnotation));

				#ifdef ASSERT
				assert(marker!=NULL);
				#endif

				marker->read(&f2,isLower);

				Vertex*node=m_subgraph->find(&kmer);
	
				#ifdef ASSERT
				if(node==NULL){
					cout<<"Not found: "<<kmer.idToWord(m_parameters->getWordSize(),
						m_parameters->getColorSpaceMode())<<endl;
					cout<<"Markers: "<<markers<<endl;
					cout<<"Vertex: "<<i<<endl;
				}
				assert(node!=NULL);
				#endif

				node->addRead(&kmer,marker);
			}
		}
		f2.close();

		cout<<"Rank "<<m_parameters->getRank()<<" loaded "<<loaded<<" markers from checkpoint OptimalMarkers."<<endl;
	}

	/* write checkpoint */
	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("OptimalMarkers")){
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint ReadOffsets"<<endl;
		ofstream f(m_parameters->getCheckpointFile("ReadOffsets").c_str());
		uint64_t count=m_myReads->size();
		f.write((char*)&count,sizeof(uint64_t));
		for(int i=0;i<(int)m_myReads->size();i++){
			m_myReads->at(i)->writeOffsets(&f);
		}
		f.close();
	
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint OptimalMarkers"<<endl;
		ofstream f2(m_parameters->getCheckpointFile("OptimalMarkers").c_str());

		GridTableIterator iterator;
		iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);
	
		count=m_subgraph->size();
		f2.write((char*)&count,sizeof(uint64_t));

		while(iterator.hasNext()){
			Vertex*node=iterator.next();
			Kmer key=*(iterator.getKey());
			node->writeAnnotations(&key,&f2,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
		}

		f2.close();
	}



	if(m_parameters->showMemoryUsage()){
		int allocatedBytes=m_si->getAllocator()->getNumberOfChunks()*m_si->getAllocator()->getChunkSize();
		cout<<"Rank "<<m_parameters->getRank()<<": memory usage for  optimal read markers= "<<allocatedBytes/1024<<" KiB"<<endl;
	}

	#ifdef ASSERT
	assert(m_subgraph!=NULL);
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

	Message aMessage(message2,2,message->getSource(),RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY,m_rank);
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

		//string kmerStr=vertex.idToWord(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

		Vertex*node=m_subgraph->find(&vertex);
		int coverage=1;
		if(node!=NULL)
			coverage=node->getCoverage(&vertex);
		message2[i]=coverage;
	}
	Message aMessage(message2,count,source,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY,m_rank);
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
		Vertex*node=m_subgraph->find(&vertex);
		vector<Kmer> outgoingEdges;
		if(node!=NULL)
			outgoingEdges=node->getOutgoingEdges(&vertex,*m_wordSize);
		int outputPosition=(1+4*KMER_U64_ARRAY_SIZE)*i;
		message2[outputPosition++]=outgoingEdges.size();
		for(int j=0;j<(int)outgoingEdges.size();j++){
			outgoingEdges[j].pack(message2,&outputPosition);
		}
	}
	Message aMessage(message2,(count/KMER_U64_ARRAY_SIZE)*(1+4*KMER_U64_ARRAY_SIZE),source,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,m_rank);
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
	Message aMessage(message2,outputPosition,source,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY,m_rank);
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
	if((*m_numberOfRanksDoneSeeding)==m_size){
		(*m_numberOfRanksDoneSeeding)=0;
		(*m_master_mode)=RAY_MASTER_MODE_DO_NOTHING;
		for(int i=0;i<m_size;i++){
			Message aMessage(NULL,0,i,RAY_MPI_TAG_REQUEST_SEED_LENGTHS,m_rank);
			m_outbox->push_back(aMessage);
		}
	}
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_SEED_LENGTHS(Message*message){
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
	
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS(Message*message){
	(*m_numberOfRanksDoneSeeding)++;

	if((*m_numberOfRanksDoneSeeding)==m_size){
		(*m_numberOfRanksDoneSeeding)=0;
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_DETECTION;
		m_seedingData->writeSeedStatistics();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER(Message*message){

	m_router->activateRelayChecker();

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_I_GO_NOW(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS(Message*message){
	int source=message->getSource();
	uint64_t*dummy=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(dummy,5,source,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY(Message*message){
	(*m_ranksDoneAttachingReads)++;
	if((*m_ranksDoneAttachingReads)==m_size){
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
			cout<<"Rank="<<m_rank<<" "<<vertex.idToWord(*m_wordSize,m_parameters->getColorSpaceMode())<<" does not exist."<<endl;
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
	Message aMessage(message2,count*5,source,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY,m_rank);
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
	if((m_ed->m_EXTENSION_numberOfRanksDone)==m_size){
		cout<<"Rank "<<m_parameters->getRank()<<" starting fusions"<<endl;
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_FUSIONS;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_EXTENSION(Message*message){
	(m_ed->m_EXTENSION_initiated)=false;
	(*m_last_value)=-1;
	m_verticesExtractor->showBuffers();

	if(m_parameters->hasOption("-show-libraries")){
		cout<<"Libs info"<<endl;
		for(int i=0;i<m_parameters->getNumberOfLibraries();i++){
			for(int j=0;j<m_parameters->getLibraryPeaks(i);j++){
				cout<<"Lib "<<i<<" Peak "<<j<<" "<<m_parameters->getLibraryAverageLength(i,j)<<" "<<m_parameters->getLibraryStandardDeviation(i,j)<<endl;
			}
		}
	}
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

	Message aMessage(message2,1,source,RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY,m_rank);
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

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_EXTENSION_DATA_REPLY,m_rank);
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

		if(node==NULL){
			continue;
		}

		int coverage=node->getCoverage(&vertex);

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
	Message aMessage(message2,count,message->getSource(),RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY,m_rank);
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
	Message aMessage(message2,bufferPosition,source,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY,m_rank);
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

	Message aMessage(message2,pos,source,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY,m_rank);
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
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY,m_rank);
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

		Vertex copy=*node;
		assert(copy.getCoverage(&vertex)>=1);
		#endif

		uint64_t wave=incoming[pos++];
		
		#ifdef ASSERT
		if(getRankFromPathUniqueId(wave)>=m_size){
			cout<<"Invalid rank: "<<getRankFromPathUniqueId(wave)<<" maximum is "<<m_size-1<<endl;
		}
		assert(getRankFromPathUniqueId(wave)<m_size);
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
	(m_seedingData->m_SEEDING_i)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REVERSE(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE(Message*message){
	(m_ed->m_EXTENSION_currentRankIsDone)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_START_FUSION(Message*message){
	(m_seedingData->m_SEEDING_i)=0;
	m_fusionData->initialise();
}

void MessageProcessor::call_RAY_MPI_TAG_FUSION_DONE(Message*message){
	
	uint64_t*incoming=message->getBuffer();
	bool reductionOccured=incoming[0];

	if(reductionOccured){
		//cout<<"Reduction occured from RAY_MPI_TAG_FUSION_DONE!"<<endl;
		(*m_nextReductionOccured)=true;
	}

	m_fusionData->m_FUSION_numberOfRanksDone++;
	if(m_fusionData->m_FUSION_numberOfRanksDone==m_size && !(*m_isFinalFusion)){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS;
	}
}
/*
 */
void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE(Message*message){
	int source=message->getSource();
	uint64_t*incoming=(uint64_t*)message->getBuffer();
	int count=message->getCount();
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*sizeof(uint64_t));

	int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE);

	for(int i=0;i<count;i+=elementsPerQuery){
		int pos=i;
		Kmer vertex;
		vertex.unpack(incoming,&pos);

		#ifdef ASSERT
		Vertex*node=m_subgraph->find(&vertex);
		if(node==NULL){
			cout<<"Source="<<message->getSource()<<" Destination="<<m_rank<<" "<<vertex.idToWord(*m_wordSize,m_parameters->getColorSpaceMode())<<" does not exist, aborting"<<endl;
			cout.flush();
		}
		assert(node!=NULL);

		int coverage=node->getCoverage(&vertex);
		assert(coverage >= 1);
		#endif

		vector<Direction> paths=m_subgraph->getDirections(&vertex);

		message2[i]=paths.size();
	}

	Message aMessage(message2,count,source,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY,m_rank);
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

			#ifdef ASSERT
			assert(m_fusionData->m_FUSION_identifier_map[id] < (int) m_ed->m_EXTENSION_contigs.size());
			#endif

			length=(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size();
		}

		message2[i]=length;
	}

	Message aMessage(message2,count,source,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY,m_rank);
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

	int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION);

	for(int i=0;i<count;i+= elementsPerQuery){
		Kmer vertex;
		int pos=i;
		vertex.unpack(incoming,&pos);
		Vertex*node=m_subgraph->find(&vertex);
		int coverage=1;
		vector<Direction> paths;
		uint8_t edges=0;
		if(node!=NULL){
			paths=m_subgraph->getDirections(&vertex);
			coverage=node->getCoverage(&vertex);
			edges=node->getEdges(&vertex);
		}
		message2[i+0]=coverage;
		message2[i+1]=(paths.size()==1);
		if(paths.size()==1){
			message2[i+2]=paths[0].getWave();
			message2[i+3]=paths[0].getProgression();
		}
		message2[i+4]=edges;
	}

	Message aMessage(message2,count,source,RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY,m_rank);
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

Receives a k-mer and a first index, and returns a message containing 
the k-mer, the new first index, and a list of path identifiers in the graph.

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
		if(getRankFromPathUniqueId(pathId)>=m_size){
			cout<<"Invalid rank: "<<getRankFromPathUniqueId(pathId)<<" maximum is "<<m_size-1<<" Index: "<<firstPathId<<" Total: "<<paths.size()<<" Progression: "<<progression<<endl;
		}
		assert(getRankFromPathUniqueId(pathId)<m_size);
		#endif

		message2[outputPosition++]=pathId;
		message2[outputPosition++]=progression;
		firstPathId++;
	}

	message2[origin]=firstPathId;

	#ifdef ASSERT
	assert(outputPosition<=availableElements);
	assert(source<m_size);
	#endif

	if(firstPathId==(int)paths.size()){
		Message aMessage(message2,outputPosition,source,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END,m_rank);
		m_outbox->push_back(aMessage);
	}else{
		Message aMessage(message2,outputPosition,source,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY,m_rank);
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
		assert(getRankFromPathUniqueId(pathId)<m_size);
		#endif
		int position=incoming[i+1];
		m_fusionData->m_FUSION_receivedPath.constructor(pathId,position,lower);
		m_fusionData->m_Machine_getPaths_result.push_back(m_fusionData->m_FUSION_receivedPath);
	}

	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	int outputPosition=0;
	vertex.pack(message2,&outputPosition);
	message2[outputPosition++]=incoming[origin];
	Message aMessage(message2,outputPosition,message->getSource(),RAY_MPI_TAG_ASK_VERTEX_PATHS,m_rank);
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
		assert(getRankFromPathUniqueId(pathId)<m_size);
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
	int count=message->getCount();
	int elementsPerItem=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATH);
	int pos=0;
	int outputPosition=0;
	uint64_t*message2=(uint64_t*)m_outboxAllocator->allocate(count*sizeof(uint64_t));

	#ifdef ASSERT
	assert(count % elementsPerItem == 0);
	#endif

	for(int i=0;i<count;i+=elementsPerItem){
		Kmer kmer;
		kmer.unpack(incoming,&pos);

		#ifdef ASSERT
		Vertex*node=m_subgraph->find(&kmer);
		assert(node!=NULL);
		#endif

		vector<Direction> paths=m_subgraph->getDirections(&kmer);
		int i=incoming[pos++];

		/* increment because there is padding */
		pos++;

		Direction d=paths[i];
		kmer.pack(message2,&outputPosition);
		message2[outputPosition++]=d.getWave();

		#ifdef ASSERT
		assert(getRankFromPathUniqueId(d.getWave())<m_size);
		#endif

		message2[outputPosition++]=d.getProgression();
	}

	Message aMessage(message2,outputPosition,source,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY,m_rank);
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
	uint64_t pathId=incoming[pos];

	#ifdef ASSERT
	assert(getRankFromPathUniqueId(pathId)<m_size);
	#endif

	int position=incoming[pos];
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
	Message aMessage(message2,count,source,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY,m_rank);
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
	Message aMessage(message2,3,source,RAY_MPI_TAG_GET_PAIRED_READ_REPLY,m_rank);
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

	/* never append reverse complement */
	appendReverseComplement=false;

	// clearing old data too!.
	m_fusionData->m_FINISH_pathLengths.clear();

	#ifdef ASSERT
	assert(m_ed->m_EXTENSION_contigs.size()==m_ed->m_EXTENSION_identifiers.size());
	#endif

	// clear graph
	GridTableIterator iterator;
	iterator.constructor(m_subgraph,*m_wordSize,m_parameters);

	#ifdef ASSERT
	uint64_t cleared=0;
	#endif

	while(iterator.hasNext()){
		iterator.next();
		Kmer key=*(iterator.getKey());
		m_subgraph->clearDirections(&key);

		#ifdef ASSERT
		cleared++;

		Vertex*node=m_subgraph->find(&key);
		assert(node->m_directions == NULL);

		#endif
	}

	#ifdef ASSERT
	if(cleared != m_subgraph->size()){
		cout<<"Hilarious error: cleared, expected: "<<m_subgraph->size()<<", actual: "<<cleared<<endl;
	}
	assert(cleared == m_subgraph->size());
	#endif

	m_directionsAllocator->clear();

	cout<<"Rank "<<m_parameters->getRank()<<" adding "<<m_fusionData->m_FINISH_newFusions.size()<<" new fusions"<<endl;

	// add the FINISHING bits
	for(int i=0;i<(int)m_fusionData->m_FINISH_newFusions.size();i++){
		(m_ed->m_EXTENSION_contigs).push_back((m_fusionData->m_FINISH_newFusions)[i]);
	}

	m_fusionData->m_FINISH_newFusions.clear();

	vector<vector<Kmer> > fusions;
	for(int i=0;i<(int)(m_ed->m_EXTENSION_contigs).size();i++){
		bool eliminated=false;

		/* it is not a new one */
		if(i<(int)m_ed->m_EXTENSION_identifiers.size()){
			uint64_t id=(m_ed->m_EXTENSION_identifiers)[i];
			if(m_fusionData->m_FUSION_eliminated.count(id)>0)
				eliminated=true;
		}

		if(!eliminated){
			fusions.push_back((m_ed->m_EXTENSION_contigs)[i]);
			if(!appendReverseComplement)
				continue;

			vector<Kmer> rc;
			for(int j=(m_ed->m_EXTENSION_contigs)[i].size()-1;j>=0;j--){
				rc.push_back(m_ed->m_EXTENSION_contigs[i][j].complementVertex(*m_wordSize,
					m_parameters->getColorSpaceMode()));
			}
			fusions.push_back(rc);
		}
	}

	(m_ed->m_EXTENSION_identifiers).clear();
	m_fusionData->m_FUSION_eliminated.clear();
	for(int i=0;i<(int)fusions.size();i++){
		uint64_t id=getPathUniqueId(m_rank,i);
		#ifdef ASSERT
		assert(m_rank<m_size);
		assert(m_rank>=0);
		assert(m_size>=1);
		assert((int)getRankFromPathUniqueId(id)<m_size);
		#endif
		(m_ed->m_EXTENSION_identifiers).push_back(id);
	}

	m_fusionData->m_FUSION_identifier_map.clear();

	#ifdef ASSERT
	assert(m_fusionData->m_FUSION_identifier_map.size()==0);
	#endif

	for(int i=0;i<(int)(m_ed->m_EXTENSION_identifiers).size();i++){
		uint64_t id=(m_ed->m_EXTENSION_identifiers)[i];
		m_fusionData->m_FUSION_identifier_map[id]=i;
	}

	(m_ed->m_EXTENSION_contigs).clear();
	(m_ed->m_EXTENSION_contigs)=fusions;

	#ifdef ASSERT
	assert(m_ed->m_EXTENSION_contigs.size()==m_ed->m_EXTENSION_identifiers.size());
	assert(m_fusionData->m_FUSION_identifier_map.size()==m_ed->m_EXTENSION_contigs.size());
	#endif

	cout<<"Rank "<<m_parameters->getRank()<<" cleared, "<<m_ed->m_EXTENSION_contigs.size()<<" paths"<<endl;

	Message aMessage(NULL,0,source,RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY(Message*message){
	(*m_CLEAR_n)++;
}


void MessageProcessor::call_RAY_MPI_TAG_FINISH_FUSIONS(Message*message){
	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
	m_fusionData->m_FUSION_first_done=false;
	m_fusionData->m_Machine_getPaths_INITIALIZED=false;
	m_fusionData->m_Machine_getPaths_DONE=false;
}

void MessageProcessor::call_RAY_MPI_TAG_FINISH_FUSIONS_FINISHED(Message*message){
	uint64_t*incoming=message->getBuffer();

	bool reductionOccured=incoming[0];

	if(reductionOccured){
		(*m_nextReductionOccured)=true;
	}

	(*m_FINISH_n)++;
}

void MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS(Message*message){
	m_fusionData->readyBuffers();
	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY(Message*message){
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY,m_rank);
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
	assert(rank<m_size);
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
	int count=message->getCount();

	int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_PATH_VERTEX);
	uint64_t*messageBytes=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));

	for(int i=0;i<count;i+=elementsPerQuery){
		uint64_t id=incoming[i];
		int position=incoming[i+1];

		#ifdef ASSERT
		assert(m_fusionData->m_FUSION_identifier_map.count(id)>0);
		if(position>=(int)(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size()){
			cout<<"Pos="<<position<<" Length="<<(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size()<<endl;
		}
		assert(position<(int)(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]].size());
		#endif

		Kmer a=(m_ed->m_EXTENSION_contigs)[m_fusionData->m_FUSION_identifier_map[id]][position];
		int pos=i;
		a.pack(messageBytes,&pos);
	}

	Message aMessage(messageBytes,count,source,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY,m_rank);
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
	m_ed->m_EXTENSION_initiated=false;
	m_ed->m_EXTENSION_currentPosition=((uint64_t*)message->getBuffer())[0];
}

void MessageProcessor::call_RAY_MPI_TAG_WRITE_AMOS_REPLY(Message*message){
	m_ed->m_EXTENSION_currentRankIsDone=true;
	m_ed->m_EXTENSION_currentPosition=((uint64_t*)message->getBuffer())[0];
}

void MessageProcessor::call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION(Message*message){
	/* write the Seeds checkpoint */
	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("Seeds")){
		ofstream f(m_parameters->getCheckpointFile("Seeds").c_str());
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint Seeds"<<endl;
		int count=m_seedingData->m_SEEDING_seeds.size();
		f.write((char*)&count,sizeof(int));
		for(int i=0;i<(int)m_seedingData->m_SEEDING_seeds.size();i++){
			int length=m_seedingData->m_SEEDING_seeds[i].size();
			f.write((char*)&length,sizeof(int));
			for(int j=0;j<(int)m_seedingData->m_SEEDING_seeds[i].size();j++){
				m_seedingData->m_SEEDING_seeds[i].at(j)->write(&f);
			}
		}
		f.close();
	}

	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
	m_ed->m_EXTENSION_reads_requested=false;
}

void MessageProcessor::call_RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE(Message*message){
	(*m_numberOfRanksDoneDetectingDistances)++;
	if((*m_numberOfRanksDoneDetectingDistances)==m_size){
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

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES(Message*message){
	m_library->allocateBuffers();
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED(Message*message){
	(*m_numberOfRanksDoneSendingDistances)++;
	if((*m_numberOfRanksDoneSendingDistances)==m_size){
		(*m_master_mode)=RAY_MASTER_MODE_START_UPDATING_DISTANCES;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION(Message*message){
	void*buffer=message->getBuffer();
	uint64_t*incoming=(uint64_t*)buffer;
	int count=message->getCount();

	for(int i=0;i<count;i+=3){
		/* the MASTER_RANK already has this information */
		if(m_rank==MASTER_RANK)
			break;

		int library=incoming[i+0];

		#ifdef ASSERT
		assert(m_parameters->isAutomatic(library));
		#endif

		int average=incoming[i+1];
		int standardDeviation=incoming[i+2];

		m_parameters->addLibraryData(library,average,standardDeviation);
	}

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_KMER_ACADEMY_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	uint64_t*incoming=(uint64_t*)buffer;

	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		/** the Kmer l is the lower between l and revcomp(l) */
		Kmer l;
		int pos=i;
		l.unpack(incoming,&pos);

		if((*m_last_value)!=(int)m_subgraph->getKmerAcademy()->size() && (int)m_subgraph->getKmerAcademy()->size()%100000==0){
			(*m_last_value)=m_subgraph->getKmerAcademy()->size();
			printf("Rank %i has %i k-mers\n",m_rank,(int)m_subgraph->getKmerAcademy()->size());
			fflush(stdout);

			if(m_parameters->showMemoryUsage()){
				showMemoryUsage(m_rank);
			}

			m_subgraph->getKmerAcademy()->printStatistics();
		}

		if(!m_bloomFilter.hasValue(&l)){
/*
			cout<<"inserting in Bloom filter: "<<endl;
			l.print();
*/

			m_bloomFilter.insertValue(&l);
			continue;
		}

		/** the Kmer object is in the BloomFilter */

		KmerCandidate*tmp=m_subgraph->insertInAcademy(&l);
		#ifdef ASSERT
		assert(tmp!=NULL);
		#endif

		/** start at 1 since this step was avoided by the BloomFilter */
		if(m_subgraph->insertedInAcademy()){
			tmp->m_count=1;
		}

		COVERAGE_TYPE oldValue=tmp->m_count;
		if(tmp->m_lowerKey==l)
			tmp->m_count++;
		if(tmp->m_count<oldValue)
			tmp->m_count=oldValue; /* avoids overflow */
	}
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_KMER_ACADEMY_DATA_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_KMER_ACADEMY_DATA_REPLY(Message*message){
}

void MessageProcessor::call_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED(Message*message){
	m_kmerAcademyFinishedRanks++;
	if(m_kmerAcademyFinishedRanks==m_parameters->getSize()){
		(*m_master_mode)=RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY(Message*message){
	(*m_numberOfRanksWithCoverageData)++;
	if((*m_numberOfRanksWithCoverageData)==m_size){
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING;
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
	Message aMessage(messageBytes,toAllocate/sizeof(uint64_t),source,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY,m_rank);
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
	tmp.getSeq(seedExtender->m_receivedString,m_parameters->getColorSpaceMode(),false);
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
	uint64_t*messageContent=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int outputPosition=0;
	int origin=outputPosition;
	outputPosition++;
	int count=0;
	while(position<length
	 && (outputPosition+KMER_U64_ARRAY_SIZE)<(int)(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t))){
		m_ed->m_EXTENSION_contigs[index][position++].pack(messageContent,&outputPosition);
		count++;
	}
	messageContent[origin]=count;
	Message aMessage(messageContent,
		m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_CONTIG_CHUNK),
		message->getSource(),RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY,
		m_parameters->getRank());
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_START_SCAFFOLDER(Message*message){}

void MessageProcessor::setScaffolder(Scaffolder*a){
	m_scaffolder=a;
}

void MessageProcessor::constructor(
MessageRouter*router,
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

	m_router=router;

	this->m_sequencesLoader=sequencesLoader;
	this->m_verticesExtractor=m_verticesExtractor;
	this->m_ed=ed;
	this->m_numberOfRanksDoneDetectingDistances=m_numberOfRanksDoneDetectingDistances;
	this->m_numberOfRanksDoneSendingDistances=m_numberOfRanksDoneSendingDistances;
	this->m_parameters=parameters;
	this->m_library=m_library;
	this->m_subgraph=m_subgraph;
	this->m_outboxAllocator=m_outboxAllocator;
	m_rank=rank;
	this->m_numberOfMachinesDoneSendingEdges=m_numberOfMachinesDoneSendingEdges;
	this->m_fusionData=m_fusionData;
	this->m_wordSize=m_wordSize;
	this->m_myReads=m_myReads;
	m_size=size;
	this->m_inboxAllocator=m_inboxAllocator;
	this->m_si=m_si;
	this->m_persistentAllocator=m_persistentAllocator;
	this->m_identifiers=m_identifiers;
	this->m_mode_sendDistribution=m_mode_sendDistribution;
	this->m_alive=m_alive;
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
	m_kmerAcademyFinishedRanks=0;
}

MessageProcessor::MessageProcessor(){
	m_last=time(NULL);
	m_consumed=0;
	m_sentinelValue=0;
	m_sentinelValue--;// overflow it in an obvious manner
}

void MessageProcessor::assignHandlers(RayScriptEngine*scriptEngine){
	
	vector<Tag> tags;
	vector<MessageProcessorHandler> handlers;

	scriptEngine->configureMessageHandlers(&tags,&handlers);

	for(int i=0;i<(int)tags.size();i++){
		m_methods[tags[i]]=handlers[i];
	}
}

void MessageProcessor::setVirtualCommunicator(VirtualCommunicator*a){
	m_virtualCommunicator=a;
}

void MessageProcessor::call_RAY_MPI_TAG_COUNT_FILE_ENTRIES(Message*message){}

void MessageProcessor::call_RAY_MPI_TAG_COUNT_FILE_ENTRIES_REPLY(Message*message){}
void MessageProcessor::call_RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS(Message*message){}
void MessageProcessor::call_RAY_MPI_TAG_REQUEST_FILE_ENTRY_COUNTS_REPLY(Message*message){}
void MessageProcessor::call_RAY_MPI_TAG_FILE_ENTRY_COUNT(Message*message){}
void MessageProcessor::call_RAY_MPI_TAG_FILE_ENTRY_COUNT_REPLY(Message*message){}

/* the switch man do the accounting for ready ranks */
void MessageProcessor::call_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL(Message*message){
	m_switchMan->closeSlaveMode(message->getSource());
}

void MessageProcessor::setSwitchMan(SwitchMan*a){
	m_switchMan=a;
}

void MessageProcessor::call_RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS(Message*message){}
void MessageProcessor::call_RAY_MPI_TAG_TEST_NETWORK_REPLY(Message*message){}
void MessageProcessor::call_RAY_MPI_TAG_TEST_NETWORK_REPLY_REPLY(Message*message){}
