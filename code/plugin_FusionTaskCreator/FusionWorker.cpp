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


#include <plugin_FusionTaskCreator/FusionWorker.h>
#include <iostream>
using namespace std;

bool FusionWorker::isDone(){
	return m_isDone;
}

/**
 * work method.
 *
 * \author Sébastien Boisvert
 *
 * Code reviews
 *
 * 2011-09-02 -- Code review by Élénie Godzaridis (found bug with worker states)
 *
 */
void FusionWorker::work(){
/*
  used tags:

TODO: does the code pay attention when the coverage indicates a repeated k-mer ? repeats slow things down...

	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE
	RAY_MPI_TAG_ASK_VERTEX_PATH
	RAY_MPI_TAG_GET_PATH_LENGTH
*/

	if(m_isDone)
		return;

	if(m_position < (int) m_path->size()){

		/* get the number of paths */
		if(!m_requestedNumberOfPaths){
/*
			if(m_position % 1000 == 0){
				cout<<"FusionWorker "<<m_workerIdentifier<<" position: ["<<m_position<<"/"<<m_path->size()<<endl;
			}
*/
			Kmer kmer=m_path->at(m_position);

			if(m_reverseStrand)
				kmer=kmer.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

			Rank destination=kmer.vertexRank(m_parameters->getSize(),
				m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
			int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE);
			MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(elementsPerQuery);
			int outputPosition=0;
			kmer.pack(message,&outputPosition);
			Message aMessage(message,elementsPerQuery,destination,
				RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);

			m_requestedNumberOfPaths=true;
			m_receivedNumberOfPaths=false;

			if(m_parameters->hasOption("-debug-fusions2")){
				cout<<"worker "<<m_workerIdentifier<<" send RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE"<<endl;
			}

		/* receive the number of paths */
		}else if(m_requestedNumberOfPaths && !m_receivedNumberOfPaths && m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
			vector<MessageUnit> response;
			m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
			m_numberOfPaths=response[0];
		
			if(m_parameters->hasOption("-debug-fusions2"))
				cout<<"worker "<<m_workerIdentifier<<" Got "<<m_numberOfPaths<<endl;

			m_receivedNumberOfPaths=true;

			m_pathIndex=0;
			m_requestedPath=false;

			/* 2^5 */
			int maximumNumberOfPathsToProcess=32;

			/* don't process repeated stuff */
			if(m_numberOfPaths> maximumNumberOfPathsToProcess)
				m_numberOfPaths=0;

		}else if(m_receivedNumberOfPaths && m_pathIndex < m_numberOfPaths){
			/* request a path */
			if(!m_requestedPath){
				Kmer kmer=m_path->at(m_position);
				if(m_reverseStrand)
					kmer=kmer.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	
				Rank destination=kmer.vertexRank(m_parameters->getSize(),
					m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
				int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATH);
				MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(elementsPerQuery);
				int outputPosition=0;
				kmer.pack(message,&outputPosition);
				message[outputPosition++]=m_pathIndex;

				Message aMessage(message,elementsPerQuery,destination,
					RAY_MPI_TAG_ASK_VERTEX_PATH,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);

				if(m_parameters->hasOption("-debug-fusions2")){
					cout<<"worker "<<m_workerIdentifier<<" send RAY_MPI_TAG_ASK_VERTEX_PATH "<<m_pathIndex<<endl;
				}

				m_requestedPath=true;
				m_receivedPath=false;
			/* receive the path */
			}else if(!m_receivedPath && m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
				vector<MessageUnit> response;
				m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
				int bufferPosition=0;

				/* skip the k-mer because we don't need it */
				bufferPosition+=KMER_U64_ARRAY_SIZE;
				PathHandle otherPathIdentifier=response[bufferPosition++];
				//int progression=response[bufferPosition++];

				if(m_parameters->hasOption("-debug-fusions2"))
					cout<<"worker "<<m_workerIdentifier<<" receive RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY"<<endl;

				if(otherPathIdentifier != m_identifier){
					m_hits[otherPathIdentifier]++;
				}
				m_receivedPath=true;

				m_pathIndex++;
				m_requestedPath=false;
			}
		/* received all paths, can do the next one */
		}else if(m_receivedNumberOfPaths && m_pathIndex == m_numberOfPaths){
			m_position++;
			m_requestedNumberOfPaths=false;
			m_receivedNumberOfPaths=false;


			if(m_parameters->hasOption("-debug-fusions2"))
				cout<<"worker "<<m_workerIdentifier<<" Next position is "<<m_position<<endl;
		}
	/* gather hit information */
	}else if(!m_gatheredHits){
		if(!m_initializedGathering){
			for(map<PathHandle,int>::iterator i=m_hits.begin();i!=m_hits.end();i++){
				m_hitNames.push_back(i->first);
			}
			m_initializedGathering=true;
			m_hitIterator=0;
			m_requestedHitLength=false;
		}else if(m_hitIterator < (int) m_hitNames.size()){
			/* ask the hit length */
			if(!m_requestedHitLength){
				MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1);

				PathHandle hitName=m_hitNames[m_hitIterator];
				Rank destination=getRankFromPathUniqueId(hitName);

				message[0]=hitName;

				Message aMessage(message,1,destination,
					RAY_MPI_TAG_GET_PATH_LENGTH,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
				m_requestedHitLength=true;

			/* receive the hit length */
			}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
				vector<MessageUnit> response;
				m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
				int length=response[0];

				if(m_parameters->hasOption("-debug-fusions2"))
					cout<<"received length, value= "<<length<<endl;
				m_hitLengths.push_back(length);

				m_hitIterator++;
				m_requestedHitLength=false;
			}
		}else{
			m_gatheredHits=true;
		}
	}else{
		/* at this point, we have:
 * 			m_hits
 * 			m_hitNames
 * 			m_hitLengths
 */
		#ifdef ASSERT
		assert(m_hits.size()==m_hitLengths.size());
		assert(m_hitLengths.size()==m_hitNames.size());
		assert(m_hitIterator == (int)m_hitLengths.size());
		#endif

		if(m_parameters->hasOption("-debug-fusions")){
			cout<<"FusionWorker worker "<<m_workerIdentifier<<" path "<<m_identifier<<" strand= "<<m_reverseStrand<<" is Done, analyzed "<<m_position<<" position length is "<<m_path->size()<<endl;
			cout<<"FusionWorker worker "<<m_hits.size()<<" hits "<<endl;
		}

		for(int i=0;i<(int)m_hitNames.size();i++){
			PathHandle hit=m_hitNames[i];
			int hitLength=m_hitLengths[i];
			int selfLength=m_path->size();
			int matches=m_hits[hit];

			#ifdef ASSERT
			assert(hit != m_identifier);
			#endif

			double ratio=(matches+0.0)/selfLength;

			if(m_parameters->hasOption("-debug-fusions")){
				cout<<"FusionWorker path "<<hit<<"	matches= "<<matches<<"	length= "<<hitLength<<endl;
			}

			if(ratio < 0.7)
				continue;

			/* the other is longer anyway */
			if(hitLength > selfLength){
				m_eliminated=true;
			}

			/* the longer is "greater" but of equal length */
			if(hitLength == selfLength && hit > m_identifier){
				m_eliminated=true;
			}

		}
		m_isDone=true;
	}
}

