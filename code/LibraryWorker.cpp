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
#include <map>
#include <mpi.h>
#include <VirtualCommunicator.h>
using namespace std;

bool LibraryWorker::isDone(){
	return m_done;
}

void LibraryWorker::constructor(int id,SeedingData*seedingData,VirtualCommunicator*virtualCommunicator,RingAllocator*outboxAllocator,Parameters*parameters,
StaticVector*inbox,StaticVector*outbox,	map<int,map<int,int> >*libraryDistances,int*detectedDistances){
	cout<<"LibraryWorker::Constructor"<<endl;
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

	m_readsPositions.clear();
	m_readsStrands.clear();
	m_strandPositions.clear();
}

void LibraryWorker::work(){
	if(m_done){
		return;
	}
	//cout<<"Work"<<endl;
	#ifdef ASSERT
	assert(m_SEEDING_i<(int)m_seedingData->m_SEEDING_seeds.size());
	#endif
	if(m_EXTENSION_currentPosition==(int)m_seedingData->m_SEEDING_seeds[m_SEEDING_i].size()){
		m_done=true;
		//cout<<"DONE"<<endl;
	}else{
		if(!m_EXTENSION_reads_requested){
			//cout<<"Requesting reads"<<endl;
			m_EXTENSION_reads_requested=true;
			#ifdef ASSERT
			assert(m_seedingData!=NULL);
			assert(m_EXTENSION_currentPosition<(int)m_seedingData->m_SEEDING_seeds[m_SEEDING_i].size());
			#endif
			uint64_t vertex=m_seedingData->m_SEEDING_seeds[m_SEEDING_i][m_EXTENSION_currentPosition];
		
			m_readFetcher.constructor(vertex,m_outboxAllocator,m_inbox,m_outbox,m_parameters,m_virtualCommunicator);
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
					uint64_t*message=(uint64_t*)(m_outboxAllocator)->allocate(1*sizeof(uint64_t));
					message[0]=rightRead;
					#ifdef ASSERT
					assert(m_parameters!=NULL);
					#endif
					Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),RAY_MPI_TAG_GET_READ_MATE,m_parameters->getRank());
					//cout<<"Requesting mate"<<endl;
					//(m_outbox)->push_back(aMessage);
					m_virtualCommunicator->pushMessage(rightRead,&aMessage);
					m_EXTENSION_hasPairedReadRequested=true;
				}else if(m_virtualCommunicator->isMessageProcessed(rightRead)){
					//cout<<"Got mate"<<endl;
					vector<uint64_t> buffer=m_virtualCommunicator->getResponseElements(rightRead);
					#ifdef ASSERT
					assert((int)buffer.size()==4);
					#endif
					if((int)buffer[1]!=-1){
						//cout<<"Got mate, checking if automatic"<<endl;
						int library=buffer[3];
						int readLength=buffer[0];
						bool isAutomatic=m_parameters->isAutomatic(library);
						if(isAutomatic){
							uint64_t uniqueReadIdentifier=getPathUniqueId(buffer[1],buffer[2]);
							if((m_readsPositions).count(uniqueReadIdentifier)>0){
								int rightStrandPosition=annotation.getPositionOnStrand();
								char rightStrand=annotation.getStrand();
								char leftStrand=m_readsStrands[uniqueReadIdentifier];
								int leftStrandPosition=m_strandPositions[uniqueReadIdentifier];
											
								if(( leftStrand=='F' && rightStrand=='R' )
								||(  leftStrand=='R' && rightStrand=='F' )){// make sure the orientation is OK
									int p1=(m_readsPositions)[uniqueReadIdentifier];
									int p2=m_EXTENSION_currentPosition;
									int d=p2-p1+readLength+leftStrandPosition-rightStrandPosition;
									//cout<<"d="<<d<<" lId="<<annotation.getUniqueId()<<" rId="<<uniqueReadIdentifier<<" pLeft="<<p1<<" pRight="<<p2<<" lStrand="<<leftStrand<<" rStrand="<<rightStrand<<" leftStrandPos="<<leftStrandPosition<<" rightStrandPos="<<rightStrandPosition<<" RightLength="<<readLength<<endl;
									(*m_libraryDistances)[library][d]++;
									(*m_detectedDistances)++;
								}
							}else{
								//cout<<"Pair was not found."<<endl;
							}
						}
					}
					//cout<<"Next read"<<endl;
					m_EXTENSION_edgeIterator++;
					m_EXTENSION_hasPairedReadRequested=false;
				}
			}else{
				//cout<<"Storing positions"<<endl;
				for(int i=0;i<(int)m_readFetcher.getResult()->size();i++){
					uint64_t uniqueId=m_readFetcher.getResult()->at(i).getUniqueId();
					int position=m_EXTENSION_currentPosition;
					char strand=m_readFetcher.getResult()->at(i).getStrand();
					int strandPosition=m_readFetcher.getResult()->at(i).getPositionOnStrand();
					// read, position, strand
					(m_readsPositions)[uniqueId]=position;
					m_readsStrands[uniqueId]=strand;
					m_strandPositions[uniqueId]=strandPosition;
					//cout<<"Read Id="<<uniqueId<<" Strand="<<strand<<" StrandPosition="<<strandPosition<<" PositionOnSeed="<<position<<endl;
				}
				//cout<<"Next position"<<endl;
				m_EXTENSION_currentPosition++;
				m_EXTENSION_reads_requested=false;
			}
		}
	}


}
