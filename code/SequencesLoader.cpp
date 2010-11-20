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

#include<string.h>
#include<SequencesLoader.h>
#include<Message.h>
#include<DistributionData.h>
#include<BubbleData.h>
#include<assert.h>
#include<vector>
#include<common_functions.h>
#include<Loader.h>
#include<Read.h>
#include<iostream>
using namespace std;

bool SequencesLoader::loadSequences(int rank,int size,vector<Read*>*m_distribution_reads,int*m_distribution_sequence_id,
	bool*m_LOADER_isLeftFile,StaticVector*m_outbox,int*m_distribution_file_id,MyAllocator*m_distributionAllocator,
	bool*m_LOADER_isRightFile,int*m_LOADER_averageFragmentLength,DistributionData*m_disData,	
	int*m_LOADER_numberOfSequencesInLeftFile,RingAllocator*m_outboxAllocator,
	int*m_distribution_currentSequenceId,int*m_LOADER_deviation,bool*m_loadSequenceStep,BubbleData*m_bubbleData,
	time_t*m_lastTime,
	Parameters*m_parameters,int*m_master_mode
){
	if(!isReady()){
		return true;
	}

	vector<string> allFiles=(*m_parameters).getAllFiles();

	#ifdef ASSERT
	assert(allFiles.size()>0);
	#endif

	if((*m_distribution_reads).size()>0 && (*m_distribution_sequence_id)==(int)(*m_distribution_reads).size()){
		// we reached the end of the file.
		if(!m_send_sequences_done){
			m_send_sequences_done=true;
			(*m_distribution_sequence_id)=0;
			flushAll(m_outboxAllocator,m_outbox);

			#ifdef ASSERT
			for(int i=0;i<m_size;i++){
				assert(m_entries[i]==0);
				cout<<"INFO "<<i<<" "<<m_numberOfSequences[i]<<endl;
			}
			#endif
			cout<<"Rank "<<rank<<" is assigning sequence reads "<<(*m_distribution_reads).size()<<"/"<<(*m_distribution_reads).size()<<" (completed)"<<endl;
		}else{
			m_waitingNumber+=m_disData->m_messagesStockPaired.flushAll(5,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank);
			cout<<"Rank "<<rank<<" is sending paired information "<<(*m_distribution_reads).size()<<"/"<<(*m_distribution_reads).size()<<" (completed)"<<endl;

			(*m_distribution_file_id)++;
			if((*m_LOADER_isLeftFile)){
				(*m_LOADER_numberOfSequencesInLeftFile)=(*m_distribution_sequence_id);
			}
			(*m_distribution_sequence_id)=0;
			(*m_distribution_reads).clear();
		}
	}else if((*m_distribution_file_id)>(int)allFiles.size()-1){
		(*m_master_mode)=MASTER_MODE_DO_NOTHING;
		(*m_loadSequenceStep)=true;

		for(int i=0;i<size;i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i,TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS,rank);
			m_outbox->push_back(aMessage);
		}
		(*m_distributionAllocator).clear();
		(*m_distribution_reads).clear();
	}else if((*m_distribution_reads).size()==0){
		m_send_sequences_done=false;
		Loader loader;
		(*m_distribution_reads).clear();
		(*m_distributionAllocator).clear();
		(*m_distributionAllocator).constructor(DISTRIBUTION_ALLOCATOR_CHUNK_SIZE);
		#ifdef SHOW_PROGRESS
		cout<<endl<<"Rank "<<rank<<" is loading "<<allFiles[(*m_distribution_file_id)]<<""<<endl;
		#else
		cout<<endl<<"Loading "<<allFiles[(*m_distribution_file_id)]<<""<<endl;
		#endif
		loader.load(allFiles[(*m_distribution_file_id)],&(*m_distribution_reads),&(*m_distributionAllocator),&(*m_distributionAllocator));
		m_parameters->setNumberOfSequences(m_distribution_reads->size());
		if((*m_distribution_reads).size()==0){
			return false;
		}

		// write Reads in AMOS format.
		if((*m_parameters).useAmos()){
			FILE*fp=(*m_bubbleData).m_amos;
			for(int i=0;i<(int)(*m_distribution_reads).size();i++){
				int iid=(*m_distribution_currentSequenceId)+i;
				char*seq=(*m_distribution_reads).at(i)->getSeq();
				char*qlt=(char*)__Malloc(strlen(seq)+1);
				strcpy(qlt,seq);
				// spec: https://sourceforge.net/apps/mediawiki/amos/index.php?title=Message_Types#Sequence_t_:_Universal_t
				for(int j=0;j<(int)strlen(qlt);j++)
					qlt[j]='D';
				fprintf(fp,"{RED\niid:%i\neid:%i\nseq:\n%s\n.\nqlt:\n%s\n.\n}\n",iid+1,iid+1,seq,qlt);
				__Free(qlt);
			}
		}

		m_isInterleavedFile=(*m_LOADER_isLeftFile)=(*m_LOADER_isRightFile)=false;
		if((*m_parameters).isLeftFile((*m_distribution_file_id))){
			(*m_LOADER_isLeftFile)=true;
			(*m_LOADER_averageFragmentLength)=(*m_parameters).getFragmentLength((*m_distribution_file_id));
			(*m_LOADER_deviation)=(*m_parameters).getStandardDeviation((*m_distribution_file_id));
		}else if((*m_parameters).isRightFile((*m_distribution_file_id))){
			(*m_LOADER_isRightFile)=true;
			(*m_LOADER_averageFragmentLength)=(*m_parameters).getFragmentLength((*m_distribution_file_id));
			(*m_LOADER_deviation)=(*m_parameters).getStandardDeviation((*m_distribution_file_id));
		}else if((*m_parameters).isInterleavedFile((*m_distribution_file_id))){
			m_isInterleavedFile=true;
			(*m_LOADER_averageFragmentLength)=(*m_parameters).getFragmentLength((*m_distribution_file_id));
			(*m_LOADER_deviation)=(*m_parameters).getStandardDeviation((*m_distribution_file_id));
		}
	}else{
		#ifdef ASSERT
		if(m_send_sequences_done && (*m_distribution_sequence_id)==0){
			for(int i=0;i<m_size;i++){
				assert(m_entries[i]==0);
			}
		}
		#endif

 		// make it wait some times
 		// this avoids spinning too fast in the memory ring of the outbox <
 		// allocator

		if(!m_send_sequences_done){
			int destination=((*m_distribution_currentSequenceId)+(*m_distribution_sequence_id))%size;
			#ifdef ASSERT
			assert(destination>=0);
			assert(destination<size);
			#endif

			int theSpaceLeft=getSpaceLeft(destination);
			char*sequence=((*m_distribution_reads))[(*m_distribution_sequence_id)]->getSeq();
			int spaceNeeded=strlen(sequence)+1;
			if(spaceNeeded>theSpaceLeft){
				flush(destination,m_outboxAllocator,m_outbox);
				return true;
			}
			#ifdef ASSERT
			assert(spaceNeeded<=getSpaceLeft(destination));
			#endif
			
			appendSequence(destination,sequence);
		}

		if((*m_distribution_sequence_id)%100000==0){
			if(!m_send_sequences_done){
				cout<<"Rank "<<rank<<" is assigning sequence reads "<<(*m_distribution_sequence_id)+1<<"/"<<(*m_distribution_reads).size()<<endl;
			}else{
				cout<<"Rank "<<rank<<" is sending paired information "<<(*m_distribution_sequence_id)+1<<"/"<<(*m_distribution_reads).size()<<endl;

			}
		}

		// add paired information here..
		// algorithm follows.
		// check if current file is in a right file.
		// if yes, the leftDistributionCurrentSequenceId=(*m_distribution_currentSequenceId)-NumberOfSequencesInRightFile.
		// the destination of a read i is i%getSize()
		// the readId on destination is i/getSize()
		// so, basically, send these bits to destination:
		//
		// rightSequenceGlobalId:= (*m_distribution_currentSequenceId)
		// rightSequenceRank:= rightSequenceGlobalId%getSize
		// rightSequenceIdOnRank:= rightSequenceGlobalId/getSize
		// leftSequenceGlobalId:= rightSequenceGlobalId-numberOfSequencesInRightFile
		// leftSequenceRank:= leftSequenceGlobalId%getSize
		// leftSequenceIdOnRank:= leftSequenceGlobalId/getSize
		// averageFragmentLength:= ask the pairedFiles in (*m_parameters).
		
		if(m_send_sequences_done &&(*m_LOADER_isRightFile)){
			int rightSequenceGlobalId=(*m_distribution_currentSequenceId);
			int rightSequenceRank=rightSequenceGlobalId%size;
			int rightSequenceIdOnRank=rightSequenceGlobalId/size;
			int leftSequenceGlobalId=rightSequenceGlobalId-(*m_LOADER_numberOfSequencesInLeftFile);

			#ifdef ASSERT
			assert(rightSequenceIdOnRank<m_numberOfSequences[rightSequenceRank]);
			#endif

			int leftSequenceRank=leftSequenceGlobalId%size;
			int leftSequenceIdOnRank=leftSequenceGlobalId/size;
			int averageFragmentLength=(*m_LOADER_averageFragmentLength);
			int deviation=(*m_LOADER_deviation);

			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,rightSequenceIdOnRank);
			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,leftSequenceRank);
			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,leftSequenceIdOnRank);
			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,averageFragmentLength);
			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,deviation);

			if(m_disData->m_messagesStockPaired.flush(rightSequenceRank,5,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank,false)){
				m_waitingNumber++;
			}
		}else if(m_send_sequences_done && (*m_LOADER_isLeftFile)){
			#ifdef ASSERT

			#endif
			int rightSequenceGlobalId=(*m_distribution_currentSequenceId)+(*m_LOADER_numberOfSequencesInLeftFile);
			int rightSequenceRank=rightSequenceGlobalId%size;
			int rightSequenceIdOnRank=rightSequenceGlobalId/size;
			int leftSequenceGlobalId=(*m_distribution_currentSequenceId);

			int leftSequenceRank=leftSequenceGlobalId%size;
			int leftSequenceIdOnRank=leftSequenceGlobalId/size;
			int averageFragmentLength=(*m_LOADER_averageFragmentLength);
			int deviation=(*m_LOADER_deviation);

			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,leftSequenceIdOnRank);
			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,rightSequenceRank);
			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,rightSequenceIdOnRank);
			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,averageFragmentLength);
			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,deviation);

			if(m_disData->m_messagesStockPaired.flush(leftSequenceRank,5,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank,false)){
				m_waitingNumber++;
			}
		}else if(m_send_sequences_done
	&& m_isInterleavedFile && ((*m_distribution_sequence_id)%2)==0){// left sequence
			int rightSequenceGlobalId=(*m_distribution_currentSequenceId)+1;
			int rightSequenceRank=rightSequenceGlobalId%size;
			int rightSequenceIdOnRank=rightSequenceGlobalId/size;

			int leftSequenceGlobalId=rightSequenceGlobalId-1;
			int leftSequenceRank=leftSequenceGlobalId%size;
			int leftSequenceIdOnRank=leftSequenceGlobalId/size;
			int averageFragmentLength=(*m_LOADER_averageFragmentLength);
			int deviation=(*m_LOADER_deviation);

			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,leftSequenceIdOnRank);
			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,rightSequenceRank);
			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,rightSequenceIdOnRank);
			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,averageFragmentLength);
			m_disData->m_messagesStockPaired.addAt(leftSequenceRank,deviation);

			if(m_disData->m_messagesStockPaired.flush(leftSequenceRank,5,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank,false)){
				m_waitingNumber++;
			}
		}else if(m_send_sequences_done
	&& m_isInterleavedFile
			&&((*m_distribution_sequence_id)%2)==1){// only the right sequence.
			int rightSequenceGlobalId=(*m_distribution_currentSequenceId);
			int rightSequenceRank=rightSequenceGlobalId%size;
			int rightSequenceIdOnRank=rightSequenceGlobalId/size;
			int leftSequenceGlobalId=rightSequenceGlobalId-1;
			int leftSequenceRank=leftSequenceGlobalId%size;
			int leftSequenceIdOnRank=leftSequenceGlobalId/size;
			int averageFragmentLength=(*m_LOADER_averageFragmentLength);
			int deviation=(*m_LOADER_deviation);

			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,rightSequenceIdOnRank);
			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,leftSequenceRank);
			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,leftSequenceIdOnRank);
			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,averageFragmentLength);
			m_disData->m_messagesStockPaired.addAt(rightSequenceRank,deviation);

			if(m_disData->m_messagesStockPaired.flush(rightSequenceRank,5,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank,false)){
				m_waitingNumber++;
			}
		}
		if(m_send_sequences_done){
			(*m_distribution_currentSequenceId)++;
		}
		(*m_distribution_sequence_id)++;
	}
	return true;
}