WorkerHandle FusionWorker::getWorkerIdentifier(){
	return m_workerIdentifier;
}

void FusionWorker::constructor(WorkerHandle number,vector<Kmer>*path,PathHandle identifier,bool reverseStrand,
	VirtualCommunicator*virtualCommunicator,Parameters*parameters,RingAllocator*outboxAllocator,

	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
	MessageTag RAY_MPI_TAG_GET_PATH_LENGTH
){
	this->RAY_MPI_TAG_GET_PATH_LENGTH=RAY_MPI_TAG_GET_PATH_LENGTH;
	this->RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE=RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	this->RAY_MPI_TAG_ASK_VERTEX_PATH=RAY_MPI_TAG_ASK_VERTEX_PATH;
	
	m_virtualCommunicator=virtualCommunicator;
	m_workerIdentifier=number;
	m_initializedGathering=false;
	m_gatheredHits=false;
	m_isDone=false;
	m_identifier=identifier;
	m_reverseStrand=reverseStrand;
	m_path=path;
	m_eliminated=false;

	m_outboxAllocator=outboxAllocator;
	m_parameters=parameters;
	m_position=0;
	m_requestedNumberOfPaths=false;

	if(m_parameters->hasOption("-debug-fusions")){
		cout<<"Spawned worker number "<<number<<endl;
		cout<<" path "<<m_identifier<<" reverse "<<m_reverseStrand<<" length "<<m_path->size()<<endl;
	}
}

bool FusionWorker::isPathEliminated(){
	return m_eliminated;
}

PathHandle FusionWorker::getPathIdentifier(){
	return m_identifier;
}
