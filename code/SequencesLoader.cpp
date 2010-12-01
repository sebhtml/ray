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
#include<SequencesLoader.h>
#include<Message.h>
#include<BubbleData.h>
#include<assert.h>
#include<vector>
#include<common_functions.h>
#include<Loader.h>
#include<Read.h>
#include<iostream>
using namespace std;

bool SequencesLoader::loadSequences(int rank,int size,
	StaticVector*m_outbox,
	RingAllocator*m_outboxAllocator,
	bool*m_loadSequenceStep,BubbleData*m_bubbleData,
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

	// we reached the end of the file.!! (processed all sequencing reads
	// case 1: it was distributing reads
	// case 2: it was distributing paired information
	//  -/-
	if(m_loader.size()>0 && m_distribution_sequence_id==(int)m_loader.size()){
		// distribution of reads is completed.
		if(!m_send_sequences_done){
			m_send_sequences_done=true;

			(m_distribution_sequence_id)=0;
			flushAll(m_outboxAllocator,m_outbox);
			
			#ifdef ASSERT
			for(int i=0;i<m_size;i++){
				assert(m_entries[i]==0);
			}
			#endif
			cout<<"Rank "<<rank<<" is assigning sequence reads "<<m_loader.size()<<"/"<<m_loader.size()<<" (completed)"<<endl;
			cout<<endl;
			cout.flush();
	
		// distribution of paired information is completed
		}else{
			m_waitingNumber+=m_bufferedData.flushAll(TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank);
			cout<<"Rank "<<rank<<" is sending paired information "<<m_loader.size()<<"/"<<m_loader.size()<<" (completed)"<<endl;
			cout<<endl;
			cout.flush();

			(m_distribution_file_id)++;  // go with the next file.

			(m_distribution_sequence_id)=0;
			m_loader.clear();  // clear the hideout of reads.
		}

	// -->  all files were processed.
	}else if((m_distribution_file_id)>(int)allFiles.size()-1){
		(*m_master_mode)=MASTER_MODE_DO_NOTHING;
		(*m_loadSequenceStep)=true;

		for(int i=0;i<size;i++){
			Message aMessage(NULL, 0, MPI_UNSIGNED_LONG_LONG, i,TAG_MASTER_IS_DONE_SENDING_ITS_SEQUENCES_TO_OTHERS,rank);
			m_outbox->push_back(aMessage);
		}

		m_loader.clear();
		m_bufferedData.clear();

	// fileID is set, but reads are not read yet.
	//
	}else if(m_loader.size()==0){
		m_send_sequences_done=false;
		printf("Rank %i is loading %s\n\n",rank,allFiles[(m_distribution_file_id)].c_str());
		fflush(stdout);
		int res=m_loader.load(allFiles[(m_distribution_file_id)],false);
		if(res==EXIT_FAILURE){
			return false;
		}
		m_parameters->setNumberOfSequences(m_loader.size());
	
		if(m_loader.size()==0){
			return false;
		}

		// write Reads in AMOS format.
		if((*m_parameters).useAmos()){
			FILE*fp=(*m_bubbleData).m_amos;
			for(int i=0;i<(int)m_loader.size();i++){
				int iid=(m_distribution_currentSequenceId)+i;
				char*seq=m_loader.at(i)->getSeq();
				char*qlt=(char*)__Malloc(strlen(seq)+1);
				strcpy(qlt,seq);
				// spec: https://sourceforge.net/apps/mediawiki/amos/index.php?title=Message_Types#Sequence_t_:_Universal_t
				for(int j=0;j<(int)strlen(qlt);j++)
					qlt[j]='D';
				fprintf(fp,"{RED\niid:%i\neid:%i\nseq:\n%s\n.\nqlt:\n%s\n.\n}\n",iid+1,iid+1,seq,qlt);
				__Free(qlt);
			}
		}

		m_isInterleavedFile=(m_LOADER_isLeftFile)=(m_LOADER_isRightFile)=false;
		if((*m_parameters).isLeftFile((m_distribution_file_id))){
			(m_LOADER_isLeftFile)=true;
			(m_LOADER_averageFragmentLength)=(*m_parameters).getFragmentLength((m_distribution_file_id));
			(m_LOADER_deviation)=(*m_parameters).getStandardDeviation((m_distribution_file_id));
		}else if((*m_parameters).isRightFile((m_distribution_file_id))){
			(m_LOADER_isRightFile)=true;
			(m_LOADER_averageFragmentLength)=(*m_parameters).getFragmentLength((m_distribution_file_id));
			(m_LOADER_deviation)=(*m_parameters).getStandardDeviation((m_distribution_file_id));
		}else if((*m_parameters).isInterleavedFile((m_distribution_file_id))){
			m_isInterleavedFile=true;
			(m_LOADER_averageFragmentLength)=(*m_parameters).getFragmentLength((m_distribution_file_id));
			(m_LOADER_deviation)=(*m_parameters).getStandardDeviation((m_distribution_file_id));
		}

	// else process a sequencing read
	}else if(!m_send_sequences_done){
		// case 1: send sequencing reads
		int destination=((m_distribution_currentSequenceId)+(m_distribution_sequence_id))%size;
		#ifdef ASSERT
		assert(destination>=0);
		assert(destination<size);
		#endif

		int theSpaceLeft=getSpaceLeft(destination);
		char*sequence=m_loader.at(m_distribution_sequence_id)->getSeq();
		int spaceNeeded=strlen(sequence)+1;
		if(spaceNeeded>theSpaceLeft){
			flush(destination,m_outboxAllocator,m_outbox,false);
		}else{
			#ifdef ASSERT
			assert(spaceNeeded<=getSpaceLeft(destination));
			#endif

			appendSequence(destination,sequence);
			(m_distribution_sequence_id)++;
		}

		if((m_distribution_sequence_id)%100000==0){
			cout<<"Rank "<<rank<<" is assigning sequence reads "<<(m_distribution_sequence_id)+1<<"/"<<m_loader.size()<<endl;
		}
	}else if(m_send_sequences_done){
		#ifdef ASSERT
		assert(m_waitingNumber==0);
		if((m_distribution_sequence_id)==0){
			for(int i=0;i<m_size;i++){
				assert(m_entries[i]==0);
			}
		}
		#endif

		if((m_distribution_sequence_id)%1000000==0){
			cout<<"Rank "<<rank<<" is sending paired information "<<(m_distribution_sequence_id)+1<<"/"<<m_loader.size()<<endl;
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
		
		// right sequence in a right file, obviously

		// left sequence in a left file, obviously
		if(false && m_LOADER_isLeftFile){
			int leftSequenceGlobalId=m_distribution_currentSequenceId;
			int leftSequenceRank=leftSequenceGlobalId%size;
			int leftSequenceIdOnRank=leftSequenceGlobalId/size;

			#ifdef ASSERT
			assert(m_loader.size()!=0);
			#endif

			int rightSequenceGlobalId=leftSequenceGlobalId+m_loader.size();

			#ifdef ASSERT
			assert(leftSequenceGlobalId<rightSequenceGlobalId);
			#endif

			int rightSequenceRank=rightSequenceGlobalId%size;

			#ifdef ASSERT
			assert(rightSequenceRank<size);
			#endif

			int rightSequenceIdOnRank=rightSequenceGlobalId/size;

			#ifdef ASSERT
		
			if(leftSequenceIdOnRank>=m_numberOfSequences[leftSequenceRank]){
				cout<<"leftSequenceIdOnRank="<<leftSequenceIdOnRank<<" but size="<<m_numberOfSequences[leftSequenceRank]<<endl;
			}
			assert(leftSequenceIdOnRank<m_numberOfSequences[leftSequenceRank]);
			#endif

			m_bufferedData.addAt(leftSequenceRank,leftSequenceIdOnRank);
			m_bufferedData.addAt(leftSequenceRank,rightSequenceRank);
			m_bufferedData.addAt(leftSequenceRank,rightSequenceIdOnRank);
			m_bufferedData.addAt(leftSequenceRank,m_parameters->getLibrary(m_distribution_file_id));

			if(m_bufferedData.flush(leftSequenceRank,4,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank,false)){
				m_waitingNumber++;
			}
		}else if(m_LOADER_isRightFile){

			#ifdef ASSERT
			assert(m_loader.size()!=0);
			#endif

			int rightSequenceGlobalId=(m_distribution_currentSequenceId);
			int rightSequenceRank=rightSequenceGlobalId%size;
			int rightSequenceIdOnRank=rightSequenceGlobalId/size;
			int leftSequenceGlobalId=rightSequenceGlobalId-m_loader.size();

			#ifdef ASSERT
			if(rightSequenceIdOnRank>=m_numberOfSequences[rightSequenceRank]){
				cout<<"rightSequenceIdOnRank="<<rightSequenceIdOnRank<<" but size="<<m_numberOfSequences[rightSequenceRank]<<" rank is "<<rightSequenceRank<<endl;
			}
			assert(rightSequenceIdOnRank<m_numberOfSequences[rightSequenceRank]);
			#endif

			int leftSequenceRank=leftSequenceGlobalId%size;
			int leftSequenceIdOnRank=leftSequenceGlobalId/size;

			m_bufferedData.addAt(rightSequenceRank,rightSequenceIdOnRank);
			m_bufferedData.addAt(rightSequenceRank,leftSequenceRank);
			m_bufferedData.addAt(rightSequenceRank,leftSequenceIdOnRank);
			m_bufferedData.addAt(rightSequenceRank,m_parameters->getLibrary(m_distribution_file_id));

			if(m_bufferedData.flush(rightSequenceRank,4,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank,false)){
				m_waitingNumber++;
			}
			#ifdef DEBUG
			assert(deviation!=0);
			assert(averageFragmentLength>=0);
			#endif

		// left sequence in interleaved file
		}else if(false && m_isInterleavedFile && ((m_distribution_sequence_id)%2)==0){
			int rightSequenceGlobalId=(m_distribution_currentSequenceId)+1;
			int rightSequenceRank=rightSequenceGlobalId%size;
			int rightSequenceIdOnRank=rightSequenceGlobalId/size;

			int leftSequenceGlobalId=rightSequenceGlobalId-1;
			int leftSequenceRank=leftSequenceGlobalId%size;
			int leftSequenceIdOnRank=leftSequenceGlobalId/size;

			#ifdef DEBUG
			assert(deviation!=0);
			assert(averageFragmentLength>=0);
			#endif

			m_bufferedData.addAt(leftSequenceRank,leftSequenceIdOnRank);
			m_bufferedData.addAt(leftSequenceRank,rightSequenceRank);
			m_bufferedData.addAt(leftSequenceRank,rightSequenceIdOnRank);
			m_bufferedData.addAt(leftSequenceRank,m_parameters->getLibrary(m_distribution_file_id));

			if(m_bufferedData.flush(leftSequenceRank,4,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank,false)){
				m_waitingNumber++;
			}

		// only the right sequence.
		}else if(m_isInterleavedFile &&((m_distribution_sequence_id)%2)==1){
			int rightSequenceGlobalId=(m_distribution_currentSequenceId);
			int rightSequenceRank=rightSequenceGlobalId%size;
			int rightSequenceIdOnRank=rightSequenceGlobalId/size;
			int leftSequenceGlobalId=rightSequenceGlobalId-1;
			int leftSequenceRank=leftSequenceGlobalId%size;
			int leftSequenceIdOnRank=leftSequenceGlobalId/size;

			m_bufferedData.addAt(rightSequenceRank,rightSequenceIdOnRank);
			m_bufferedData.addAt(rightSequenceRank,leftSequenceRank);
			m_bufferedData.addAt(rightSequenceRank,leftSequenceIdOnRank);
			m_bufferedData.addAt(rightSequenceRank,m_parameters->getLibrary(m_distribution_file_id));

			if(m_bufferedData.flush(rightSequenceRank,4,TAG_INDEX_PAIRED_SEQUENCE,m_outboxAllocator,m_outbox,rank,false)){
				m_waitingNumber++;
			}

			#ifdef DEBUG
			assert(deviation!=0);
			assert(averageFragmentLength>=0);
			#endif

		}
		(m_distribution_currentSequenceId)++;
		(m_distribution_sequence_id)++;
	}
	return true;
}

SequencesLoader::SequencesLoader(){
	m_distribution_file_id=m_distribution_sequence_id=m_distribution_currentSequenceId=0;
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
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_size=size;
	m_buffers=(char*)__Malloc(m_size*MAXIMUM_MESSAGE_SIZE_IN_BYTES*sizeof(char));
	m_entries=(int*)__Malloc(m_size*sizeof(int));
	#ifdef ASSERT
	m_numberOfSequences=(int*)__Malloc(m_size*sizeof(int));
	m_numberOfFlushedSequences=(int*)__Malloc(m_size*sizeof(int));
	#endif
	for(int i=0;i<m_size;i++){
		m_entries[i]=0;
		#ifdef ASSERT
		m_numberOfSequences[i]=0;
		m_numberOfFlushedSequences[i]=0;
		#endif
	}
	#ifdef ASSERT
	assert(m_buffers!=NULL);
	assert(m_entries!=NULL);
	#endif
}

int SequencesLoader::getSpaceLeft(int rank){
	return MAXIMUM_MESSAGE_SIZE_IN_BYTES-getUsedSpace(rank)-1;// -1 for the extra space for \0
}

int SequencesLoader::getUsedSpace(int rank){
	return m_entries[rank];
}

void SequencesLoader::appendSequence(int rank,char*sequence){
	char*destination=m_buffers+rank*MAXIMUM_MESSAGE_SIZE_IN_BYTES+m_entries[rank];
	strcpy(destination,sequence);
	m_entries[rank]+=(strlen(sequence)+1);
	#ifdef ASSERT
	assert(m_entries[rank]<=MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	#endif
	#ifdef ASSERT
	m_numberOfSequences[rank]++;
	#endif
}

/*
 * seq1: length 2
 * seq2: length 2
 * F: final 0
 *
 * 0 1 2 3 4 5 6
 * 1 1 1 2 2 2 F
 */
void SequencesLoader::flush(int rank,RingAllocator*m_outboxAllocator,StaticVector*m_outbox,bool forceNothing){
	if(m_entries[rank]==0 && !forceNothing){
		return;
	}
	int cells=getUsedSpace(rank)+1;// + 1 for the supplementary \0
	char*message=(char*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES*sizeof(char));
	#ifdef ASSERT
	int n=0;
	#endif
	for(int i=0;i<m_entries[rank];i++){
		char bufferChar=m_buffers[rank*MAXIMUM_MESSAGE_SIZE_IN_BYTES+i];
		#ifdef ASSERT
		if(bufferChar=='\0'){
			n++;
		}
		#endif
		message[i]=bufferChar;
	}
	#ifdef ASSERT
	assert(n>0);
	m_numberOfFlushedSequences[rank]+=n;
	#endif
	//cout<<"sending "<<n<<" sequences to "<<rank<<endl;
	message[cells-1]=ASCII_END_OF_TRANSMISSION;
	Message aMessage(message,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(VERTEX_TYPE),MPI_UNSIGNED_LONG_LONG,rank,TAG_SEND_SEQUENCE_REGULATOR,rank);
	m_outbox->push_back(aMessage);
	m_entries[rank]=0;
	m_waitingNumber++;
}

void SequencesLoader::flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox){
	#ifdef ASSERT
	assert(m_size!=0);
	#endif
	for(int i=0;i<m_size;i++){
		flush(i,m_outboxAllocator,m_outbox,false);
	}
}