SequencesLoader::SequencesLoader(){
	m_waitingNumber=0;
	m_produced=0;
	m_last=time(NULL);
}

void SequencesLoader::setReadiness(){
	m_waitingNumber--;
}

bool SequencesLoader::isReady(){
	return m_waitingNumber==0;
}

void SequencesLoader::constructor(int size){
	m_size=size;
	m_buffers=(char*)__Malloc(m_size*MPI_BTL_SM_EAGER_LIMIT*sizeof(char));
	m_entries=(int*)__Malloc(m_size*sizeof(int));
	m_numberOfSequences=(int*)__Malloc(m_size*sizeof(int));
	for(int i=0;i<m_size;i++){
		m_entries[i]=0;
		m_numberOfSequences[i]=0;
	}
	#ifdef ASSERT
	assert(m_buffers!=NULL);
	assert(m_entries!=NULL);
	#endif
}

int SequencesLoader::getSpaceLeft(int rank){
	return MPI_BTL_SM_EAGER_LIMIT-getUsedSpace(rank)-1;// -1 for the extra space for \0
}

int SequencesLoader::getUsedSpace(int rank){
	return m_entries[rank];
}

void SequencesLoader::appendSequence(int rank,char*sequence){
	char*destination=m_buffers+rank*MPI_BTL_SM_EAGER_LIMIT+m_entries[rank];
	strcpy(destination,sequence);
	m_entries[rank]+=(strlen(sequence)+1);
	#ifdef ASSERT
	assert(m_entries[rank]<=MPI_BTL_SM_EAGER_LIMIT);
	#endif
	m_numberOfSequences[rank]++;
}

