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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

//#define GUILLIMIN_BUG

#include <application_core/constants.h>
#include <string.h>
#include <core/OperatingSystem.h>
#include <assert.h>
#include <plugin_SequencesLoader/Read.h>
#include <plugin_MessageProcessor/MessageProcessor.h>
#include <structures/StaticVector.h>
#include <application_core/common_functions.h>
#include <plugin_SequencesIndexer/ReadAnnotation.h>
#include <structures/SplayTree.h>
#include <plugin_SeedExtender/Direction.h>
#include <structures/SplayNode.h>
#include <structures/SplayTreeIterator.h>
#include <plugin_FusionData/FusionData.h>
#include <application_core/Parameters.h>
#include <core/ComputeCore.h>

__CreatePlugin(MessageProcessor);

 /**/
 /**/
 /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_LOAD_SEQUENCES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_CONTIG_INFO); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SCAFFOLDING_LINKS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_READ_MARKERS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_READ_MATE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_READS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SET_WORD_SIZE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_READS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_INFO); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_READS_FROM_LIST); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_START_INDEXING_SEQUENCES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEQUENCES_READY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTICES_DATA); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_PURGE_NULL_EDGES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_VERTICES_DISTRIBUTED); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_OUT_EDGES_DATA_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_OUT_EDGES_DATA); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_IN_EDGES_DATA_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_IN_EDGES_DATA); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_TEST_NETWORK_MESSAGE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_COVERAGE_DATA); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_COVERAGE_END); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEND_COVERAGE_VALUES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_READY_TO_SEED); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_START_SEEDING); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_COVERAGE_AND_MARK); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_EDGES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEEDING_IS_OVER); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_SEED_LENGTHS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEND_SEED_LENGTHS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_IS_DONE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_EXTENSION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_EXTENSION_DATA); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA_END); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ATTACH_SEQUENCE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_LENGTH); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASSEMBLE_WAVES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_START_FUSION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_FUSION_DONE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_LENGTH); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATH); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_HAS_PAIRED_READ); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PAIRED_READ); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PAIRED_READ_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_CLEAR_DIRECTIONS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_FINISH_FUSIONS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_START); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ELIMINATE_PATH); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_VERTEX); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_WRITE_AMOS); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_WRITE_AMOS_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_LIBRARY_DISTANCE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_READ_SEQUENCE); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_I_FINISHED_SCAFFOLDING); /**/
__CreateMessageTagAdapter(MessageProcessor,RAY_MPI_TAG_GET_CONTIG_CHUNK); /**/
 /**/


void MessageProcessor::call_RAY_MPI_TAG_LOAD_SEQUENCES(Message*message){
	uint32_t*incoming=(uint32_t*)message->getBuffer();
	for(int i=0;i<(int)incoming[0];i++){
		if(m_parameters->hasOption("-debug-partitioner"))
			cout<<"Rank "<<m_parameters->getRank()<<" RAY_MPI_TAG_LOAD_SEQUENCES File "<<i<<" "<<incoming[i+1]<<endl;
		m_parameters->setNumberOfSequences(i,incoming[1+i]);
	}
}

