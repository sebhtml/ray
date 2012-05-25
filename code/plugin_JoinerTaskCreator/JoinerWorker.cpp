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


#include <plugin_JoinerTaskCreator/JoinerWorker.h>
#include <iostream>
using namespace std;

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

			#ifdef ASSERT
			assert(m_position < (int)m_path->size());
			#endif

			Kmer kmer=m_path->at(m_position);

			if(m_reverseStrand)
				kmer=kmer.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

			int destination=kmer.vertexRank(m_parameters->getSize(),
				m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

			#ifdef ASSERT
			assert(destination < m_parameters->getSize() && destination >= 0);
			#endif

			int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE);
			MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(elementsPerQuery);
			int outputPosition=0;
			kmer.pack(message,&outputPosition);

			//cout<<"Comm"<<m_parameters->getRank()<<" sends to "<<destination<<" tag "<<MESSAGE_TAGS[RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE]<<endl;

			Message aMessage(message,elementsPerQuery,destination,
				RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,m_parameters->getRank());
			m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);

			m_requestedNumberOfPaths=true;
			m_receivedNumberOfPaths=false;

			if(m_parameters->hasOption("-debug-fusions2"))
				cout<<"worker "<<m_workerIdentifier<<" send RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE"<<endl;

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
				if(m_reverseStrand){
					kmer=kmer.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
				}
	
				int destination=kmer.vertexRank(m_parameters->getSize(),
					m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
				int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATH);
				MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(elementsPerQuery);
				int outputPosition=0;
				kmer.pack(message,&outputPosition);
				message[outputPosition++]=m_pathIndex;

				Message aMessage(message,elementsPerQuery,destination,
					RAY_MPI_TAG_ASK_VERTEX_PATH,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);

				if(m_parameters->hasOption("-debug-fusions2"))
					cout<<"worker "<<m_workerIdentifier<<" send RAY_MPI_TAG_ASK_VERTEX_PATH "<<m_pathIndex<<endl;

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
				int progression=response[bufferPosition++];

				if(m_parameters->hasOption("-debug-fusions2")){
					cout<<"worker "<<m_workerIdentifier<<" receive RAY_MPI_TAG_ASK_VERTEX_PATH_REPLY"<<endl;
				}

				if(otherPathIdentifier != m_identifier){
					m_hits[otherPathIdentifier]++;
			
					int positionOnSelf=m_position;

					if(m_reverseStrand){
						positionOnSelf=m_path->size()-m_position-1;
					}

					// maybe it would be better not to store everything ?
					// an algorithm would be needed for that however.

					// just store everything and check them later...
					m_selfPositions[otherPathIdentifier].push_back(positionOnSelf);
					m_hitPositions[otherPathIdentifier].push_back(progression);

					if(m_parameters->hasOption("-debug-fusions")){
						cout<<"SelfLength= "<<m_path->size()<<" SelfStrand= "<<m_reverseStrand<<" MatchPair "<<" Self= "<<positionOnSelf<<" Other= "<<progression<<endl;
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


			if(m_parameters->hasOption("-debug-fusions2")){
				cout<<"worker "<<m_workerIdentifier<<" Next position is "<<m_position<<endl;
			}
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

				PathHandle hitName=m_hitNames[m_hitIterator];
				int length=response[0];
				if(m_parameters->hasOption("-debug-fusions2")){
					cout<<"received length, value= "<<length<<endl;
				}

				m_hitLengths[hitName]=(length);

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

		// here, populate coordinates

		#ifdef ASSERT
		assert(m_selfPositions.size()==m_hitPositions.size());
		#endif

		int bestSelfScore=0;
		int bestOtherScore=0;

		for(map<PathHandle,vector<int> >::iterator i=m_selfPositions.begin();i!=m_selfPositions.end();i++){
	
			PathHandle otherIdentifier=i->first;

			set<int> positionsForSelf;
			set<int> positionsForOther;

			for(int j=0;j<(int)m_selfPositions[otherIdentifier].size();j++){
				positionsForSelf.insert(m_selfPositions[otherIdentifier][j]);
				positionsForOther.insert(m_hitPositions[otherIdentifier][j]);
			}

			// find the coordinates now
			
			int maximumBadEvents=32;
			int relevantScore=128;

			// for self
			for(int j=0;j<(int)m_selfPositions[otherIdentifier].size();j++){

				int start=m_selfPositions[otherIdentifier][j];
				int before=start-1;

				if(positionsForSelf.count(before)>0){
					continue;
				}

				int localScore=0;

				int badEvents=0;

				int position=start;
				int last=start;

				while(positionsForSelf.count(position)>0 || badEvents < maximumBadEvents ){

					if(positionsForSelf.count(position)==0){
						badEvents++;
					}else{
						badEvents=0;
						last=position;
					}

					localScore++;
					position++;
				}

				if(localScore > bestSelfScore){
			
					if(localScore >= relevantScore){
						cout<<"New best score for self (handle: "<<m_identifier<<", length: "<<m_path->size()<<") with "<<localScore<<", other handle: "<<otherIdentifier<<endl;
					}

					m_minPositionOnSelf[otherIdentifier]=start;
					m_maxPositionOnSelf[otherIdentifier]=last;

					bestSelfScore=localScore;// this is needed to avoid future deceptions in the probing event scheduling enterprise
				}
			}

			// for other
			for(int j=0;j<(int)m_selfPositions[otherIdentifier].size();j++){

				int start=m_hitPositions[otherIdentifier][j];
				int before=start-1;

				if(positionsForOther.count(before)>0){
					continue;
				}

				int localScore=0;

				int badEvents=0;

				int position=start;
				int last=start;

				while(positionsForOther.count(position)>0 || badEvents < maximumBadEvents ){

					if(positionsForOther.count(position)==0){
						badEvents++;
					}else{
						badEvents=0;
						last=position;
					}

					localScore++;
					position++;
				}

				if(localScore > bestOtherScore){
			
					if(localScore >= relevantScore){
						cout<<"New best score for other (handle: "<<otherIdentifier<<", length: "<<m_hitLengths[otherIdentifier]<<") with "<<localScore<<", other handle: "<<otherIdentifier<<endl;
					}

					m_minPosition[otherIdentifier]=start;
					m_maxPosition[otherIdentifier]=last;


					bestOtherScore=localScore;// this is needed to avoid future deceptions in the probing event scheduling enterprise
				}

			}
		}

		m_selfPositions.clear();
		m_hitPositions.clear();

		#ifdef ASSERT
		assert(m_selfPositions.size()==m_hitPositions.size());
		assert(m_selfPositions.size()==0);
		#endif

		if(m_parameters->hasOption("-debug-fusions")){
			cout<<"JoinerWorker worker "<<m_workerIdentifier<<" path "<<m_identifier<<" strand= "<<m_reverseStrand<<" is Done, analyzed "<<m_position<<" position length is "<<m_path->size()<<endl;
			cout<<"JoinerWorker worker "<<m_hits.size()<<" hits "<<endl;
		}

		int selectedHit=0;
		int numberOfHits=0;

		for(int i=0;i<(int)m_hitNames.size();i++){
			PathHandle hit=m_hitNames[i];
			//int hitLength=m_hitLengths[i];
			//int selfLength=m_path->size();


			#ifdef ASSERT
			assert(m_hits.count(hit)>0);
			#endif

			int matches=m_hits[hit];

			#ifdef ASSERT
			assert(hit != m_identifier);
			#endif

/*
			double ratio=(matches+0.0)/selfLength;
			if(ratio < 0.1)
				continue;
*/

			int hitLength=m_hitLengths[hit];
			int selfLength=m_path->size();

			if(m_parameters->hasOption("-debug-fusions")){
				cout<<"JoinerWorker hit selfPath= "<<m_identifier<<" selfStrand="<<m_reverseStrand<<" selfLength= "<<selfLength<<" MinSelf="<<m_minPositionOnSelf[hit]<<" MaxSelf="<<m_maxPositionOnSelf[hit]<<" Path="<<hit<<"	matches= "<<matches<<"	length= "<<hitLength<<" minPosition= "<<m_minPosition[hit]<<" maxPosition= "<<m_maxPosition[hit]<<endl;
			}

			double minimumMatchProportion=0.1;
			int minimumMatchesTest1=1000;
			//int minimumMatchesTest2=4096;
			double softZone=0.3;


			// TODO: why 1000 ? why not 1024 ?
			if(matches < minimumMatchesTest1){
				continue;
			}

			/* TODO check that the hit is on one side */

			int selfRange=m_maxPositionOnSelf[hit] - m_minPositionOnSelf[hit];

			if(selfRange < 0)
				selfRange-=selfRange;

			int otherRange=m_maxPosition[hit] - m_minPosition[hit];

			if(otherRange< 0)
				otherRange-=otherRange;

			double selfRangeRatio=(selfRange+0.0)/matches;
			double otherRangeRatio=(otherRange+0.0)/matches;

			if(selfRangeRatio < (1-softZone) || otherRangeRatio < (1-softZone) 
				|| selfRangeRatio > (1+softZone) || otherRangeRatio > (1+softZone)){

				continue;
			}

/*
			if(matches < minimumMatchesTest2){
				continue; // this is too risky !
			}
*/

			double selfMatchRatio=matches/(0.0+selfLength);
			double otherMatchRatio=matches/(0.0+hitLength);

			if(selfMatchRatio < minimumMatchProportion || otherMatchRatio < minimumMatchProportion){
				continue;
			}

	/*
 *
 * only do this trick if we are really sure about it ...
 *

Example that will create a misassembly:

Created new path, length= 370536
Received hit path data.
Matches: 1451
Self
 Identifier: 5000003
 Strand: 0
 Length: 154684
 Begin: 153202
 End: 154683
Hit
 Identifier: 1
 Strand: 0
 Length: 242904
 Begin: 25570
 End: 27051

while there is 1451 matches, this is simply not sufficient to do the matching.
at the moment, the minimum is 1000. Maybe change that to something larger, like 
8192.

The main purpose of this code is to merge things that are identical, almost.

We have to be careful however because repeated stuff in the genome will cause assembly errors
if the present code is not careful.

irb(main):002:0> 1451/154684.0
=> 0.0093804142639187

rb(main):003:0> 1451.0/242904
=> 0.00597355333794421

Also, don't do it if the matching ratios are below 10%.


 *
 *
 */

			numberOfHits++;
			selectedHit=i;
		}

		if(numberOfHits == 1){
			PathHandle hit=m_hitNames[selectedHit];
			int hitLength=m_hitLengths[hit];
			int selfLength=m_path->size();
			int matches=m_hits[hit];
			
			if(m_parameters->hasOption("-debug-fusions")){
				cout<<"SelectedHit selfPath= "<<m_identifier<<" selfStrand="<<m_reverseStrand<<" selfLength= "<<selfLength<<" MinSelf="<<m_minPositionOnSelf[hit]<<" MaxSelf="<<m_maxPositionOnSelf[hit]<<" Path="<<hit<<"	matches= "<<matches<<"	length= "<<hitLength<<" minPosition= "<<m_minPosition[hit]<<" maxPosition= "<<m_maxPosition[hit]<<endl;
			}

			m_selectedHit=true;
			m_selectedHitIndex=selectedHit;
			m_hitPosition=0;
			m_requestedHitVertex=false;
		}else{

			// there is more than 1 hit
			// do nothing with this.
			m_isDone=true;

			cout<<"Notice: number of hits is not 1: "<<numberOfHits<<" fetched hits."<<endl;
		}

	/* gather all the vertices of the hit and try to join them */
	}else if(m_selectedHit){

		PathHandle hitName=m_hitNames[m_selectedHitIndex];

		int hitLength=m_hitLengths[hitName];

		if(m_hitPosition < hitLength){
			if(!m_requestedHitVertex){
				MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1);

				int destination=getRankFromPathUniqueId(hitName);

				message[0]=hitName;
				message[1]=m_hitPosition;

				int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_PATH_VERTEX);

				Message aMessage(message,elementsPerQuery,destination,
					RAY_MPI_TAG_GET_PATH_VERTEX,m_parameters->getRank());
				m_virtualCommunicator->pushMessage(m_workerIdentifier,&aMessage);
				m_requestedHitVertex=true;
			}else if(m_virtualCommunicator->isMessageProcessed(m_workerIdentifier)){
				vector<MessageUnit> response;
				m_virtualCommunicator->getMessageResponseElements(m_workerIdentifier,&response);
				int position=0;
				Kmer kmer;
				kmer.unpack(&response,&position);
				m_hitVertices.push_back(kmer);
				
				m_hitPosition++;
				m_requestedHitVertex=false;
			}
		}else{
			PathHandle hitName=m_hitNames[m_selectedHitIndex];
			int matches=m_hits[hitName];


			if(m_parameters->hasOption("-debug-fusions")){
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
			}

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

			/*
 * 			--------------->
 * 				---------------->
 * 				*/
			if(selfSide==RIGHT_SIDE && otherSide == LEFT_SIDE){
				cout<<"VALID"<<endl;
				
				vector<Kmer> newPath;

				/* we take directly the path */
				if(!m_reverseStrand){
					newPath=(*m_path);
				}else{
					/* we need the reverse complement path */
					vector<Kmer> rc;
					for(int j=(*m_path).size()-1;j>=0;j--){
						rc.push_back((*m_path)[j].complementVertex(m_parameters->getWordSize(),
							m_parameters->getColorSpaceMode()));
					}
					newPath=rc;
				}

				/* other path is always forward strand */
				for(int i=m_maxPosition[hitName]+1;i<(int)hitLength;i++){
					newPath.push_back(m_hitVertices.at(i));
				}

				m_newPaths->push_back(newPath);

				cout<<"Created new path, length= "<<newPath.size()<<endl;
				
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


				m_eliminated=true;

/*
 *                  ------------->
 *             ------------>
 *             */
			}else if(selfSide==LEFT_SIDE && otherSide == RIGHT_SIDE){
				cout<<"VALID"<<endl;

				/* other path is always forward strand */
				vector<Kmer> newPath=m_hitVertices;

				/* we push the forward path */
				if(!m_reverseStrand){
					for(int i=m_maxPositionOnSelf[hitName]+1;i<(int)m_path->size();i++){
						newPath.push_back(m_path->at(i));
					}

				/* we push the reverse path */
				}else{
					vector<Kmer> rc;
					for(int j=(*m_path).size()-1;j>=0;j--){
						rc.push_back((*m_path)[j].complementVertex(m_parameters->getWordSize(),
							m_parameters->getColorSpaceMode()));
					}

					for(int i=m_maxPositionOnSelf[hitName]+1;i<(int)m_path->size();i++){
						newPath.push_back(rc.at(i));
					}

				}
				m_newPaths->push_back(newPath);

				cout<<"Created new path, length= "<<newPath.size()<<endl;

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


				m_eliminated=true;

			}else{
				cout<<"INVALID"<<endl;
			}

			m_isDone=true;
		}
	}
}

WorkerHandle JoinerWorker::getWorkerIdentifier(){
	return m_workerIdentifier;
}

void JoinerWorker::constructor(WorkerHandle number,vector<Kmer>*path,PathHandle identifier,bool reverseStrand,
	VirtualCommunicator*virtualCommunicator,Parameters*parameters,RingAllocator*outboxAllocator,
vector<vector<Kmer> >*newPaths,

	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
	MessageTag RAY_MPI_TAG_GET_PATH_LENGTH,
	MessageTag RAY_MPI_TAG_GET_PATH_VERTEX
){

	this->RAY_MPI_TAG_ASK_VERTEX_PATH=RAY_MPI_TAG_ASK_VERTEX_PATH;
	this->RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE=RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	this->RAY_MPI_TAG_GET_PATH_LENGTH=RAY_MPI_TAG_GET_PATH_LENGTH;
	this->RAY_MPI_TAG_GET_PATH_VERTEX=RAY_MPI_TAG_GET_PATH_VERTEX;

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

PathHandle JoinerWorker::getPathIdentifier(){
	return m_identifier;
}
