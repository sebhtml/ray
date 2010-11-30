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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#include<ReadAnnotation.h>
#include<Library.h>
#include<common_functions.h>
#include<assert.h>
#include<Parameters.h>

void Library::updateDistances(){
	if(!m_ready){
		return;
	}

	// we are done.
	if((*m_fileId)==(*m_parameters).getNumberOfFiles()){
		// flush
		m_bufferedData->flushAll(TAG_UPDATE_LIBRARY_INFORMATION,m_outboxAllocator,m_outbox,getRank());

		m_timePrinter->printElapsedTime("Computation of library sizes");
		cout<<endl;
		cout<<"Rank 0 asks others to extend their seeds."<<endl;
		#ifndef SHOW_PROGRESS
		cout<<"\r"<<"Extending seeds"<<endl;
		#endif
		(*m_master_mode)=MASTER_MODE_TRIGGER_EXTENSIONS;
		m_ed->m_EXTENSION_rank=-1;
		m_ed->m_EXTENSION_currentRankIsSet=false;

	// skip the file.
	}else if(!(*m_parameters).isAutomatic(*m_fileId) ||m_parameters->isLeftFile(*m_fileId)){
		(*m_fileId)++;
		(*m_sequence_id)+=(*m_parameters).getNumberOfSequences(*m_fileId);
	// the file is finished loading.
	}else if((*m_sequence_idInFile)==(*m_parameters).getNumberOfSequences(*m_fileId)){
		cout<<"Rank 0 is updating library lengths "<<(*m_parameters).getNumberOfSequences(*m_fileId)<<"/"<<(*m_parameters).getNumberOfSequences(*m_fileId)<<" (completed)"<<endl;
		(*m_fileId)++;
		(*m_sequence_idInFile)=0;
	// we can process the sequence.
	// the file is set to be automatic.
	}else{
		int sequenceRank=(*m_sequence_id)%getSize();
		int sequenceIndex=(*m_sequence_id)/getSize();

		if((*m_sequence_idInFile)%1000000==0){
			cout<<"Rank 0 is updating library lengths "<<(*m_sequence_idInFile)+1<<"/"<<(*m_parameters).getNumberOfSequences(*m_fileId)<<endl;
		}

		if(m_parameters->isRightFile(*m_fileId) || (m_parameters->isInterleavedFile(*m_fileId) &&(*m_sequence_idInFile)%2==1)){
			int library=(*m_parameters).getLibrary(*m_fileId);
			int averageLength=(*m_parameters).getObservedAverageDistance(library);
			int standardDeviation=(*m_parameters).getObservedStandardDeviation(library);

			m_bufferedData->addAt(sequenceRank,sequenceIndex);
			m_bufferedData->addAt(sequenceRank,averageLength);
			m_bufferedData->addAt(sequenceRank,standardDeviation);

			if(m_bufferedData->flush(sequenceRank,3,TAG_UPDATE_LIBRARY_INFORMATION,m_outboxAllocator,m_outbox,getRank(),false)){
				m_ready=false;
			}
		}

		(*m_sequence_id)++;
		(*m_sequence_idInFile)++;
	}
}