void MessageProcessor::call_RAY_MPI_TAG_CONTIG_INFO(Message*message){
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
	m_scaffolder->addMasterContig(incoming[0],incoming[1]);
	
	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(outgoingMessage,message->getCount(),
		message->getSource(),RAY_MPI_TAG_CONTIG_INFO_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_SCAFFOLDING_LINKS(Message*message){
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();

	int position=0;

	PathHandle leftContig=incoming[position++];
	Strand leftStrand=incoming[position++];
	PathHandle rightContig=incoming[position++];
	Strand rightStrand=incoming[position++];
	int number=incoming[position++];
	int average=incoming[position++];
	int standardDeviation=incoming[position++];

	if(rightContig<leftContig){
		PathHandle t=leftContig;
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

	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(outgoingMessage,message->getCount(),
		message->getSource(),RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_READ_MARKERS(Message*message){
	int count=message->getCount();
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
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

	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int j=0;
	MessageUnit*buffer=(MessageUnit*)message->getBuffer();

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

	MessageUnit*buffer=(MessageUnit*)message->getBuffer();
	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);


	#ifdef GUILLIMIN_BUG
	bool printBug=m_rank==message->getSource();

	if(printBug){
		cout<<endl;
		cout<<"MessageProcessor Receiving RAY_MPI_TAG_REQUEST_VERTEX_READS from "<<message->getSource()<<endl;
	}
	#endif

	int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_REQUEST_VERTEX_READS);

	int offset=0;

	int OFFSET_POINTER=offset++;
	int OFFSET_RANK=offset++;
	int OFFSET_READ_INDEX=offset++;
	int OFFSET_POSITION_ON_STRAND=offset++;
	int OFFSET_STRAND=offset++;

	#ifdef ASSERT
	assert(OFFSET_STRAND < period);
	assert(message->getCount()%period ==0);
	#endif

	#ifdef GUILLIMIN_BUG
	if(printBug){
		cout<<"Count: "<<message->getCount()<<" period "<<period<<endl;
		cout<<"multiplexed messages: "<<message->getCount()/period<<endl;
	}
	#endif

	#ifdef ASSERT
	assert(period>=5);
	#endif

	// this is a multiplexed message.
	for(int i=0;i<message->getCount();i+=period){
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(buffer,&bufferPosition);

		// another fancy trick to receive a pointer.
		ReadAnnotation*ptr;

		uint64_t integerValue=buffer[bufferPosition++];
		unpack_pointer((void**)&ptr, integerValue);

		#ifdef ASSERT
		// check the padding
		
		int start=KMER_U64_ARRAY_SIZE+1;

		for(int iterator=start;iterator<period;iterator++){
			if(buffer[i+iterator]!=0){
				cout<<"Error: message corruption detected, the padding changed !"<<endl;
			}
			assert(buffer[i+iterator]==0);
		}
		#endif

		#ifdef GUILLIMIN_BUG
		if(printBug){
			cout<<"Pointer: "<<ptr<<" integerValue= "<<integerValue<<endl;

			for(int k=0;k<period;k++){
				cout<<" "<<k<<" -> "<<buffer[i+k];
			}
			cout<<endl;
		}
		#endif

		Kmer complement=m_parameters->_complementVertex(&vertex);
		bool isLower=vertex<complement;

		/* prime the thing */
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
				outgoingMessage[i+OFFSET_RANK]=rank;
				outgoingMessage[i+OFFSET_READ_INDEX]=ptr->getReadIndex();
				outgoingMessage[i+OFFSET_POSITION_ON_STRAND]=ptr->getPositionOnStrand();
				outgoingMessage[i+OFFSET_STRAND]=ptr->getStrand();

				gotOne=true;
			}

			ptr=ptr->getNext();
		}

		if(!gotOne){
			outgoingMessage[i+OFFSET_RANK]=INVALID_RANK;
		}

		// send the void*
		outgoingMessage[i+OFFSET_POINTER]=pack_pointer((void**)&ptr);

		#ifdef GUILLIMIN_BUG
		if(printBug){
			cout<<"Will send pointer "<<ptr<<" back"<<endl;
			for(int p=0;p<period;p++){
				cout<<" "<<p<<" -> "<<outgoingMessage[i+p];
			}
			cout<<endl;
		}
		#endif

	}

	#ifdef GUILLIMIN_BUG
	if(printBug){
		cout<<endl;
		cout<<"MessageProcessor Sending response RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY to "<<message->getSource()<<endl;
		cout<<"RAY_MPI_TAG_REQUEST_VERTEX_READS= "<<RAY_MPI_TAG_REQUEST_VERTEX_READS<<endl;
		cout<<"RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY= "<<RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY<<endl;
	}
	#endif

	Message aMessage(outgoingMessage,message->getCount(),message->getSource(),RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY,m_rank);
	m_outbox->push_back(aMessage);

	#ifdef GUILLIMIN_BUG
	if(printBug){
		for(int k=0;k<message->getCount();k++){
			cout<<"; "<<k<<" -> "<<outgoingMessage[k];
		}
		cout<<endl;
	}
	#endif
}

void MessageProcessor::call_RAY_MPI_TAG_SET_WORD_SIZE(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
	(*m_wordSize)=incoming[0];
}

/*
 * <- k-mer -><- pointer ->
 */
void MessageProcessor::call_RAY_MPI_TAG_VERTEX_READS(Message*message){
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
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

	int maximumToReturn=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit)/4-1;
	int processed=0;

	while(e!=NULL&&processed<maximumToReturn){
		if(e->isLower()==lower){
			processed++;
		}
		e=e->getNext();
	}

	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate((processed+1)*4*sizeof(MessageUnit));
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
	outgoingMessage[outputPosition++]=(MessageUnit)e;
	Message aMessage(outgoingMessage,outputPosition,message->getSource(),RAY_MPI_TAG_VERTEX_READS_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_VERTEX_INFO(Message*message){
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
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

	PathHandle wave=incoming[bufferPosition++];

	Rank origin=getRankFromPathUniqueId(wave);

/*
	int progression=incoming[bufferPosition++];
	// add direction in the graph
	Direction*d=(Direction*)m_directionsAllocator->allocate(sizeof(Direction));
	d->constructor(wave,progression,lower);
	m_subgraph->addDirection(&vertex,d);
*/

	m_subgraph->find(&vertex)->assemble(origin);

	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	outgoingMessage[0]=node->getCoverage(&vertex);
	outgoingMessage[1]=node->getEdges(&vertex);
	outgoingMessage[2]=n;
	int pos=5;
	int processed=0;
	int maximumToReturn=(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit)-5)/4;
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

	outgoingMessage[3]=(MessageUnit)e;
	outgoingMessage[4]=processed;
	Message aMessage(outgoingMessage,pos,message->getSource(),RAY_MPI_TAG_VERTEX_INFO_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT(Message*message){
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
	int count=message->getCount();
	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
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

/*
 * <--vertex--><--pointer--><--numberOfMates--><--mates -->
 */
void MessageProcessor::call_RAY_MPI_TAG_VERTEX_READS_FROM_LIST(Message*message){
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
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
	MessageUnit*outgoingMessage=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int processed=0;
	int pos=1;
	set<PathHandle> localIndex;
	for(int i=0;i<numberOfMates;i++){
		localIndex.insert(incoming[KMER_U64_ARRAY_SIZE+2+i]);
	}

	while(e!=NULL){
		if(e->isLower()==lower){
			PathHandle uniqueId=getPathUniqueId(e->getRank(),e->getReadIndex());
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

void MessageProcessor::call_RAY_MPI_TAG_START_INDEXING_SEQUENCES(Message*message){

	/* read the Graph checkpoint here */
	if(m_parameters->hasCheckpoint("GenomeGraph")){
		cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint GenomeGraph"<<endl;
		ifstream f(m_parameters->getCheckpointFile("GenomeGraph").c_str());
		LargeCount n=0;
		f.read((char*)&n,sizeof(LargeCount));

		for(LargeIndex i=0;i<n;i++){
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
		LargeCount theSize=m_subgraph->size();

		f.write((char*)&theSize,sizeof(LargeCount));

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
		m_switchMan->closeMasterMode();
	}
}

/*
 * receive vertices (data)
 */
void MessageProcessor::call_RAY_MPI_TAG_VERTICES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	MessageUnit*incoming=(MessageUnit*)buffer;

	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		Kmer kmerObject;
		int pos=i;
		kmerObject.unpack(incoming,&pos);

/* make sure that the payload 
 * is for this process and not another one...
 */
		#ifdef ASSERT
		Rank rankToFlush=kmerObject.hash_function_1()%m_parameters->getSize();
		assert(rankToFlush==m_rank);
		#endif

		//bool isTheLowerKmer=false;
		Kmer lowerKmer=kmerObject;

/* 
 * TODO: remove call to reverseComplement, this if should never be 
 * picked up because only the lowest k-mers are sent
 * I am not sure it would work but if it does that would reduces 
 * the number of sent messages 
 * 
 * *** Anyway, the message was delivered anyway already.
 */
		Kmer reverseComplement=kmerObject.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

/*
 * This assert can only fail if the user modified
 * the source code to enable odd k-mer length
 * values
 */
		#ifdef ASSERT
		assert(reverseComplement!=kmerObject);
		#endif

		if(reverseComplement < lowerKmer)
			lowerKmer=reverseComplement;
/*
 * If the Bloom filter has exactly 0 bits,
 * this means that it is disabled.
 * The Bloom filter only contain the lower k-mers.
 */
		if(m_bloomBits>0 && !m_bloomFilter.hasValue(&lowerKmer)){
/*
			cout<<"inserting in Bloom filter: "<<endl;
			kmerObject.print();
*/

			m_bloomFilter.insertValue(&lowerKmer);

			continue;
		}


		if((*m_last_value)!=(int)m_subgraph->size() && (int)m_subgraph->size()%100000==0){
			(*m_last_value)=m_subgraph->size();
			printf("Rank %i has %i vertices\n",m_rank,(int)m_subgraph->size());
			fflush(stdout);

			if(m_parameters->showMemoryUsage()){
				showMemoryUsage(m_rank);
			}
		}
		

/*
 * We have a go. We insert the k-mer in the distributed
 * de Bruijn graph.
 */
		Vertex*tmp=m_subgraph->insert(&kmerObject);

		#ifdef ASSERT
		assert(tmp!=NULL);
		#endif

/*
 * Initialize the k-mer coverage 
 * It starts at 0 if the Bloom filter
 * is disabled, 1 otherwise.
 */
		if(m_subgraph->inserted()){
			tmp->constructor(); 
		
			CoverageDepth startingValue=0;

/*
 * If the k-mers must go in the Bloom filter first,
 * their coverage must start at 1 instead of 0.
 */
			if(m_bloomBits>0)
				startingValue++;

			tmp->setCoverage(&kmerObject,startingValue);
		}

/*
 * We only increase the k-mer coverage of the pair
 * when we see the lower k-mer of the pair. Otherwise,
 * the coverage will be double what it should be.
 * This logic is implemented in the class Vertex.
 */
		CoverageDepth oldCoverage=tmp->getCoverage(&kmerObject);
		CoverageDepth newCoverage=oldCoverage+1;
		tmp->setCoverage(&kmerObject,newCoverage);
	}

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_VERTICES_DATA_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_PURGE_NULL_EDGES(Message*message){


	#ifdef ASSERT
	GridTableIterator iterator;
	iterator.constructor(m_subgraph,*m_wordSize,m_parameters);

	LargeCount n=0;

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
	
	#if 0
	m_subgraph->printStatistics();
	#endif

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_rank);
		fflush(stdout);
	}

	*m_mode=RAY_SLAVE_MODE_PURGE_NULL_EDGES;
}

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
	MessageUnit*incoming=(MessageUnit*)buffer;
	int length=count;

	int period=2*KMER_U64_ARRAY_SIZE;

	for(int i=0;i<(int)length;i+=period){
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

	// with 4 000 000 kmers, the ratio is objects/bits is 16.
	// with 0.000574
	m_bloomBits=__BLOOM_DEFAULT_BITS; 

	if(m_parameters->hasConfigurationOption("-bloom-filter-bits",1))
		m_bloomBits=m_parameters->getConfigurationInteger("-bloom-filter-bits",0);

	if(m_bloomBits>0){
		m_bloomFilter.constructor(m_bloomBits);
		cout<<"Rank "<<m_rank<<" created its Bloom filter"<<endl;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA_REPLY(Message*message){
	m_verticesExtractor->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_IN_EDGES_DATA(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int length=count;

/**
 * We process each edge.
 * For each block, there are KMER_U64_ARRAY_SIZE elements for the vertex1 and
 * KMER_U64_ARRAY_SIZE elements for vertex2.
 *
 * It is like this:
 *
 * | vertex1 | vertex2 | vertex1 |vertex 2 | ...
 */
	int period=2*KMER_U64_ARRAY_SIZE;

	for(int i=0;i<(int)length;i+=period){
		int bufferPosition=i;
		Kmer prefix;
		prefix.unpack(incoming,&bufferPosition);
		Kmer suffix;
		suffix.unpack(incoming,&bufferPosition);

		Vertex*node=m_subgraph->find(&suffix);

/*
 * The suffix is not in the graph.
 */
		if(node==NULL){
			continue;
		}

		node->addIngoingEdge(&suffix,&prefix,(*m_wordSize));

/*
 * Make sure that the edge was added.
 */
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
		m_switchMan->closeMasterMode();
	}
}

/* we reply with an empty message */
void MessageProcessor::call_RAY_MPI_TAG_TEST_NETWORK_MESSAGE(Message*message){
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION(Message*message){

	uint64_t kmersInBloomFilter=0;

	if(m_bloomBits>0){
		uint64_t setBits=m_bloomFilter.getNumberOfSetBits();
		uint64_t bits=m_bloomFilter.getNumberOfBits();
	
		#ifdef ASSERT
		assert(bits>0);
		assert(bits==m_bloomBits);
		#endif

		double ratio=100.0*setBits/bits;

		cout<<"Rank "<<m_rank<<" number of set bits in the Bloom filter: ";
		cout<<"[ "<<setBits<<" / "<<bits<<" ] ("<<ratio<<"%)";

		if(ratio >= 50.0)
			cout<<" Warning: the oracle is half full."<<endl;

		cout<<endl;

		kmersInBloomFilter=m_bloomFilter.getNumberOfInsertions();
		kmersInBloomFilter*=2; // pairs of k-mers

		m_bloomFilter.destructor();
		cout<<"Rank "<<m_rank<<" destroyed its Bloom filter"<<endl;
	
	}

	// complete incremental resizing, if any
	m_subgraph->completeResizing();

	uint64_t kmersInGraph=m_subgraph->size();
	cout<<"Rank "<<m_rank<<" has "<<kmersInGraph<<" k-mers (completed)"<<endl;

/*
 * The number of k-mers in the Bloom filter will be greater than
 * the number of k-mers in the graph only if the Bloom filter is
 * enabled (its number of bits is greater than 0) and if its false
 * positive rate is acceptable.
 * Otherwise, the counts are meaningless.
 * For instance, if -bloom-filter-bits is set to 30, then the false
 * positive rate will be ridiculous.
 */
	if(kmersInBloomFilter >= kmersInGraph){
		uint64_t filteredKmers=kmersInBloomFilter-kmersInGraph;
		cout<<"[BloomFilter] Rank "<<m_rank<<": k-mers sampled -> "<<kmersInBloomFilter;
		cout<<", k-mers dropped -> "<<filteredKmers<<" ("<<100.0*filteredKmers/kmersInBloomFilter<<"%)";
		cout<<", k-mers accepted -> "<<kmersInGraph;
		cout<<" ("<<100.0*kmersInGraph/kmersInBloomFilter<<"%)";
		cout<<endl;
	}

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_rank);
		fflush(stdout);
	}

	(*m_mode_send_coverage_iterator)=0;
	(*m_mode_sendDistribution)=true;

}

void MessageProcessor::call_RAY_MPI_TAG_COVERAGE_DATA(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();

	for(int i=0;i<count;i+=2){
		CoverageDepth coverage=incoming[i+0x0];
		LargeCount count=incoming[i+1];
		(*m_coverageDistribution)[coverage]+=count;
	}

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_COVERAGE_DATA_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_COVERAGE_END(Message*message){
	(*m_numberOfMachinesDoneSendingCoverage)++;
	if((*m_numberOfMachinesDoneSendingCoverage)==m_size){
		m_switchMan->closeMasterMode();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_COVERAGE_VALUES(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
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
		LargeCount n=0;
		f.read((char*)&n,sizeof(LargeCount));
		for(LargeIndex i=0;i<n;i++){
			m_myReads->at(i)->readOffsets(&f);
		}
		f.close();

		cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint OptimalMarkers"<<endl;
		ifstream f2(m_parameters->getCheckpointFile("OptimalMarkers").c_str());

		n=0;
		f2.read((char*)&n,sizeof(LargeCount));
		LargeCount loaded=0;
		for(LargeIndex i=0;i<n;i++){
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
		LargeCount count=m_myReads->size();
		f.write((char*)&count,sizeof(LargeCount));
		for(int i=0;i<(int)m_myReads->size();i++){
			m_myReads->at(i)->writeOffsets(&f);
		}
		f.close();
	
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint OptimalMarkers"<<endl;
		ofstream f2(m_parameters->getCheckpointFile("OptimalMarkers").c_str());

		GridTableIterator iterator;
		iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);
	
		count=m_subgraph->size();
		f2.write((char*)&count,sizeof(LargeCount));

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
	MessageUnit*incoming=(MessageUnit*)buffer;
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(2*sizeof(MessageUnit));
	Kmer vertex;
	int bufferPosition=0;
	vertex.unpack(incoming,&bufferPosition);
	Vertex*node=m_subgraph->find(&vertex);

	#ifdef ASSERT
	assert(node!=NULL);
	#endif

	CoverageDepth coverage=node->getCoverage(&vertex);
	message2[0]=coverage;
	message2[1]=node->getEdges(&vertex);
	Kmer rc=m_parameters->_complementVertex(&vertex);
	bool lower=vertex<rc;
	PathHandle wave=incoming[1];
	int progression=incoming[2];
	// mark direction in the graph
	Direction*e=(Direction*)m_directionsAllocator->allocate(sizeof(Direction));
	e->constructor(wave,progression,lower);
	m_subgraph->addDirection(&vertex,e);

	Message aMessage(message2,2,message->getSource(),RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE(Message*message){
	void*buffer=message->getBuffer();
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(count*sizeof(MessageUnit));

	for(int i=0;i<count;i+=KMER_U64_ARRAY_SIZE){
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(incoming,&bufferPosition);

		//string kmerStr=vertex.idToWord(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

		Vertex*node=m_subgraph->find(&vertex);

		// if it is not there, then it has a coverage of 0
		CoverageDepth coverage=0;

		if(node!=NULL){
			coverage=node->getCoverage(&vertex);

			#ifdef ASSERT
			assert(coverage!=0);
			#endif
		}

		message2[i]=coverage;
	}

	Message aMessage(message2,count,source,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;

	#ifdef ASSERT
	#endif

	(m_seedingData->m_SEEDING_receivedVertexCoverage)=incoming[0];
	(m_seedingData->m_SEEDING_vertexCoverageReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES(Message*message){
	void*buffer=message->getBuffer();
	int source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(count*5*sizeof(MessageUnit));

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
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(count*10*sizeof(MessageUnit));
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
	MessageUnit*incoming=(MessageUnit*)buffer;

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
	MessageUnit*incoming=(MessageUnit*)buffer;
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
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();
	for(int i=0;i<count;i+=2){
		int seedLength=incoming[i];
		int number=incoming[i+1];
		m_seedingData->m_masterSeedLengths[seedLength]+=number;
	}
	
	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS(Message*message){
	(*m_numberOfRanksDoneSeeding)++;

	if((*m_numberOfRanksDoneSeeding)==m_size){
		(*m_numberOfRanksDoneSeeding)=0;
		(*m_master_mode)=RAY_MASTER_MODE_TRIGGER_DETECTION;
		m_seedingData->writeSeedStatistics();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER(Message*message){

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS(Message*message){
	Rank source=message->getSource();
	MessageUnit*dummy=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

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
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();

	#ifdef ASSERT
	if(count*5*sizeof(MessageUnit)>MAXIMUM_MESSAGE_SIZE_IN_BYTES){
		cout<<"Count="<<count<<endl;
	}
	assert(count*5*sizeof(MessageUnit)<=MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif

	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(count*5*sizeof(MessageUnit));
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
	MessageUnit*incoming=(MessageUnit*)buffer;
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


void MessageProcessor::call_RAY_MPI_TAG_ASK_EXTENSION_DATA(Message*message){
	(m_seedingData->m_SEEDING_i)=0;
	(m_ed->m_EXTENSION_currentPosition)=0;
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA_REPLY(Message*message){
	(*m_ready)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();
	for(int i=0;i<count;i++){
		(*m_allPaths)[(*m_allPaths).size()-1].push_back(incoming[i+0]);
	}

	Message aMessage(NULL,0,message->getSource(),RAY_MPI_TAG_EXTENSION_DATA_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_DATA_END(Message*message){
	(m_ed->m_EXTENSION_currentRankIsDone)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_ATTACH_SEQUENCE(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	MessageUnit*incoming=(MessageUnit*)buffer;
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
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	Message aMessage(message2,count,message->getSource(),RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY(Message*message){
	m_si->setReadiness();
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION(Message*message){
	void*buffer=message->getBuffer();
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	Strand strand=incoming[2];
	Kmer vertex=(*m_myReads)[incoming[0]]->getVertex(incoming[1],(*m_wordSize),strand,m_parameters->getColorSpaceMode());
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
	int bufferPosition=0;
	vertex.pack(message2,&bufferPosition);
	Message aMessage(message2,bufferPosition,source,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY(Message*message){
	MessageUnit*buffer=(MessageUnit*)message->getBuffer();
	(m_ed->m_EXTENSION_read_vertex_received)=true;
	int bufferPosition=0;
	m_ed->m_EXTENSION_receivedReadVertex.unpack(buffer,&bufferPosition);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_LENGTH(Message*message){
	void*buffer=message->getBuffer();
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int index=incoming[0];
	#ifdef ASSERT
	assert(index<(int)m_myReads->size());
	#endif
	Read*read=(*m_myReads)[index];
	#ifdef ASSERT
	assert(read!=NULL);
	#endif
	int length=read->length();
	
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(3*sizeof(MessageUnit));
	int pos=0;
	message2[pos++]=length;
	message2[pos++]=read->getForwardOffset();
	message2[pos++]=read->getReverseOffset();

	Message aMessage(message2,pos,source,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_READ_LENGTH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
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
	MessageUnit*incoming=(MessageUnit*)buffer;
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

		PathHandle wave=incoming[pos++];
		
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

void MessageProcessor::call_RAY_MPI_TAG_ASSEMBLE_WAVES_DONE(Message*message){
	(m_ed->m_EXTENSION_currentRankIsDone)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_START_FUSION(Message*message){
	(m_seedingData->m_SEEDING_i)=0;
	m_fusionData->initialise();
}

void MessageProcessor::call_RAY_MPI_TAG_FUSION_DONE(Message*message){
	
	MessageUnit*incoming=message->getBuffer();
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
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
	int count=message->getCount();
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(count*sizeof(MessageUnit));

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
	MessageUnit*incoming=(MessageUnit*)buffer;
	m_fusionData->m_FUSION_paths_received=true;
	m_fusionData->m_FUSION_receivedPaths.clear();
	m_fusionData->m_FUSION_numberOfPaths=incoming[0];
	m_seedingData->m_SEEDING_receivedVertexCoverage=incoming[1];
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_LENGTH(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

	for(int i=0;i<count;i++){
		PathHandle id=incoming[i];
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

void MessageProcessor::call_RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION(Message*message){
	void*buffer=message->getBuffer();
	int count=message->getCount();
	int source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

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
	MessageUnit*incoming=(MessageUnit*)buffer;
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
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	Kmer vertex;
	int pos=0;
	vertex.unpack(incoming,&pos);

	int firstPathId=incoming[pos];
	vector<Direction> paths=m_subgraph->getDirections(&vertex);

	int availableElements=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit);
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int outputPosition=0;
	vertex.pack(message2,&outputPosition);
	int origin=outputPosition;
	outputPosition++;

	while(firstPathId<(int)paths.size() && (outputPosition+2)<availableElements){
		#ifdef ASSERT
		assert(firstPathId<(int)paths.size());
		#endif

		PathHandle pathId=paths[firstPathId].getWave();
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
	MessageUnit*incoming=(MessageUnit*)buffer;
	Kmer vertex;
	int pos=0;
	vertex.unpack(incoming,&pos);
	int count=message->getCount();
	Kmer complement=m_parameters->_complementVertex(&vertex);
	bool lower=vertex<complement;
	int origin=pos;
	pos++;
	for(int i=pos;i<count;i+=2){
		PathHandle pathId=incoming[i];
		#ifdef ASSERT
		assert(getRankFromPathUniqueId(pathId)<m_size);
		#endif
		int position=incoming[i+1];
		m_fusionData->m_FUSION_receivedPath.constructor(pathId,position,lower);
		m_fusionData->m_Machine_getPaths_result.push_back(m_fusionData->m_FUSION_receivedPath);
	}

	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(2*sizeof(MessageUnit));
	int outputPosition=0;
	vertex.pack(message2,&outputPosition);
	message2[outputPosition++]=incoming[origin];
	Message aMessage(message2,outputPosition,message->getSource(),RAY_MPI_TAG_ASK_VERTEX_PATHS,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
	Kmer vertex;
	int bufferPosition=0;
	vertex.unpack(incoming,&bufferPosition);
	int count=message->getCount();
	Kmer complement=m_parameters->_complementVertex(&vertex);
	bufferPosition++;
	bool lower=vertex<complement;
	for(int i=bufferPosition;i<count;i+=2){
		PathHandle pathId=incoming[i];
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
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();
	int elementsPerItem=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATH);
	int pos=0;
	int outputPosition=0;
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(count*sizeof(MessageUnit));

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
		int indexInArray=incoming[pos++];

		/* increment because there is padding */
		pos++;

		#ifdef ASSERT
		assert(indexInArray<(int)paths.size());
		#endif

		Direction d=paths[indexInArray];
		kmer.pack(message2,&outputPosition);
		message2[outputPosition++]=d.getWave();

		#ifdef ASSERT
		Rank rank=getRankFromPathUniqueId(d.getWave());
		if(rank>=m_size){
			cout<<"Fatal error: rank: "<<rank<<endl;
		}
		assert(rank<m_size);
		#endif

		message2[outputPosition++]=d.getProgression();
	}

	Message aMessage(message2,outputPosition,source,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
	m_fusionData->m_FUSION_path_received=true;
	Kmer vertex;
	int pos=0;
	vertex.unpack(incoming,&pos);
	Kmer complement=m_parameters->_complementVertex(&vertex);
	bool lower=vertex<complement;
	MessageUnit pathId=incoming[pos];

	#ifdef ASSERT
	assert(getRankFromPathUniqueId(pathId)<m_size);
	#endif

	int position=incoming[pos];
	m_fusionData->m_FUSION_receivedPath.constructor(pathId,position,lower);
}

void MessageProcessor::call_RAY_MPI_TAG_HAS_PAIRED_READ(Message*message){
	Rank source=message->getSource();
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
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
	MessageUnit*incoming=(MessageUnit*)buffer;
	(m_ed->m_EXTENSION_hasPairedReadAnswer)=incoming[0];
	(m_ed->m_EXTENSION_hasPairedReadReceived)=true;
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PAIRED_READ(Message*message){
	void*buffer=message->getBuffer();
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
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

	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(3*sizeof(MessageUnit));
	message2[0]=t->getRank();
	message2[1]=t->getId();
	message2[2]=t->getLibrary();

	Message aMessage(message2,3,source,RAY_MPI_TAG_GET_PAIRED_READ_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PAIRED_READ_REPLY(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
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
	LargeCount cleared=0;
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
			PathHandle id=(m_ed->m_EXTENSION_identifiers)[i];

			if(m_fusionData->m_FUSION_eliminated.count(id)>0){
				eliminated=true;
			}
		}

		if(!eliminated){
			fusions.push_back((m_ed->m_EXTENSION_contigs)[i]);
			if(!appendReverseComplement){
				continue;
			}

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
		PathHandle id=getPathUniqueId(m_rank,i);
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
		PathHandle id=(m_ed->m_EXTENSION_identifiers)[i];
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
	MessageUnit*incoming=message->getBuffer();

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

void MessageProcessor::call_RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED(Message*message){
	(*m_DISTRIBUTE_n)++;
}

void MessageProcessor::call_RAY_MPI_TAG_EXTENSION_START(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
	vector<uint64_t> a;
	(*m_allPaths).push_back(a);

	PathHandle id=incoming[0];

	#ifdef ASSERT
	Rank rank=getRankFromPathUniqueId(id);
	assert(rank<m_size);
	#endif

	(*m_identifiers).push_back(id);
}

void MessageProcessor::call_RAY_MPI_TAG_ELIMINATE_PATH(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
	m_fusionData->m_FUSION_eliminated.insert(incoming[0]);
}

void MessageProcessor::call_RAY_MPI_TAG_GET_PATH_VERTEX(Message*message){
	void*buffer=message->getBuffer();
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();

	int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_PATH_VERTEX);
	MessageUnit*messageBytes=(MessageUnit*)m_outboxAllocator->allocate(sizeof(MessageUnit));

	for(int i=0;i<count;i+=elementsPerQuery){
		PathHandle id=incoming[i];
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
	MessageUnit*incoming=(MessageUnit*)buffer;
	m_fusionData->m_FINISH_vertex_received=true;
	int pos=0;
	m_fusionData->m_FINISH_received_vertex.unpack(incoming,&pos);
}

void MessageProcessor::call_RAY_MPI_TAG_WRITE_AMOS(Message*message){
	m_ed->m_EXTENSION_initiated=false;
	m_ed->m_EXTENSION_currentPosition=((MessageUnit*)message->getBuffer())[0];
}

void MessageProcessor::call_RAY_MPI_TAG_WRITE_AMOS_REPLY(Message*message){
	m_ed->m_EXTENSION_currentRankIsDone=true;
	m_ed->m_EXTENSION_currentPosition=((MessageUnit*)message->getBuffer())[0];
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
	MessageUnit*incoming=(MessageUnit*)buffer;
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
	MessageUnit*incoming=(MessageUnit*)buffer;
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

void MessageProcessor::call_RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED(Message*message){
	m_kmerAcademyFinishedRanks++;
	if(m_kmerAcademyFinishedRanks==m_parameters->getSize()){
		m_switchMan->closeMasterMode();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY(Message*message){
	(*m_numberOfRanksWithCoverageData)++;
	if((*m_numberOfRanksWithCoverageData)==m_size){
		m_switchMan->closeMasterMode();
	}
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE(Message*message){
	void*buffer=message->getBuffer();
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
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

	int beforeRounding=5*sizeof(MessageUnit)+m_myReads->at(index)->getRequiredBytes();
	int toAllocate=roundNumber(beforeRounding,sizeof(MessageUnit));

	MessageUnit*messageBytes=(MessageUnit*)m_outboxAllocator->allocate(toAllocate);
	messageBytes[0]=t->getRank();
	messageBytes[1]=t->getId();
	messageBytes[2]=t->getLibrary();
	messageBytes[3]=(*m_myReads)[index]->getType();
	messageBytes[4]=m_myReads->at(index)->length();

	char*dest=(char*)(messageBytes+5);
	memcpy(dest,m_myReads->at(index)->getRawSequence(),m_myReads->at(index)->getRequiredBytes());
	Message aMessage(messageBytes,toAllocate/sizeof(MessageUnit),source,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY,m_rank);
	m_outbox->push_back(aMessage);
}

void MessageProcessor::call_RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
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
		m_scaffolder->m_initialised=false;

		(*m_master_mode)=RAY_MASTER_MODE_WRITE_SCAFFOLDS;
	}
}

void MessageProcessor::call_RAY_MPI_TAG_GET_CONTIG_CHUNK(Message*message){
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
	PathHandle contigId=incoming[0];
	int position=incoming[1];
	int index=m_fusionData->m_FUSION_identifier_map[contigId];
	int length=m_ed->m_EXTENSION_contigs[index].size();
	MessageUnit*messageContent=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int outputPosition=0;
	int origin=outputPosition;
	outputPosition++;
	int count=0;

	while(position<length
	 && (outputPosition+KMER_U64_ARRAY_SIZE)<(int)(MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit))){
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
	vector<PathHandle>*m_identifiers,
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
	map<CoverageDepth,LargeCount>*m_coverageDistribution,
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

void MessageProcessor::setVirtualCommunicator(VirtualCommunicator*a){
	m_virtualCommunicator=a;
}

void MessageProcessor::setSwitchMan(SwitchMan*a){
	m_switchMan=a;
}

void MessageProcessor::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"MessageProcessor");
	core->setPluginDescription(plugin,"Legacy plugin for message tags");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_MPI_TAG_LOAD_SEQUENCES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_LOAD_SEQUENCES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_LOAD_SEQUENCES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_LOAD_SEQUENCES,"RAY_MPI_TAG_LOAD_SEQUENCES");

	RAY_MPI_TAG_CONTIG_INFO=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_CONTIG_INFO, __GetAdapter(MessageProcessor,RAY_MPI_TAG_CONTIG_INFO));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_CONTIG_INFO,"RAY_MPI_TAG_CONTIG_INFO");

	RAY_MPI_TAG_SCAFFOLDING_LINKS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SCAFFOLDING_LINKS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SCAFFOLDING_LINKS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SCAFFOLDING_LINKS,"RAY_MPI_TAG_SCAFFOLDING_LINKS");

	RAY_MPI_TAG_GET_READ_MARKERS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_READ_MARKERS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_READ_MARKERS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_READ_MARKERS,"RAY_MPI_TAG_GET_READ_MARKERS");

	RAY_MPI_TAG_GET_READ_MATE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_READ_MATE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_READ_MATE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_READ_MATE,"RAY_MPI_TAG_GET_READ_MATE");

	RAY_MPI_TAG_REQUEST_VERTEX_READS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_READS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_READS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_READS,"RAY_MPI_TAG_REQUEST_VERTEX_READS");

	RAY_MPI_TAG_SET_WORD_SIZE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SET_WORD_SIZE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SET_WORD_SIZE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SET_WORD_SIZE,"RAY_MPI_TAG_SET_WORD_SIZE");

	RAY_MPI_TAG_VERTEX_READS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_VERTEX_READS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_READS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTEX_READS,"RAY_MPI_TAG_VERTEX_READS");

	RAY_MPI_TAG_VERTEX_INFO=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_VERTEX_INFO, __GetAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_INFO));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTEX_INFO,"RAY_MPI_TAG_VERTEX_INFO");

	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT");

	RAY_MPI_TAG_VERTEX_READS_FROM_LIST=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_VERTEX_READS_FROM_LIST, __GetAdapter(MessageProcessor,RAY_MPI_TAG_VERTEX_READS_FROM_LIST));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTEX_READS_FROM_LIST,"RAY_MPI_TAG_VERTEX_READS_FROM_LIST");

	RAY_MPI_TAG_START_INDEXING_SEQUENCES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_START_INDEXING_SEQUENCES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_START_INDEXING_SEQUENCES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_START_INDEXING_SEQUENCES,"RAY_MPI_TAG_START_INDEXING_SEQUENCES");

	RAY_MPI_TAG_SEQUENCES_READY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SEQUENCES_READY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SEQUENCES_READY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEQUENCES_READY,"RAY_MPI_TAG_SEQUENCES_READY");

	RAY_MPI_TAG_VERTICES_DATA=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_VERTICES_DATA, __GetAdapter(MessageProcessor,RAY_MPI_TAG_VERTICES_DATA));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTICES_DATA,"RAY_MPI_TAG_VERTICES_DATA");

	RAY_MPI_TAG_VERTICES_DATA_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTICES_DATA_REPLY,"RAY_MPI_TAG_VERTICES_DATA_REPLY");

	RAY_MPI_TAG_PURGE_NULL_EDGES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_PURGE_NULL_EDGES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_PURGE_NULL_EDGES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_PURGE_NULL_EDGES,"RAY_MPI_TAG_PURGE_NULL_EDGES");

	RAY_MPI_TAG_VERTICES_DISTRIBUTED=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_VERTICES_DISTRIBUTED, __GetAdapter(MessageProcessor,RAY_MPI_TAG_VERTICES_DISTRIBUTED));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_VERTICES_DISTRIBUTED,"RAY_MPI_TAG_VERTICES_DISTRIBUTED");

	RAY_MPI_TAG_OUT_EDGES_DATA_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_OUT_EDGES_DATA_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_OUT_EDGES_DATA_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_OUT_EDGES_DATA_REPLY,"RAY_MPI_TAG_OUT_EDGES_DATA_REPLY");

	RAY_MPI_TAG_OUT_EDGES_DATA=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_OUT_EDGES_DATA, __GetAdapter(MessageProcessor,RAY_MPI_TAG_OUT_EDGES_DATA));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_OUT_EDGES_DATA,"RAY_MPI_TAG_OUT_EDGES_DATA");

	RAY_MPI_TAG_START_VERTICES_DISTRIBUTION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION,"RAY_MPI_TAG_START_VERTICES_DISTRIBUTION");

	RAY_MPI_TAG_IN_EDGES_DATA_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_IN_EDGES_DATA_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_IN_EDGES_DATA_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_IN_EDGES_DATA_REPLY,"RAY_MPI_TAG_IN_EDGES_DATA_REPLY");

	RAY_MPI_TAG_IN_EDGES_DATA=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_IN_EDGES_DATA, __GetAdapter(MessageProcessor,RAY_MPI_TAG_IN_EDGES_DATA));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_IN_EDGES_DATA,"RAY_MPI_TAG_IN_EDGES_DATA");

	RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION,"RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION");

	RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER, __GetAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER,"RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER");

	RAY_MPI_TAG_TEST_NETWORK_MESSAGE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_TEST_NETWORK_MESSAGE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_TEST_NETWORK_MESSAGE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_TEST_NETWORK_MESSAGE,"RAY_MPI_TAG_TEST_NETWORK_MESSAGE");

	RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION,"RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION");

	RAY_MPI_TAG_COVERAGE_DATA=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_COVERAGE_DATA, __GetAdapter(MessageProcessor,RAY_MPI_TAG_COVERAGE_DATA));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_COVERAGE_DATA,"RAY_MPI_TAG_COVERAGE_DATA");

	RAY_MPI_TAG_COVERAGE_END=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_COVERAGE_END, __GetAdapter(MessageProcessor,RAY_MPI_TAG_COVERAGE_END));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_COVERAGE_END,"RAY_MPI_TAG_COVERAGE_END");

	RAY_MPI_TAG_SEND_COVERAGE_VALUES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SEND_COVERAGE_VALUES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SEND_COVERAGE_VALUES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEND_COVERAGE_VALUES,"RAY_MPI_TAG_SEND_COVERAGE_VALUES");

	RAY_MPI_TAG_READY_TO_SEED=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_READY_TO_SEED, __GetAdapter(MessageProcessor,RAY_MPI_TAG_READY_TO_SEED));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_READY_TO_SEED,"RAY_MPI_TAG_READY_TO_SEED");

	RAY_MPI_TAG_START_SEEDING=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_START_SEEDING, __GetAdapter(MessageProcessor,RAY_MPI_TAG_START_SEEDING));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_START_SEEDING,"RAY_MPI_TAG_START_SEEDING");

	RAY_MPI_TAG_GET_COVERAGE_AND_MARK=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_COVERAGE_AND_MARK, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_COVERAGE_AND_MARK));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_COVERAGE_AND_MARK,"RAY_MPI_TAG_GET_COVERAGE_AND_MARK");

	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE");

	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY");

	RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES,"RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES");

	RAY_MPI_TAG_REQUEST_VERTEX_EDGES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_EDGES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_EDGES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_EDGES,"RAY_MPI_TAG_REQUEST_VERTEX_EDGES");

	RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY,"RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY");

	RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY,"RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY");

	RAY_MPI_TAG_SEEDING_IS_OVER=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SEEDING_IS_OVER, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SEEDING_IS_OVER));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEEDING_IS_OVER,"RAY_MPI_TAG_SEEDING_IS_OVER");

	RAY_MPI_TAG_REQUEST_SEED_LENGTHS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_SEED_LENGTHS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_SEED_LENGTHS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_SEED_LENGTHS,"RAY_MPI_TAG_REQUEST_SEED_LENGTHS");

	RAY_MPI_TAG_SEND_SEED_LENGTHS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SEND_SEED_LENGTHS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SEND_SEED_LENGTHS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEND_SEED_LENGTHS,"RAY_MPI_TAG_SEND_SEED_LENGTHS");

	RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS,"RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS");

	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER");

	RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS,"RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS");

	RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY,"RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY");

	RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES,"RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES");

	RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY,"RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY");

	RAY_MPI_TAG_EXTENSION_IS_DONE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_EXTENSION_IS_DONE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_IS_DONE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_EXTENSION_IS_DONE,"RAY_MPI_TAG_EXTENSION_IS_DONE");

	RAY_MPI_TAG_ASK_EXTENSION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_EXTENSION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_EXTENSION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_EXTENSION,"RAY_MPI_TAG_ASK_EXTENSION");


	RAY_MPI_TAG_ASK_EXTENSION_DATA=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_EXTENSION_DATA, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_EXTENSION_DATA));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_EXTENSION_DATA,"RAY_MPI_TAG_ASK_EXTENSION_DATA");

	RAY_MPI_TAG_EXTENSION_DATA_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_EXTENSION_DATA_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_EXTENSION_DATA_REPLY,"RAY_MPI_TAG_EXTENSION_DATA_REPLY");

	RAY_MPI_TAG_EXTENSION_DATA=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_EXTENSION_DATA, __GetAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_EXTENSION_DATA,"RAY_MPI_TAG_EXTENSION_DATA");

	RAY_MPI_TAG_EXTENSION_DATA_END=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_EXTENSION_DATA_END, __GetAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_DATA_END));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_EXTENSION_DATA_END,"RAY_MPI_TAG_EXTENSION_DATA_END");

	RAY_MPI_TAG_ATTACH_SEQUENCE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ATTACH_SEQUENCE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ATTACH_SEQUENCE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ATTACH_SEQUENCE,"RAY_MPI_TAG_ATTACH_SEQUENCE");

	RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY,"RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY");

	RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION,"RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION");

	RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY,"RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY");

	RAY_MPI_TAG_ASK_READ_LENGTH=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_READ_LENGTH, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_LENGTH));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_READ_LENGTH,"RAY_MPI_TAG_ASK_READ_LENGTH");

	RAY_MPI_TAG_ASK_READ_LENGTH_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_READ_LENGTH_REPLY,"RAY_MPI_TAG_ASK_READ_LENGTH_REPLY");

	RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY,"RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY");

	RAY_MPI_TAG_SAVE_WAVE_PROGRESSION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION,"RAY_MPI_TAG_SAVE_WAVE_PROGRESSION");

	RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY,"RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY");

	RAY_MPI_TAG_ASSEMBLE_WAVES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASSEMBLE_WAVES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASSEMBLE_WAVES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASSEMBLE_WAVES,"RAY_MPI_TAG_ASSEMBLE_WAVES");

	RAY_MPI_TAG_ASSEMBLE_WAVES_DONE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASSEMBLE_WAVES_DONE,"RAY_MPI_TAG_ASSEMBLE_WAVES_DONE");

	RAY_MPI_TAG_START_FUSION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_START_FUSION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_START_FUSION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_START_FUSION,"RAY_MPI_TAG_START_FUSION");

	RAY_MPI_TAG_FUSION_DONE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_FUSION_DONE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_FUSION_DONE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_FUSION_DONE,"RAY_MPI_TAG_FUSION_DONE");

	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE");

	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY");

	RAY_MPI_TAG_GET_PATH_LENGTH=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_PATH_LENGTH, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_LENGTH));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_PATH_LENGTH,"RAY_MPI_TAG_GET_PATH_LENGTH");

	RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION,"RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION");

	RAY_MPI_TAG_GET_PATH_LENGTH_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_PATH_LENGTH_REPLY,"RAY_MPI_TAG_GET_PATH_LENGTH_REPLY");

	RAY_MPI_TAG_ASK_VERTEX_PATHS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS,"RAY_MPI_TAG_ASK_VERTEX_PATHS");

	RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY,"RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY");

	RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END,"RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END");

	RAY_MPI_TAG_ASK_VERTEX_PATH=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_VERTEX_PATH, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATH));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_VERTEX_PATH,"RAY_MPI_TAG_ASK_VERTEX_PATH");

	RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY,"RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY");

	RAY_MPI_TAG_HAS_PAIRED_READ=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_HAS_PAIRED_READ, __GetAdapter(MessageProcessor,RAY_MPI_TAG_HAS_PAIRED_READ));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_HAS_PAIRED_READ,"RAY_MPI_TAG_HAS_PAIRED_READ");

	RAY_MPI_TAG_HAS_PAIRED_READ_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_HAS_PAIRED_READ_REPLY,"RAY_MPI_TAG_HAS_PAIRED_READ_REPLY");

	RAY_MPI_TAG_GET_PAIRED_READ=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_PAIRED_READ, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_PAIRED_READ));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_PAIRED_READ,"RAY_MPI_TAG_GET_PAIRED_READ");

	RAY_MPI_TAG_GET_PAIRED_READ_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_PAIRED_READ_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_PAIRED_READ_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_PAIRED_READ_REPLY,"RAY_MPI_TAG_GET_PAIRED_READ_REPLY");

	RAY_MPI_TAG_CLEAR_DIRECTIONS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_CLEAR_DIRECTIONS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_CLEAR_DIRECTIONS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_CLEAR_DIRECTIONS,"RAY_MPI_TAG_CLEAR_DIRECTIONS");

	RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY,"RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY");

	RAY_MPI_TAG_FINISH_FUSIONS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_FINISH_FUSIONS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_FINISH_FUSIONS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_FINISH_FUSIONS,"RAY_MPI_TAG_FINISH_FUSIONS");

	RAY_MPI_TAG_FINISH_FUSIONS_FINISHED=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED, __GetAdapter(MessageProcessor,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED,"RAY_MPI_TAG_FINISH_FUSIONS_FINISHED");

	RAY_MPI_TAG_DISTRIBUTE_FUSIONS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_DISTRIBUTE_FUSIONS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_DISTRIBUTE_FUSIONS,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS");

	RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY");

	RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED, __GetAdapter(MessageProcessor,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED");

	RAY_MPI_TAG_EXTENSION_START=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_EXTENSION_START, __GetAdapter(MessageProcessor,RAY_MPI_TAG_EXTENSION_START));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_EXTENSION_START,"RAY_MPI_TAG_EXTENSION_START");

	RAY_MPI_TAG_ELIMINATE_PATH=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ELIMINATE_PATH, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ELIMINATE_PATH));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ELIMINATE_PATH,"RAY_MPI_TAG_ELIMINATE_PATH");

	RAY_MPI_TAG_GET_PATH_VERTEX=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_PATH_VERTEX, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_VERTEX));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_PATH_VERTEX,"RAY_MPI_TAG_GET_PATH_VERTEX");

	RAY_MPI_TAG_GET_PATH_VERTEX_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_PATH_VERTEX_REPLY,"RAY_MPI_TAG_GET_PATH_VERTEX_REPLY");

	RAY_MPI_TAG_WRITE_AMOS=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_WRITE_AMOS, __GetAdapter(MessageProcessor,RAY_MPI_TAG_WRITE_AMOS));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_WRITE_AMOS,"RAY_MPI_TAG_WRITE_AMOS");

	RAY_MPI_TAG_WRITE_AMOS_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_WRITE_AMOS_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_WRITE_AMOS_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_WRITE_AMOS_REPLY,"RAY_MPI_TAG_WRITE_AMOS_REPLY");

	RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION,"RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION");

	RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE,"RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE");

	RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY,"RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY");

	RAY_MPI_TAG_LIBRARY_DISTANCE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_LIBRARY_DISTANCE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_LIBRARY_DISTANCE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_LIBRARY_DISTANCE,"RAY_MPI_TAG_LIBRARY_DISTANCE");

	RAY_MPI_TAG_ASK_LIBRARY_DISTANCES=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES,"RAY_MPI_TAG_ASK_LIBRARY_DISTANCES");

	RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED, __GetAdapter(MessageProcessor,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED,"RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED");

	RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION, __GetAdapter(MessageProcessor,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION,"RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION");


	RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED, __GetAdapter(MessageProcessor,RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED,"RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED");

	RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY,"RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY");

	RAY_MPI_TAG_REQUEST_READ_SEQUENCE=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_READ_SEQUENCE, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_READ_SEQUENCE));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_READ_SEQUENCE,"RAY_MPI_TAG_REQUEST_READ_SEQUENCE");

	RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY, __GetAdapter(MessageProcessor,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY,"RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY");

	RAY_MPI_TAG_I_FINISHED_SCAFFOLDING=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_I_FINISHED_SCAFFOLDING, __GetAdapter(MessageProcessor,RAY_MPI_TAG_I_FINISHED_SCAFFOLDING));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_I_FINISHED_SCAFFOLDING,"RAY_MPI_TAG_I_FINISHED_SCAFFOLDING");

	RAY_MPI_TAG_GET_CONTIG_CHUNK=core->allocateMessageTagHandle(plugin);
	core->setMessageTagObjectHandler(plugin,RAY_MPI_TAG_GET_CONTIG_CHUNK, __GetAdapter(MessageProcessor,RAY_MPI_TAG_GET_CONTIG_CHUNK));
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GET_CONTIG_CHUNK,"RAY_MPI_TAG_GET_CONTIG_CHUNK");

}


