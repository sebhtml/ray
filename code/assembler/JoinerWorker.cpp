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


#include <assembler/JoinerWorker.h>

bool JoinerWorker::isDone(){
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
void JoinerWorker::work(){
/*
  used tags:

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
				cout<<"JoinerWorker "<<m_workerIdentifier<<" position: ["<<m_position<<"/"<<m_path->size()<<endl;
			}
*/
			Kmer kmer=m_path->at(m_position);

			if(m_reverseStrand)
				kmer=kmer.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

			int destination=kmer.vertexRank(m_parameters->getSize(),
				m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
			int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE);
			uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(elementsPerQuery);
			int outputPosition=0;
			kmer.pack(message,&outputPosition);
			Message aMessage(message,elementsPerQuery,destination,
				RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);

			m_requestedNumberOfPaths=true;
			m_receivedNumberOfPaths=false;

			if(m_parameters->hasOption("-debug-fusions"))
				cout<<"worker "<<m_workerIdentifier<<" send RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE"<<endl;

		/* receive the number of paths */
		}else if(m_requestedNumberOfPaths && !m_receivedNumberOfPaths && m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
			vector<uint64_t> response;
			m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
			m_numberOfPaths=response[0];
		
			if(m_parameters->hasOption("-debug-fusions"))
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
	
				int destination=kmer.vertexRank(m_parameters->getSize(),
					m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
				int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATH);
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(elementsPerQuery);
				int outputPosition=0;
				kmer.pack(message,&outputPosition);
				message[outputPosition++]=m_pathIndex;

				Message aMessage(message,elementsPerQuery,destination,
					RAY_MPI_TAG_ASK_VERTEX_PATH,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);

				if(m_parameters->hasOption("-debug-fusions"))
					cout<<"worker "<<m_workerIdentifier<<" send RAY_MPI_TAG_ASK_VERTEX_PATH "<<m_pathIndex<<endl;

				m_requestedPath=true;
				m_receivedPath=false;
			/* receive the path */
			}else if(!m_receivedPath && m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
				vector<uint64_t> response;
				m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
				int bufferPosition=0;

				/* skip the k-mer because we don't need it */
				bufferPosition+=KMER_U64_ARRAY_SIZE;
				uint64_t otherPathIdentifier=response[bufferPosition++];
				int progression=response[bufferPosition++];

				if(m_parameters->hasOption("-debug-fusions"))
					cout<<"worker "<<m_workerIdentifier<<" receive RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY"<<endl;

				if(otherPathIdentifier != m_identifier){
					m_hits[otherPathIdentifier]++;
			
					/* TODO: these values should be updated in a better way. */

					/* TODO  if m_reverseStrand is true, then m_position should be (LENGTH - m_position -1) */

					int positionOnSelf=m_position;
					if(m_reverseStrand){
						positionOnSelf=m_path->size()-m_position-1;
					}

					if(m_minPosition.count(otherPathIdentifier) == 0){
						m_minPosition[otherPathIdentifier]=progression;
						m_minPositionOnSelf[otherPathIdentifier]=positionOnSelf;
					}

					if(m_maxPosition.count(otherPathIdentifier) == 0){
						m_maxPosition[otherPathIdentifier]=progression;
						m_maxPositionOnSelf[otherPathIdentifier]=positionOnSelf;
					}

					if(progression < m_minPosition[otherPathIdentifier]){
						m_minPosition[otherPathIdentifier]=progression;
						m_minPositionOnSelf[otherPathIdentifier]=positionOnSelf;
					}

					if(progression > m_maxPosition[otherPathIdentifier]){
						m_maxPosition[otherPathIdentifier]=progression;
						m_maxPositionOnSelf[otherPathIdentifier]=positionOnSelf;
					}
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


			if(m_parameters->hasOption("-debug-fusions"))
				cout<<"worker "<<m_workerIdentifier<<" Next position is "<<m_position<<endl;
		}
	/* gather hit information */
	}else if(!m_gatheredHits){
		if(!m_initializedGathering){
			for(map<uint64_t,int>::iterator i=m_hits.begin();i!=m_hits.end();i++){
				m_hitNames.push_back(i->first);
			}
			m_initializedGathering=true;
			m_hitIterator=0;
			m_requestedHitLength=false;
		}else if(m_hitIterator < (int) m_hitNames.size()){
			/* ask the hit length */
			if(!m_requestedHitLength){
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1);

				uint64_t hitName=m_hitNames[m_hitIterator];
				int destination=getRankFromPathUniqueId(hitName);

				message[0]=hitName;

				Message aMessage(message,1,destination,
					RAY_MPI_TAG_GET_PATH_LENGTH,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
				m_requestedHitLength=true;

			/* receive the hit length */
			}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
				vector<uint64_t> response;
				m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
				int length=response[0];
				if(m_parameters->hasOption("-debug-fusions"))
					cout<<"received length, value= "<<length<<endl;
				m_hitLengths.push_back(length);

				m_hitIterator++;
				m_requestedHitLength=false;
			}
		}else{
			m_gatheredHits=true;
			m_selectedHit=false;
		}
	}else if(!m_selectedHit){
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

		cout<<"worker "<<m_workerIdentifier<<" path "<<m_identifier<<" is Done, analyzed "<<m_position<<" position length is "<<m_path->size()<<endl;
		cout<<"worker "<<m_workerIdentifier<<" hits "<<endl;

		int selectedHit=0;
		int numberOfHits=0;

		for(int i=0;i<(int)m_hitNames.size();i++){
			uint64_t hit=m_hitNames[i];
			//int hitLength=m_hitLengths[i];
			//int selfLength=m_path->size();
			int matches=m_hits[hit];

			#ifdef ASSERT
			assert(hit != m_identifier);
			#endif

/*
			double ratio=(matches+0.0)/selfLength;
			if(ratio < 0.1)
				continue;
*/

			if(matches < 1000)
				continue;

			/* TODO check that the hit is on one side */

			int selfRange=m_maxPositionOnSelf[hit] - m_minPositionOnSelf[hit];
			if(selfRange < 0)
				selfRange-=selfRange;

			int otherRange=m_maxPosition[hit] - m_minPosition[hit];

			if(otherRange< 0)
				otherRange-=otherRange;

			double selfRangeRatio=(selfRange+0.0)/matches;
			double otherRangeRatio=(otherRange+0.0)/matches;

			if(selfRangeRatio < 0.7 || otherRangeRatio < 0.7 || selfRangeRatio > 1.3 || otherRangeRatio > 1.3)
				continue;

			numberOfHits++;
			selectedHit=i;
		}

		if(numberOfHits == 1){
			uint64_t hit=m_hitNames[selectedHit];
			int hitLength=m_hitLengths[selectedHit];
			int selfLength=m_path->size();
			int matches=m_hits[hit];
			
			cout<<"SelectedHit selfPath= "<<m_identifier<<" selfStrand="<<m_reverseStrand<<" selfLength= "<<selfLength<<" MinSelf="<<m_minPositionOnSelf[hit]<<" MaxSelf="<<m_maxPositionOnSelf[hit]<<" Path="<<hit<<"	matches= "<<matches<<"	length= "<<hitLength<<" minPosition= "<<m_minPosition[hit]<<" maxPosition= "<<m_maxPosition[hit]<<endl;
			m_selectedHit=true;
			m_selectedHitIndex=selectedHit;
			m_hitPosition=0;
			m_requestedHitVertex=false;
		}else{
			m_isDone=true;
		}

	/* gather all the vertices of the hit and try to join them */
	}else if(m_selectedHit){
		int hitLength=m_hitLengths[m_selectedHitIndex];
		if(m_hitPosition < hitLength){
			if(!m_requestedHitVertex){
				uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(1);

				uint64_t hitName=m_hitNames[m_selectedHitIndex];
				int destination=getRankFromPathUniqueId(hitName);

				message[0]=hitName;
				message[1]=m_hitPosition;

				int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_PATH_VERTEX);

				Message aMessage(message,elementsPerQuery,destination,
					RAY_MPI_TAG_GET_PATH_VERTEX,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
				m_requestedHitVertex=true;
			}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
				vector<uint64_t> response;
				m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
				int position=0;
				Kmer kmer;
				kmer.unpack(&response,&position);
				m_hitVertices.push_back(kmer);
				
				m_hitPosition++;
				m_requestedHitVertex=false;
			}
		}else{
			uint64_t hitName=m_hitNames[m_selectedHitIndex];
			int matches=m_hits[hitName];

			cout<<"Received hit path data."<<endl;
			cout<<"Matches: "<<matches<<endl;
			cout<<"Self"<<endl;
			cout<<" Identifier: "<<m_identifier<<endl;
			cout<<" Strand: "<<m_reverseStrand<<endl;
			cout<<" Length: "<<m_path->size()<<endl;
			cout<<" Begin: "<<m_minPositionOnSelf[hitName]<<endl;
			cout<<" End: "<<m_maxPositionOnSelf[hitName]<<endl;
			cout<<"Hit"<<endl;
			cout<<" Identifier: "<<hitName<<endl;
			cout<<" Strand: 0"<<endl;
			cout<<" Length: "<<hitLength<<endl;
			cout<<" Begin: "<<m_minPosition[hitName]<<endl;
			cout<<" End: "<<m_maxPosition[hitName]<<endl;

			#ifdef ASSERT
			assert(hitLength == (int)m_hitVertices.size());
			#endif

			/* self can be forward or reverse
 * 				self can hit on its left or on its right side
 * 			other can be forward
 * 			other can hit on its left and on its right side 
 * 			*/

			int selfMiddle=(m_minPositionOnSelf[hitName]+m_maxPositionOnSelf[hitName])/2;

			int otherMiddle=(m_minPosition[hitName]+m_maxPosition[hitName])/2;

			int LEFT_SIDE=0;
			int RIGHT_SIDE=1;

			int selfSide=LEFT_SIDE;
			if(selfMiddle > (int)(m_path->size()/2))
				selfSide=RIGHT_SIDE;

			int otherSide=LEFT_SIDE;
			if(otherMiddle > hitLength/2)
				otherSide=RIGHT_SIDE;

			/* self is not reverse complement */
			if(!m_reverseStrand){
				/*
 * 				--------------->
 * 					---------------->
 * 					*/
				if(selfSide==RIGHT_SIDE && otherSide == LEFT_SIDE){
					cout<<"VALID"<<endl;
					
					vector<Kmer> newPath=(*m_path);
					for(int i=m_maxPosition[hitName]+1;i<(int)hitLength;i++){
						newPath.push_back(m_hitVertices.at(i));
					}

					m_newPaths->push_back(newPath);

					cout<<"Created new path, length= "<<newPath.size()<<endl;
					m_eliminated=true;

/*
 *                          ------------->
 *                     ------------>
 *                     */
				}else if(selfSide==LEFT_SIDE && otherSide == RIGHT_SIDE){
					cout<<"VALID"<<endl;

					vector<Kmer> newPath=m_hitVertices;
					for(int i=m_maxPositionOnSelf[hitName]+1;i<(int)m_path->size();i++){
						newPath.push_back(m_path->at(i));
					}
					m_newPaths->push_back(newPath);

					cout<<"Created new path, length= "<<newPath.size()<<endl;
					m_eliminated=true;

				}else{
					cout<<"INVALID"<<endl;
				}
			/* self is reverse complement */
			}else{

/*
 *
 *                      <------------------------
 *                                    --------------------->
 *                                    */
				
				if(selfSide==RIGHT_SIDE && otherSide == LEFT_SIDE){
					cout<<"VALID"<<endl;
/*
 *
 *                      <------------------------
 *        --------------------->
 *                                    */
				}else if(selfSide==LEFT_SIDE && otherSide == RIGHT_SIDE){
					cout<<"VALID"<<endl;
				}else{
					cout<<"INVALID"<<endl;
				}

					
			}

			m_isDone=true;
		}
	}
}

uint64_t JoinerWorker::getWorkerIdentifier(){
	return m_workerIdentifier;
}

void JoinerWorker::constructor(uint64_t number,vector<Kmer>*path,uint64_t identifier,bool reverseStrand,
	VirtualCommunicator*virtualCommunicator,Parameters*parameters,RingAllocator*outboxAllocator,
vector<vector<Kmer> >*newPaths
){
	m_newPaths=newPaths;
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

bool JoinerWorker::isPathEliminated(){
	return m_eliminated;
}

uint64_t JoinerWorker::getPathIdentifier(){
	return m_identifier;
}