void Library::detectDistances(){
	if(m_seedingData->m_SEEDING_i==(int)m_seedingData->m_SEEDING_seeds.size()){
		printf("Rank %i detected %i library lengths\n",getRank(),m_detectedDistances);
		fflush(stdout);
		printf("Rank %i is calculating library lengths %i/%i (completed)\n",getRank(),(int)m_seedingData->m_SEEDING_seeds.size(),(int)m_seedingData->m_SEEDING_seeds.size());
		fflush(stdout);
		
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE,getRank());
		m_outbox->push_back(aMessage);
		(*m_mode)=MODE_DO_NOTHING;
	}else if(m_ed->m_EXTENSION_currentPosition==(int)m_seedingData->m_SEEDING_seeds[m_seedingData->m_SEEDING_i].size()){
		m_ed->m_EXTENSION_currentPosition=0;
		m_seedingData->m_SEEDING_i++;
		(*m_readsPositions).clear();
		m_readsStrands.clear();
		#ifdef ASSERT
		assert((*m_readsPositions).size()==0);
		#endif
	}else{
		if(!m_ed->m_EXTENSION_reads_requested){
			if(m_ed->m_EXTENSION_currentPosition==0 && m_seedingData->m_SEEDING_i%30==0){
				printf("Rank %i is calculating library lengths %i/%i (completed)\n",getRank(),(int)m_seedingData->m_SEEDING_i+1,(int)m_seedingData->m_SEEDING_seeds.size());
				fflush(stdout);
			}
			m_ed->m_EXTENSION_reads_requested=true;
			m_ed->m_EXTENSION_reads_received=false;
			VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
			#ifdef ASSERT
			assert(m_ed->m_EXTENSION_currentPosition<(int)m_seedingData->m_SEEDING_seeds[m_seedingData->m_SEEDING_i].size());
			#endif
			VERTEX_TYPE vertex=m_seedingData->m_SEEDING_seeds[m_seedingData->m_SEEDING_i][m_ed->m_EXTENSION_currentPosition];
			message[0]=vertex;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],getSize()),TAG_REQUEST_READS,getRank());
			m_outbox->push_back(aMessage);
			m_ed->m_EXTENSION_edgeIterator=0;// iterate over reads
			m_ed->m_EXTENSION_hasPairedReadRequested=false;
		}else if(m_ed->m_EXTENSION_reads_received){
			if(m_ed->m_EXTENSION_edgeIterator<(int)m_ed->m_EXTENSION_receivedReads.size()){
				ReadAnnotation annotation=m_ed->m_EXTENSION_receivedReads[m_ed->m_EXTENSION_edgeIterator];
				int rightRead=annotation.getReadIndex();
				#ifdef ASSERT_AUTO
				u64 rightReadUniqueId=annotation.getUniqueId();
				#endif
				if(!m_ed->m_EXTENSION_hasPairedReadRequested){
					VERTEX_TYPE*message=(VERTEX_TYPE*)(m_outboxAllocator)->allocate(1*sizeof(VERTEX_TYPE));
					message[0]=rightRead;
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_HAS_PAIRED_READ,getRank());
					(m_outbox)->push_back(aMessage);
					m_ed->m_EXTENSION_hasPairedReadRequested=true;
					m_ed->m_EXTENSION_hasPairedReadReceived=false;
					m_ed->m_EXTENSION_readLength_requested=false;
				}else if(m_ed->m_EXTENSION_hasPairedReadReceived){
					if(m_ed->m_EXTENSION_hasPairedReadAnswer){
						if(!m_ed->m_EXTENSION_readLength_requested){
							m_ed->m_EXTENSION_readLength_requested=true;
							m_ed->m_EXTENSION_readLength_received=false;
							VERTEX_TYPE*message=(VERTEX_TYPE*)m_outboxAllocator->allocate(1*sizeof(VERTEX_TYPE));
							m_ed->m_EXTENSION_pairedSequenceRequested=false;
							message[0]=rightRead;
							Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_ASK_READ_LENGTH,getRank());
							m_outbox->push_back(aMessage);
						}else if(m_ed->m_EXTENSION_readLength_received){
							if(!m_ed->m_EXTENSION_pairedSequenceRequested){
								m_ed->m_EXTENSION_pairedSequenceReceived=false;
								m_ed->m_EXTENSION_pairedSequenceRequested=true;
								VERTEX_TYPE*message=(VERTEX_TYPE*)(m_outboxAllocator)->allocate(1*sizeof(VERTEX_TYPE));
								message[0]=rightRead;
								Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_GET_PAIRED_READ,getRank());
								(m_outbox)->push_back(aMessage);
							}else if(m_ed->m_EXTENSION_pairedSequenceReceived){
								int expectedDeviation=m_ed->m_EXTENSION_pairedRead.getStandardDeviation();
								if(expectedDeviation==_AUTOMATIC_DETECTION){
									u64 uniqueReadIdentifier=m_ed->m_EXTENSION_pairedRead.getUniqueId();
									if((*m_readsPositions).count(uniqueReadIdentifier)>0){
										int library=m_ed->m_EXTENSION_pairedRead.getAverageFragmentLength();
										char rightStrand=annotation.getStrand();
										char leftStrand=m_readsStrands[uniqueReadIdentifier];
											
										if((rightStrand=='R' && leftStrand=='F')
										||(rightStrand=='F' && leftStrand=='R')){// make sure the orientation is OK
											int p1=(*m_readsPositions)[uniqueReadIdentifier];
											
										
											int p2=m_ed->m_EXTENSION_currentPosition;
											int d=p2-p1+m_ed->m_EXTENSION_receivedLength;
											(*m_libraryDistances)[library][d]++;
											m_detectedDistances++;
										}
									}else{
										#ifdef ASSERT_AUTO
										cout<<"Pair was not found."<<endl;
										#endif
									}
								}
							
								m_ed->m_EXTENSION_edgeIterator++;
								m_ed->m_EXTENSION_hasPairedReadRequested=false;
							}
						}
					}else{
						m_ed->m_EXTENSION_edgeIterator++;
						m_ed->m_EXTENSION_hasPairedReadRequested=false;
					}
				}
			}else{
				#ifdef ASSERT_AUTO
				cout<<"Adding reads in positions "<<m_ed->m_EXTENSION_currentPosition<<endl;
				#endif
				for(int i=0;i<(int)m_ed->m_EXTENSION_receivedReads.size();i++){
					u64 uniqueId=m_ed->m_EXTENSION_receivedReads[i].getUniqueId();
					int position=m_ed->m_EXTENSION_currentPosition;
					char strand=m_ed->m_EXTENSION_receivedReads[i].getStrand();
					// read, position, strand
					(*m_readsPositions)[uniqueId]=position;
					m_readsStrands[uniqueId]=strand;
				}

				m_ed->m_EXTENSION_currentPosition++;
				m_ed->m_EXTENSION_reads_requested=false;
			}
		}
	}
}