/*
 * seq1: length 2
 * seq2: length 2
 * F: final 0
 *
 * 0 1 2 3 4 5 6
 * 1 1 1 2 2 2 F
 */
void SequencesLoader::flush(int rank,RingAllocator*m_outboxAllocator,StaticVector*m_outbox){
	if(m_entries[rank]==0){
		return;// nothing to flush down the toilet.
	}
	int cells=getUsedSpace(rank)+1;// + 1 for the supplementary \0
	char*message=(char*)m_outboxAllocator->allocate(cells*sizeof(char));
	int n=0;
	for(int i=0;i<m_entries[rank];i++){
		char bufferChar=m_buffers[rank*MPI_BTL_SM_EAGER_LIMIT+i];
		if(bufferChar=='\0'){
			n++;
		}
		message[i]=bufferChar;
	}
	message[cells-1]='\0';
	Message aMessage(message,MPI_BTL_SM_EAGER_LIMIT/sizeof(VERTEX_TYPE),MPI_UNSIGNED_LONG_LONG,rank,TAG_SEND_SEQUENCE_REGULATOR,rank);
	m_outbox->push_back(aMessage);
	m_entries[rank]=0;
	m_waitingNumber++;
}

void SequencesLoader::flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox){
	#ifdef ASSERT
	assert(m_size!=0);
	#endif
	for(int i=0;i<m_size;i++){
		flush(i,m_outboxAllocator,m_outbox);
	}
}
