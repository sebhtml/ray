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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#include <LibraryWorker.h>
#include <Message.h>
#include <SeedingData.h>
#include <mpi.h>
#include <VirtualCommunicator.h>

void LibraryWorker::constructor(int id,SeedingData*seedingData,VirtualCommunicator*virtualCommunicator){
	m_virtualCommunicator=virtualCommunicator;
	m_currentPosition=0;
	m_id=id;
	m_seedingData=seedingData;
	m_readsRequested=false;
}

bool LibraryWorker::isDone(){
	return m_done;
}

void LibraryWorker::work(){
	if(m_currentPosition==(int)m_seedingData->m_SEEDING_seeds[m_id].size()){
		m_done=true;
	}else{
		if(!m_readsRequested){
			m_readsRequested=true;
			uint64_t vertex=m_seedingData->m_SEEDING_seeds[m_id][m__currentPosition];
			m_readFetcher.constructor(vertex,m_outboxAllocator,m_inbox,m_outbox,m_parameters);
			m_edgeIterator=0;// iterate over reads
			m_hasPairedReadRequested=false;
		}else if(!m_readFetcher.isDone()){
			m_readFetcher.work();
		}else{
			if(m_edgeIterator<(int)m_readFetcher.getResult()->size()){
				ReadAnnotation annotation=m_readFetcher.getResult()->at(m_edgeIterator);

				int rightRead=annotation.getReadIndex();
				#ifdef ASSERT_AUTO
				uint64_t rightReadUniqueId=annotation.getUniqueId();
				#endif
				if(!m_hasPairedReadRequested){
					uint64_t*message=(uint64_t*)(m_outboxAllocator)->allocate(1*sizeof(uint64_t));
					message[0]=rightRead;
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),RAY_MPI_TAG_HAS_PAIRED_READ,getRank());
					
					(m_outbox)->push_back(aMessage);
					m_hasPairedReadRequested=true;
					m_hasPairedReadReceived=false;
					m_readLength_requested=false;
				}else if(m_hasPairedReadReceived){
					if(m_EXTENSION_hasPairedReadAnswer){
						if(!m_EXTENSION_readLength_requested){
							m_EXTENSION_readLength_requested=true;
							m_EXTENSION_readLength_received=false;
							uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(uint64_t));
							m_EXTENSION_pairedSequenceRequested=false;
							message[0]=rightRead;
							Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),RAY_MPI_TAG_ASK_READ_LENGTH,getRank());
							m_outbox->push_back(aMessage);
						}else if(m_ed->m_EXTENSION_readLength_received){
							if(!m_ed->m_EXTENSION_pairedSequenceRequested){
								m_ed->m_EXTENSION_pairedSequenceReceived=false;
								m_ed->m_EXTENSION_pairedSequenceRequested=true;
								uint64_t*message=(uint64_t*)(m_outboxAllocator)->allocate(1*sizeof(uint64_t));
								message[0]=rightRead;
								Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),RAY_MPI_TAG_GET_PAIRED_READ,getRank());
								(m_outbox)->push_back(aMessage);
							}else if(m_ed->m_EXTENSION_pairedSequenceReceived){
								int library=m_ed->m_EXTENSION_pairedRead.getLibrary();
								bool isAutomatic=m_parameters->isAutomatic(library);
								if(isAutomatic){
									uint64_t uniqueReadIdentifier=m_ed->m_EXTENSION_pairedRead.getUniqueId();
									if((*m_readsPositions).count(uniqueReadIdentifier)>0){
										int library=m_ed->m_EXTENSION_pairedRead.getLibrary();
										int rightStrandPosition=annotation.getPositionOnStrand();
										char rightStrand=annotation.getStrand();
										char leftStrand=m_readsStrands[uniqueReadIdentifier];
										int leftStrandPosition=m_strandPositions[uniqueReadIdentifier];
											
										if(( leftStrand=='F' && rightStrand=='R' )
										||(  leftStrand=='R' && rightStrand=='F' )){// make sure the orientation is OK
											int p1=(*m_readsPositions)[uniqueReadIdentifier];
											
										
											int p2=m_ed->m_EXTENSION_currentPosition;
											int d=p2-p1+m_ed->m_EXTENSION_receivedLength+leftStrandPosition-rightStrandPosition;
											//cout<<"d="<<d<<" lId="<<annotation.getUniqueId()<<" rId="<<uniqueReadIdentifier<<" pLeft="<<p1<<" pRight="<<p2<<" lStrand="<<leftStrand<<" rStrand="<<rightStrand<<" leftStrandPos="<<leftStrandPosition<<" rightStrandPos="<<rightStrandPosition<<" RightLength="<<m_ed->m_EXTENSION_receivedLength<<endl;
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
				for(int i=0;i<(int)m_readFetcher.getResult()->size();i++){
					ReadAnnotation a=m_readFetcher.getResult()->at(i);
					uint64_t uniqueId=a.getUniqueId();
					int position=m_ed->m_EXTENSION_currentPosition;
					char strand=a.getStrand();
					int strandPosition=a.getPositionOnStrand();
					// read, position, strand
					(*m_readsPositions)[uniqueId]=position;
					m_readsStrands[uniqueId]=strand;
					m_strandPositions[uniqueId]=strandPosition;
					//cout<<"Read Id="<<uniqueId<<" Strand="<<strand<<" StrandPosition="<<strandPosition<<" PositionOnSeed="<<position<<endl;
				}

				m_ed->m_EXTENSION_currentPosition++;
				m_ed->m_EXTENSION_reads_requested=false;
			}
		}
	}
}