void Library::constructor(int m_rank,StaticVector*m_outbox,RingAllocator*m_outboxAllocator,BufferedData*m_bufferedData,int*m_sequence_id,int*m_sequence_idInFile,ExtensionData*m_ed,
map<u64,int >*m_readsPositions,int m_size,
TimePrinter*m_timePrinter,int*m_mode,int*m_master_mode,
Parameters*m_parameters,int*m_fileId,SeedingData*m_seedingData,map<int,map<int,int> >*m_libraryDistances
){
	this->m_rank=m_rank;
	this->m_outbox=m_outbox;
	this->m_outboxAllocator=m_outboxAllocator;
	#ifdef ASSERT
	assert(this->m_outboxAllocator!=NULL);
	#endif
	this->m_bufferedData=m_bufferedData;
	this->m_sequence_id=m_sequence_id;
	this->m_sequence_idInFile=m_sequence_idInFile;
	this->m_ed=m_ed;
	this->m_readsPositions=m_readsPositions;
	this->m_size=m_size;
	this->m_timePrinter=m_timePrinter;
	this->m_mode=m_mode;
	this->m_master_mode=m_master_mode;
	this->m_parameters=m_parameters;
	this->m_fileId=m_fileId;
	this->m_seedingData=m_seedingData;
	this->m_libraryDistances=m_libraryDistances;
	setReadiness();
}

void Library::setReadiness(){
	m_ready=true;
}

int Library::getRank(){
	return m_rank;
}

int Library::getSize(){
	return m_size;
}

Library::Library(){
	m_detectedDistances=0;
}
