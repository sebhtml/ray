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

void SequencesLoader::registerSequence(){
	if(m_myReads->size()%100000==0){
		int amount=m_myReads->size();
		printf("Rank %i has %i sequence reads\n",m_rank,amount);
		fflush(stdout);
	}
	#ifdef ASSERT
	assert(m_distribution_sequence_id<m_loader.size());
	#endif

	Read*theRead=m_loader.at(m_distribution_sequence_id);
	char*read=theRead->getSeq();

	Read myRead;
	myRead.copy(NULL,read,&(*m_persistentAllocator),true);
	m_myReads->push_back(&myRead);

	if(false && m_LOADER_isLeftFile){
		int leftSequenceGlobalId=m_distribution_currentSequenceId;
		int leftSequenceIdOnRank=leftSequenceGlobalId/m_size;

		#ifdef ASSERT
		assert(m_loader.size()!=0);
		#endif

		int rightSequenceGlobalId=leftSequenceGlobalId+m_loader.size();

		#ifdef ASSERT
		assert(leftSequenceGlobalId<rightSequenceGlobalId);
		#endif

		int rightSequenceRank=rightSequenceGlobalId%m_size;

		#ifdef ASSERT
		assert(rightSequenceRank<m_size);
		#endif

		int rightSequenceIdOnRank=rightSequenceGlobalId/m_size;

		int library=m_parameters->getLibrary(m_distribution_file_id);

		PairedRead*t=(PairedRead*)(*m_persistentAllocator).allocate(sizeof(PairedRead));
		t->constructor(rightSequenceRank,rightSequenceIdOnRank,library);
		(*m_myReads)[leftSequenceIdOnRank]->setPairedRead(t);
	}else if(m_LOADER_isRightFile){

		#ifdef ASSERT
		assert(m_loader.size()!=0);
		#endif

		int rightSequenceGlobalId=(m_distribution_currentSequenceId);
		int rightSequenceIdOnRank=rightSequenceGlobalId/m_size;
		int leftSequenceGlobalId=rightSequenceGlobalId-m_loader.size();

		int leftSequenceRank=leftSequenceGlobalId%m_size;
		int leftSequenceIdOnRank=leftSequenceGlobalId/m_size;
		int library=m_parameters->getLibrary(m_distribution_file_id);

		#ifdef DEBUG
		assert(deviation!=0);
		assert(averageFragmentLength>=0);
		#endif

		PairedRead*t=(PairedRead*)(*m_persistentAllocator).allocate(sizeof(PairedRead));
		t->constructor(leftSequenceRank,leftSequenceIdOnRank,library);
		(*m_myReads)[rightSequenceIdOnRank]->setPairedRead(t);

	// left sequence in interleaved file
	}else if(false && m_isInterleavedFile && ((m_distribution_sequence_id)%2)==0){
		int rightSequenceGlobalId=(m_distribution_currentSequenceId)+1;
		int rightSequenceRank=rightSequenceGlobalId%m_size;
		int rightSequenceIdOnRank=rightSequenceGlobalId/m_size;

		int leftSequenceGlobalId=rightSequenceGlobalId-1;
		int leftSequenceIdOnRank=leftSequenceGlobalId/m_size;

		#ifdef DEBUG
		assert(deviation!=0);
		assert(averageFragmentLength>=0);
		#endif

		int library=m_parameters->getLibrary(m_distribution_file_id);

		PairedRead*t=(PairedRead*)(*m_persistentAllocator).allocate(sizeof(PairedRead));
		t->constructor(rightSequenceRank,rightSequenceIdOnRank,library);
		(*m_myReads)[leftSequenceIdOnRank]->setPairedRead(t);

	// only the right sequence.
	}else if(m_isInterleavedFile &&((m_distribution_sequence_id)%2)==1){
		int rightSequenceGlobalId=(m_distribution_currentSequenceId);
		int rightSequenceIdOnRank=rightSequenceGlobalId/m_size;
		int leftSequenceGlobalId=rightSequenceGlobalId-1;
		int leftSequenceRank=leftSequenceGlobalId%m_size;
		int leftSequenceIdOnRank=leftSequenceGlobalId/m_size;
		int library=m_parameters->getLibrary(m_distribution_file_id);

		#ifdef DEBUG
		assert(deviation!=0);
		assert(averageFragmentLength>=0);
		#endif

		PairedRead*t=(PairedRead*)(*m_persistentAllocator).allocate(sizeof(PairedRead));
		t->constructor(leftSequenceRank,leftSequenceIdOnRank,library);
		(*m_myReads)[rightSequenceIdOnRank]->setPairedRead(t);
	}
}

bool SequencesLoader::loadSequences(int rank,int size,
	StaticVector*m_outbox,
	RingAllocator*m_outboxAllocator,
	bool*m_loadSequenceStep,BubbleData*m_bubbleData,
	time_t*m_lastTime,
	Parameters*m_parameters,int*m_master_mode,int*m_mode
){
	m_rank=rank;

	this->m_parameters=m_parameters;

	// count the number of sequences in all files.
	vector<string> allFiles=(*m_parameters).getAllFiles();
	
	m_distribution_currentSequenceId=0;
	int files=allFiles.size();
	for(m_distribution_file_id=0;m_distribution_file_id<(int)allFiles.size();
		m_distribution_file_id++){

		printf("Rank %i is loading %s [%i/%i]\n",m_rank,allFiles.at(m_distribution_file_id).c_str(),
			m_distribution_file_id+1,files);

		int res=m_loader.load(allFiles[(m_distribution_file_id)],false);
		if(res==EXIT_FAILURE){
			return false;
		}
		m_parameters->setNumberOfSequences(m_loader.size());
	
		if(m_loader.size()==0){
			return false;
		}

		#ifdef ASSERT
		assert(m_loader.size()!=0);
		#endif

		// write Reads in AMOS format.
		if(rank==MASTER_RANK&&(*m_parameters).useAmos()){
			FILE*fp=(*m_bubbleData).m_amos;
			char qlt[20000];
			for(int i=0;i<(int)m_loader.size();i++){
				int iid=m_distribution_currentSequenceId+i;
				char*seq=m_loader.at(i)->getSeq();
				#ifdef ASSERT
				assert(seq!=NULL);
				#endif
				strcpy(qlt,seq);
				// spec: https://sourceforge.net/apps/mediawiki/amos/index.php?title=Message_Types#Sequence_t_:_Universal_t
				for(int j=0;j<(int)strlen(qlt);j++){
					qlt[j]='D';
				}
				fprintf(fp,"{RED\niid:%i\neid:%i\nseq:\n%s\n.\nqlt:\n%s\n.\n}\n",iid+1,iid+1,seq,qlt);
			}
			m_loader.clear();
			m_loader.load(allFiles[(m_distribution_file_id)],false);
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

		for(m_distribution_sequence_id=0;
			m_distribution_sequence_id<m_loader.size();
				m_distribution_sequence_id++){

			if(m_distribution_currentSequenceId%m_size==m_rank){
				registerSequence();
			}
			m_distribution_currentSequenceId++;
		}
		m_loader.clear();
	}
	
	Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_SEQUENCES_READY,rank);
	m_outbox->push_back(aMessage);

	(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;

	int amount=m_myReads->size();
	printf("Rank %i has %i sequence reads (completed)\n",m_rank,amount);
	fflush(stdout);

	return true;
}

void SequencesLoader::constructor(int size,MyAllocator*allocator,ArrayOfReads*reads){
	m_size=size;
	m_persistentAllocator=allocator;
	m_myReads=reads;
}
