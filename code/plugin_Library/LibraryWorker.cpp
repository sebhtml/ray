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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <application_core/constants.h>
#include <plugin_Library/LibraryWorker.h>
#include <communication/Message.h>
#include <plugin_SeedingData/SeedingData.h>
#include <map>
#include <assert.h>
#include <communication/VirtualCommunicator.h>
using namespace std;

bool LibraryWorker::isDone(){
	return m_done;
}

void LibraryWorker::constructor(WorkerHandle id,SeedingData*seedingData,VirtualCommunicator*virtualCommunicator,RingAllocator*outboxAllocator,Parameters*parameters,
StaticVector*inbox,StaticVector*outbox,	map<int,map<int,int> >*libraryDistances,int*detectedDistances,MyAllocator*allocator,
MessageTag RAY_MPI_TAG_GET_READ_MATE,
MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS
){

	this->RAY_MPI_TAG_GET_READ_MATE=RAY_MPI_TAG_GET_READ_MATE;
	this->RAY_MPI_TAG_REQUEST_VERTEX_READS=RAY_MPI_TAG_REQUEST_VERTEX_READS;

	m_done=false;
	m_parameters=parameters;
	m_SEEDING_i=id;
	m_seedingData=seedingData;
	m_virtualCommunicator=virtualCommunicator;
	m_outboxAllocator=outboxAllocator;
	m_EXTENSION_currentPosition=0;
	m_EXTENSION_reads_requested=false;
	m_EXTENSION_hasPairedReadRequested=false;
	m_EXTENSION_edgeIterator=0;
	m_inbox=inbox;
	m_outbox=outbox;
	m_libraryDistances=libraryDistances;
	m_detectedDistances=detectedDistances;

	m_allocator=allocator;
	m_database.constructor();
}

void LibraryWorker::work(){
	if(m_done){
		return;
	}
	#ifdef ASSERT
	assert(m_SEEDING_i<m_seedingData->m_SEEDING_seeds.size());
	#endif
	if(m_EXTENSION_currentPosition==(int)m_seedingData->m_SEEDING_seeds[m_SEEDING_i].size()){
		while(m_database.size()>0){
			SplayNode<ReadHandle,LibraryElement>*node=m_database.getRoot();
			ReadHandle key=node->getKey();
			m_database.remove(key,true,m_allocator);
		}
		m_done=true;
	}else{
		if(!m_EXTENSION_reads_requested){
			m_EXTENSION_reads_requested=true;
			#ifdef ASSERT
			assert(m_seedingData!=NULL);
			assert(m_EXTENSION_currentPosition<(int)m_seedingData->m_SEEDING_seeds[m_SEEDING_i].size());
			#endif
			Kmer*vertex=m_seedingData->m_SEEDING_seeds[m_SEEDING_i].at(m_EXTENSION_currentPosition);
		
			m_readFetcher.constructor(vertex,m_outboxAllocator,m_inbox,m_outbox,m_parameters,m_virtualCommunicator,
				m_SEEDING_i,RAY_MPI_TAG_REQUEST_VERTEX_READS);
			#ifdef ASSERT
			assert(!m_readFetcher.isDone());
			#endif
			m_EXTENSION_edgeIterator=0;// iterate over reads
			m_EXTENSION_hasPairedReadRequested=false;
		}else if(!m_readFetcher.isDone()){
			m_readFetcher.work();
		}else{
			if(m_EXTENSION_edgeIterator<(int)m_readFetcher.getResult()->size()){
				ReadAnnotation annotation=m_readFetcher.getResult()->at(m_EXTENSION_edgeIterator);
				int rightRead=annotation.getReadIndex();
				if(!m_EXTENSION_hasPairedReadRequested){
					MessageUnit*message=(MessageUnit*)(m_outboxAllocator)->allocate(1*sizeof(MessageUnit));
					message[0]=rightRead;
					#ifdef ASSERT
					assert(m_parameters!=NULL);
					if(!(annotation.getRank()<m_parameters->getSize())){
						cout<<"Error rank="<<annotation.getRank()<<" size="<<m_parameters->getSize()<<endl;
					}
					assert(annotation.getRank()<m_parameters->getSize());
					#endif
					Message aMessage(message,1,annotation.getRank(),RAY_MPI_TAG_GET_READ_MATE,m_parameters->getRank());
					m_virtualCommunicator->pushMessage(m_SEEDING_i,&aMessage);
					m_EXTENSION_hasPairedReadRequested=true;
				}else if(m_virtualCommunicator->isMessageProcessed(m_SEEDING_i)){
					vector<MessageUnit> buffer;
					m_virtualCommunicator->getMessageResponseElements(m_SEEDING_i,&buffer);
					#ifdef ASSERT
					assert((int)buffer.size()==4);
					#endif
	
					/** this is a sentinel value */
					/** data: readLength, rank, id, library */
					if((int)buffer[1] != MAX_NUMBER_OF_MPI_PROCESSES){
						int library=buffer[3];
						int readLength=buffer[0];
						bool isAutomatic=m_parameters->isAutomatic(library);
						if(isAutomatic){
							PathHandle uniqueReadIdentifier=getPathUniqueId(buffer[1],buffer[2]);
							SplayNode<ReadHandle,LibraryElement>*node=m_database.find(uniqueReadIdentifier,false);
							if(node!=NULL){
								LibraryElement*element=node->getValue();
								int rightStrandPosition=annotation.getPositionOnStrand();
								Strand rightStrand=annotation.getStrand();
								Strand leftStrand=element->m_readStrand;
								int leftStrandPosition=element->m_strandPosition;
											
								if(( leftStrand=='F' && rightStrand=='R' )
								||(  leftStrand=='R' && rightStrand=='F' )){// make sure the orientation is OK
									int p1=element->m_readPosition;
									int p2=m_EXTENSION_currentPosition;
									int d=p2-p1+readLength+leftStrandPosition-rightStrandPosition;
									(*m_libraryDistances)[library][d]++;
									(*m_detectedDistances)++;
								}
							}
						}
					}
					m_EXTENSION_edgeIterator++;
					m_EXTENSION_hasPairedReadRequested=false;
				}
			}else{
				for(int i=0;i<(int)m_readFetcher.getResult()->size();i++){
					ReadHandle uniqueId=m_readFetcher.getResult()->at(i).getUniqueId();
					int position=m_EXTENSION_currentPosition;
					Strand strand=m_readFetcher.getResult()->at(i).getStrand();
					int strandPosition=m_readFetcher.getResult()->at(i).getPositionOnStrand();
					// read, position, strand
					bool flag;
					SplayNode<ReadHandle,LibraryElement>*node=m_database.insert(uniqueId,m_allocator,&flag);
					LibraryElement*element=node->getValue();
					element->m_readPosition=position;
					element->m_readStrand=strand;
					element->m_strandPosition=strandPosition;
				}
				m_EXTENSION_currentPosition++;
				m_EXTENSION_reads_requested=false;
			}
		}
	}
}

WorkerHandle LibraryWorker::getWorkerIdentifier(){
	return m_SEEDING_i;
}