void MessageProcessor::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_PURGE_NULL_EDGES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_PURGE_NULL_EDGES");

	RAY_SLAVE_MODE_AMOS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_AMOS");
	RAY_SLAVE_MODE_ASSEMBLE_WAVES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ASSEMBLE_WAVES");
	RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION");
	RAY_SLAVE_MODE_ADD_VERTICES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ADD_VERTICES");
	RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS");
	RAY_SLAVE_MODE_EXTENSION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_EXTENSION");
	RAY_SLAVE_MODE_FINISH_FUSIONS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_FINISH_FUSIONS");
	RAY_SLAVE_MODE_FUSION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_FUSION");
	RAY_SLAVE_MODE_INDEX_SEQUENCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_INDEX_SEQUENCES");
	RAY_SLAVE_MODE_LOAD_SEQUENCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_LOAD_SEQUENCES");
	RAY_SLAVE_MODE_SEND_DISTRIBUTION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_DISTRIBUTION");
	RAY_SLAVE_MODE_SEND_EXTENSION_DATA=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_EXTENSION_DATA");
	RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES");
	RAY_SLAVE_MODE_SEND_SEED_LENGTHS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_SEND_SEED_LENGTHS");
	RAY_SLAVE_MODE_START_SEEDING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_START_SEEDING");

	RAY_MASTER_MODE_ASK_DISTANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ASK_DISTANCES");
	RAY_MASTER_MODE_DO_NOTHING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_DO_NOTHING");
	RAY_MASTER_MODE_PREPARE_SEEDING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PREPARE_SEEDING");
	RAY_MASTER_MODE_PURGE_NULL_EDGES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PURGE_NULL_EDGES");
	RAY_MASTER_MODE_START_UPDATING_DISTANCES=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_START_UPDATING_DISTANCES");
	RAY_MASTER_MODE_TRIGGER_DETECTION=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_DETECTION");
	RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS");
	RAY_MASTER_MODE_TRIGGER_FUSIONS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_FUSIONS");
	RAY_MASTER_MODE_TRIGGER_SEEDING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_SEEDING");
	RAY_MASTER_MODE_WRITE_SCAFFOLDS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_WRITE_SCAFFOLDS");



	RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION");
	RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_ANSWER");
	RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION_QUESTION");
	RAY_MPI_TAG_PURGE_NULL_EDGES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PURGE_NULL_EDGES");
	RAY_MPI_TAG_READY_TO_SEED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_READY_TO_SEED");
	RAY_MPI_TAG_REQUEST_READ_SEQUENCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_READ_SEQUENCE");
	RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY");
	RAY_MPI_TAG_REQUEST_SEED_LENGTHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_SEED_LENGTHS");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_EDGES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_EDGES");
	RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_EDGES_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES");
	RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_INGOING_EDGES_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES");
	RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY");
	RAY_MPI_TAG_REQUEST_VERTEX_READS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_READS");
	RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY");
	RAY_MPI_TAG_SAVE_WAVE_PROGRESSION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SAVE_WAVE_PROGRESSION");
	RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_REPLY");
	RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY");
	RAY_MPI_TAG_SCAFFOLDING_LINKS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SCAFFOLDING_LINKS");
	RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY");
	RAY_MPI_TAG_SEEDING_IS_OVER=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEEDING_IS_OVER");
	RAY_MPI_TAG_SEND_COVERAGE_VALUES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEND_COVERAGE_VALUES");
	RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEND_COVERAGE_VALUES_REPLY");
	RAY_MPI_TAG_SEND_SEED_LENGTHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEND_SEED_LENGTHS");
	RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEND_SEED_LENGTHS_REPLY");
	RAY_MPI_TAG_SEQUENCES_READY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SEQUENCES_READY");
	RAY_MPI_TAG_SET_WORD_SIZE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SET_WORD_SIZE");
	RAY_MPI_TAG_START_FUSION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_FUSION");
	RAY_MPI_TAG_START_INDEXING_SEQUENCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_INDEXING_SEQUENCES");
	RAY_MPI_TAG_START_SEEDING=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_SEEDING");
	RAY_MPI_TAG_START_VERTICES_DISTRIBUTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_START_VERTICES_DISTRIBUTION");
	RAY_MPI_TAG_TEST_NETWORK_MESSAGE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TEST_NETWORK_MESSAGE");
	RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY");
	RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION");
	RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY");
	RAY_MPI_TAG_VERTEX_INFO=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_INFO");
	RAY_MPI_TAG_VERTEX_INFO_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_INFO_REPLY");
	RAY_MPI_TAG_VERTEX_READS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_READS");
	RAY_MPI_TAG_VERTEX_READS_FROM_LIST=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_READS_FROM_LIST");
	RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY");
	RAY_MPI_TAG_VERTEX_READS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTEX_READS_REPLY");
	RAY_MPI_TAG_VERTICES_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTICES_DATA");
	RAY_MPI_TAG_VERTICES_DATA_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTICES_DATA_REPLY");
	RAY_MPI_TAG_VERTICES_DISTRIBUTED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_VERTICES_DISTRIBUTED");
	RAY_MPI_TAG_WRITE_AMOS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_AMOS");
	RAY_MPI_TAG_WRITE_AMOS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_WRITE_AMOS_REPLY");

	RAY_MPI_TAG_CONTIG_INFO=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_INFO");
	RAY_MPI_TAG_CONTIG_INFO_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CONTIG_INFO_REPLY");
	RAY_MPI_TAG_COVERAGE_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COVERAGE_DATA");
	RAY_MPI_TAG_COVERAGE_DATA_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COVERAGE_DATA_REPLY");
	RAY_MPI_TAG_COVERAGE_END=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_COVERAGE_END");
	RAY_MPI_TAG_DISTRIBUTE_FUSIONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS");
	RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED");
	RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY");
	RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY");
	RAY_MPI_TAG_ELIMINATE_PATH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ELIMINATE_PATH");
	RAY_MPI_TAG_EXTENSION_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_EXTENSION_DATA");
	RAY_MPI_TAG_EXTENSION_DATA_END=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_EXTENSION_DATA_END");
	RAY_MPI_TAG_EXTENSION_DATA_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_EXTENSION_DATA_REPLY");
	RAY_MPI_TAG_EXTENSION_IS_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_EXTENSION_IS_DONE");
	RAY_MPI_TAG_EXTENSION_START=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_EXTENSION_START");
	RAY_MPI_TAG_FINISH_FUSIONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_FINISH_FUSIONS");
	RAY_MPI_TAG_FINISH_FUSIONS_FINISHED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_FINISH_FUSIONS_FINISHED");
	RAY_MPI_TAG_FUSION_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_FUSION_DONE");
	RAY_MPI_TAG_GET_CONTIG_CHUNK=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_CONTIG_CHUNK");
	RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY");
	RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION");
	RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY");
	RAY_MPI_TAG_GET_COVERAGE_AND_MARK=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_MARK");
	RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY");
	RAY_MPI_TAG_GET_PAIRED_READ=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PAIRED_READ");
	RAY_MPI_TAG_GET_PAIRED_READ_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PAIRED_READ_REPLY");
	RAY_MPI_TAG_GET_PATH_LENGTH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_LENGTH");
	RAY_MPI_TAG_GET_PATH_LENGTH_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_LENGTH_REPLY");
	RAY_MPI_TAG_GET_PATH_VERTEX=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_VERTEX");
	RAY_MPI_TAG_GET_PATH_VERTEX_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_VERTEX_REPLY");
	RAY_MPI_TAG_GET_READ_MARKERS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MARKERS");
	RAY_MPI_TAG_GET_READ_MARKERS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MARKERS_REPLY");
	RAY_MPI_TAG_GET_READ_MATE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MATE");
	RAY_MPI_TAG_GET_READ_MATE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_READ_MATE_REPLY");
	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT");
	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY");
	RAY_MPI_TAG_HAS_PAIRED_READ=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_HAS_PAIRED_READ");
	RAY_MPI_TAG_HAS_PAIRED_READ_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_HAS_PAIRED_READ_REPLY");
	RAY_MPI_TAG_I_FINISHED_SCAFFOLDING=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_I_FINISHED_SCAFFOLDING");
	RAY_MPI_TAG_IN_EDGES_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_IN_EDGES_DATA");
	RAY_MPI_TAG_IN_EDGES_DATA_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_IN_EDGES_DATA_REPLY");
	RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS");
	RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_KMER_ACADEMY_DISTRIBUTED");
	RAY_MPI_TAG_LIBRARY_DISTANCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_LIBRARY_DISTANCE");
	RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_LIBRARY_DISTANCE_REPLY");
	RAY_MPI_TAG_LOAD_SEQUENCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_LOAD_SEQUENCES");
	RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS");
	RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY");
	RAY_MPI_TAG_OUT_EDGES_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_OUT_EDGES_DATA");
	RAY_MPI_TAG_OUT_EDGES_DATA_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_OUT_EDGES_DATA_REPLY");

	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER");
	RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ACTIVATE_RELAY_CHECKER_REPLY");
	RAY_MPI_TAG_ASK_EXTENSION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_EXTENSION");
	RAY_MPI_TAG_ASK_EXTENSION_DATA=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_EXTENSION_DATA");
	RAY_MPI_TAG_ASK_LIBRARY_DISTANCES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_LIBRARY_DISTANCES");
	RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED");
	RAY_MPI_TAG_ASK_READ_LENGTH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_READ_LENGTH");
	RAY_MPI_TAG_ASK_READ_LENGTH_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_READ_LENGTH_REPLY");
	RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION");
	RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_READ_VERTEX_AT_POSITION_REPLY");
	RAY_MPI_TAG_ASK_VERTEX_PATH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATH");
	RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY");
	RAY_MPI_TAG_ASK_VERTEX_PATHS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_REPLY_END");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY");
	RAY_MPI_TAG_ASSEMBLE_WAVES=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASSEMBLE_WAVES");
	RAY_MPI_TAG_ASSEMBLE_WAVES_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASSEMBLE_WAVES_DONE");
	RAY_MPI_TAG_ATTACH_SEQUENCE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ATTACH_SEQUENCE");
	RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY");
	RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION");
	RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE");
	RAY_MPI_TAG_CLEAR_DIRECTIONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CLEAR_DIRECTIONS");
	RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_CLEAR_DIRECTIONS_REPLY");

	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_LOAD_SEQUENCES, RAY_SLAVE_MODE_LOAD_SEQUENCES);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_WRITE_AMOS, RAY_SLAVE_MODE_AMOS);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_START_INDEXING_SEQUENCES, RAY_SLAVE_MODE_INDEX_SEQUENCES );
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_START_VERTICES_DISTRIBUTION, RAY_SLAVE_MODE_ADD_VERTICES);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MPI_TAG_PREPARE_COVERAGE_DISTRIBUTION, RAY_SLAVE_MODE_SEND_DISTRIBUTION);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_START_SEEDING, RAY_SLAVE_MODE_START_SEEDING);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_REQUEST_SEED_LENGTHS, RAY_SLAVE_MODE_SEND_SEED_LENGTHS);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_ASK_EXTENSION, RAY_SLAVE_MODE_EXTENSION );
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_ASK_EXTENSION_DATA, RAY_SLAVE_MODE_SEND_EXTENSION_DATA);

	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_ASSEMBLE_WAVES, RAY_SLAVE_MODE_ASSEMBLE_WAVES);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_START_FUSION, RAY_SLAVE_MODE_FUSION);

	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_FINISH_FUSIONS, RAY_SLAVE_MODE_FINISH_FUSIONS);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_DISTRIBUTE_FUSIONS, RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS );
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION, RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_ASK_LIBRARY_DISTANCES, RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES);


	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_GET_CONTIG_CHUNK,             RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_REQUEST_VERTEX_READS, RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_GET_READ_MATE,                RAY_MPI_TAG_GET_READ_MATE_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,      RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_ATTACH_SEQUENCE,              RAY_MPI_TAG_ATTACH_SEQUENCE_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,     RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_HAS_PAIRED_READ,              RAY_MPI_TAG_HAS_PAIRED_READ_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_GET_READ_MARKERS,             RAY_MPI_TAG_GET_READ_MARKERS_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_GET_PATH_LENGTH,              RAY_MPI_TAG_GET_PATH_LENGTH_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION,   RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_SCAFFOLDING_LINKS,            RAY_MPI_TAG_SCAFFOLDING_LINKS_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_CONTIG_INFO,                  RAY_MPI_TAG_CONTIG_INFO_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_ASK_READ_LENGTH,              RAY_MPI_TAG_ASK_READ_LENGTH_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,        RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_ASK_VERTEX_PATH,              RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_GET_PATH_VERTEX,              RAY_MPI_TAG_GET_PATH_VERTEX_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_VERTEX_INFO,                  RAY_MPI_TAG_VERTEX_INFO_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_REQUEST_READ_SEQUENCE,                RAY_MPI_TAG_REQUEST_READ_SEQUENCE_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES,        RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES_REPLY );
	core->setMessageTagReplyMessageTag(m_plugin, RAY_MPI_TAG_TEST_NETWORK_MESSAGE,                 RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY );

	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_GET_CONTIG_CHUNK,             MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit) );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_REQUEST_VERTEX_READS,                 max(5,KMER_U64_ARRAY_SIZE+1) );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_GET_READ_MATE,                4 );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,      KMER_U64_ARRAY_SIZE );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_ATTACH_SEQUENCE,              KMER_U64_ARRAY_SIZE+4 );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,     max(2,KMER_U64_ARRAY_SIZE));
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_HAS_PAIRED_READ,              1 );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_GET_READ_MARKERS,             3+2*KMER_U64_ARRAY_SIZE );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_GET_PATH_LENGTH,              1 );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION,   max(KMER_U64_ARRAY_SIZE,5) );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_SCAFFOLDING_LINKS,            7 );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_CONTIG_INFO,                  2);
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_ASK_READ_LENGTH,              3 );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, KMER_U64_ARRAY_SIZE );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_ASK_VERTEX_PATH, (KMER_U64_ARRAY_SIZE + 2) );
	core->setMessageTagSize(m_plugin, RAY_MPI_TAG_GET_PATH_VERTEX, max(2,KMER_U64_ARRAY_SIZE) );

	__BindPlugin(MessageProcessor);
}


