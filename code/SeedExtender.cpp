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
#include<StaticVector.h>
#include<TipWatchdog.h>
#include<SeedExtender.h>
#include<Chooser.h>
#include<assert.h>
#include<BubbleTool.h>

// uncomment to display how Ray chooses things.
//#define SHOW_CHOICE

void debugMessage(int source,int destination,string message){
	cout<<"Microseconds: "<<getMicroSeconds()<<" Source: "<<source<<" Destination: "<<destination<<" Message: "<<message<<endl;
}

void SeedExtender::extendSeeds(vector<vector<uint64_t> >*seeds,ExtensionData*ed,int theRank,StaticVector*outbox,
  uint64_t*currentVertex,FusionData*fusionData,RingAllocator*outboxAllocator,bool*edgesRequested,int*outgoingEdgeIndex,
int*last_value,bool*vertexCoverageRequested,int wordSize,bool*colorSpaceMode,int size,bool*vertexCoverageReceived,
int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,vector<uint64_t>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,
int minimumCoverage,OpenAssemblerChooser*oa,bool*edgesReceived,int*m_mode){
	if((*seeds).size()==0){
		ed->destructor();
		printf("Rank %i is extending seeds [%i/%i] (completed)\n",theRank,(int)(*seeds).size(),(int)(*seeds).size());
		fflush(stdout);
		showMemoryUsage(theRank);
		fflush(stdout);
		ed->m_mode_EXTENSION=false;
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_EXTENSION_IS_DONE,theRank);
		(*outbox).push_back(aMessage);
		delete m_cache;
		delete m_dfsData;
		return;
	}
	if(!ed->m_EXTENSION_initiated){
		ed->m_EXTENSION_initiated=true;
		ed->m_EXTENSION_currentSeedIndex=0;
		ed->m_EXTENSION_currentPosition=0;
		ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
		(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
		ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
		ed->m_EXTENSION_directVertexDone=false;
		ed->m_EXTENSION_VertexAssembled_requested=false;
		ed->m_EXTENSION_complementedSeed=false;
		ed->m_EXTENSION_complementedSeed2=false;
	
		ed->constructor();

	}else if(ed->m_EXTENSION_currentSeedIndex==(int)(*seeds).size()){
		ed->destructor();
		printf("Rank %i is extending seeds [%i/%i] (completed)\n",theRank,(int)(*seeds).size(),(int)(*seeds).size());
		fflush(stdout);

		showMemoryUsage(theRank);
		ed->m_mode_EXTENSION=false;
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
	
		// store the lengths.
		for(int i=0;i<(int)ed->m_EXTENSION_identifiers.size();i++){
			uint64_t id=ed->m_EXTENSION_identifiers[i];
			fusionData->m_FUSION_identifier_map[id]=i;
		}
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_EXTENSION_IS_DONE,theRank);
		outbox->push_back(aMessage);
		return;
	}

	// algorithms here.
	// if the current vertex is assembled or if its reverse complement is assembled, return
	// else, mark it as assembled, and mark its reverse complement as assembled too.
	// 	enumerate the available choices
	// 	if choices are included in the seed itself
	// 		choose it
	// 	else
	// 		use read paths or pairs of reads to resolve the repeat.
	
	// only check that at bootstrap.

	if(!ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled){
		if(ed->m_EXTENSION_currentPosition>0){
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
			ed->m_EXTENSION_markedCurrentVertexAsAssembled=false;
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_VertexMarkAssembled_requested=false;
			(*vertexCoverageRequested)=false;
		}else{
			checkIfCurrentVertexIsAssembled(ed,outbox,outboxAllocator,outgoingEdgeIndex,last_value,
	currentVertex,theRank,vertexCoverageRequested,wordSize,colorSpaceMode,size,seeds);
		}
	}else if((ed->m_EXTENSION_currentPosition==0 && m_eliminatedSeeds.count(ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition])>0)
	|| (ed->m_EXTENSION_vertexIsAssembledResult and ed->m_EXTENSION_currentPosition==0 and ed->m_EXTENSION_complementedSeed==false)){
		//cout<<"Rank "<<theRank<<": Ray Early-Stopping Technology was triggered, Case 1: seed is already processed at p=0."<<endl;
		ed->m_EXTENSION_currentSeedIndex++;// skip the current one.
		ed->m_EXTENSION_currentPosition=0;

		//int waveId=ed->m_EXTENSION_currentSeedIndex*MAX_NUMBER_OF_MPI_PROCESSES+theRank;
		//m_earlyStoppingTechnology.constructor(waveId,theRank);

		ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
		ed->m_EXTENSION_directVertexDone=false;
		ed->m_EXTENSION_VertexAssembled_requested=false;
		if(ed->m_EXTENSION_currentSeedIndex<(int)(*seeds).size()){
			ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
			(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
		}
		// TODO: check if the position !=0
		ed->m_EXTENSION_complementedSeed=false;
		ed->m_EXTENSION_complementedSeed2=false;
		ed->resetStructures();

	}else if(!ed->m_EXTENSION_markedCurrentVertexAsAssembled){
		markCurrentVertexAsAssembled(currentVertex,outboxAllocator,outgoingEdgeIndex,outbox,
size,theRank,ed,vertexCoverageRequested,vertexCoverageReceived,receivedVertexCoverage,
repeatedLength,maxCoverage,edgesRequested,receivedOutgoingEdges,chooser,bubbleData,minimumCoverage,
oa,colorSpaceMode,wordSize);
	}else if(!ed->m_EXTENSION_enumerateChoices){
		enumerateChoices(edgesRequested,ed,edgesReceived,outboxAllocator,outgoingEdgeIndex,outbox,
		currentVertex,theRank,vertexCoverageRequested,receivedOutgoingEdges,
		vertexCoverageReceived,size,receivedVertexCoverage,chooser,wordSize);
	}else if(!ed->m_EXTENSION_choose){
		doChoice(outboxAllocator,outgoingEdgeIndex,outbox,currentVertex,bubbleData,theRank,wordSize,
	ed,minimumCoverage,*maxCoverage,oa,chooser,colorSpaceMode,seeds,
edgesRequested,vertexCoverageRequested,vertexCoverageReceived,size,receivedVertexCoverage,edgesReceived,
receivedOutgoingEdges);
	}
}

// upon successful completion, ed->m_EXTENSION_coverages and ed->m_enumerateChoices_outgoingEdges are
// populated variables.
void SeedExtender::enumerateChoices(bool*edgesRequested,ExtensionData*ed,bool*edgesReceived,RingAllocator*outboxAllocator,
	int*outgoingEdgeIndex,StaticVector*outbox,
uint64_t*currentVertex,int theRank,bool*vertexCoverageRequested,vector<uint64_t>*receivedOutgoingEdges,
bool*vertexCoverageReceived,int size,int*receivedVertexCoverage,Chooser*chooser,int wordSize
){
	if(!(*edgesRequested)){
		//cout<<__func__<<endl;
		ed->m_EXTENSION_coverages->clear();
		(*edgesReceived)=false;
		(*edgesRequested)=true;
		uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(1*sizeof(uint64_t));
		message[0]=(uint64_t)(*currentVertex);
		int dest=vertexRank((*currentVertex),size);
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,dest,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES,theRank);
		//cout<<"Sending a message to "<<dest<<endl;
		(*outbox).push_back(aMessage);
		ed->m_EXTENSION_currentPosition++;
		(*vertexCoverageRequested)=false;
		(*outgoingEdgeIndex)=0;
	}else if((*edgesReceived)){
		if((*outgoingEdgeIndex)<(int)(*receivedOutgoingEdges).size()){
			// get the coverage of these.
			if(!(*vertexCoverageRequested)&&(*m_cache).count((*receivedOutgoingEdges)[(*outgoingEdgeIndex)])>0){
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=true;
				(*receivedVertexCoverage)=(*m_cache)[(*receivedOutgoingEdges)[(*outgoingEdgeIndex)]];

				#ifdef ASSERT
				assert((*receivedVertexCoverage)<=255);
				#endif
			}else if(!(*vertexCoverageRequested)){
				uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(1*sizeof(uint64_t));
				message[0]=(uint64_t)(*receivedOutgoingEdges)[(*outgoingEdgeIndex)];
				int dest=vertexRank(message[0],size);
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,dest,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,theRank);
				(*outbox).push_back(aMessage);
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=false;
				(*receivedVertexCoverage)=-1;
			}else if((*vertexCoverageReceived)){
				(*m_cache)[(*receivedOutgoingEdges)[(*outgoingEdgeIndex)]]=*receivedVertexCoverage;
				(*outgoingEdgeIndex)++;
				(*vertexCoverageRequested)=false;
				#ifdef ASSERT
				assert((*receivedVertexCoverage)<=255);
				#endif
				ed->m_EXTENSION_coverages->push_back((*receivedVertexCoverage));
			}
		}else{
			//cout<<__func__<<" Got choices."<<endl;
			ed->m_EXTENSION_enumerateChoices=true;
			ed->m_EXTENSION_choose=false;
			ed->m_EXTENSION_singleEndResolution=false;
			ed->m_EXTENSION_readIterator=ed->m_EXTENSION_readsInRange->begin();
			ed->m_EXTENSION_readsOutOfRange.clear();
			ed->m_EXTENSION_readLength_done=false;
			ed->m_EXTENSION_readPositionsForVertices.clear();
			ed->m_EXTENSION_pairedReadPositionsForVertices.clear();
			ed->m_EXTENSION_pairedLibrariesForVertices.clear();
			ed->m_EXTENSION_pairedReadsForVertices.clear();

			ed->m_enumerateChoices_outgoingEdges=(*receivedOutgoingEdges);

			#ifdef ASSERT
			assert(ed->m_EXTENSION_coverages->size()==ed->m_enumerateChoices_outgoingEdges.size());
			#endif
		}
	}
}

/**
 *
 *  This function do a choice:
 *   IF the position is inside the given seed, THEN the seed is used as a backbone to do the choice
 *   IF the position IS NOT inside the given seed, THEN
 *      reads in range are mapped on available choices.
 *      then, Ray attempts to choose with paired-end reads
 *      if this fails, Ray attempts to choose with single-end reads
 *      if this fails, Ray attempts to choose by removing tips.
 *      if this fails, Ray attempts to choose by resolving bubbles
 */
void SeedExtender::doChoice(RingAllocator*outboxAllocator,int*outgoingEdgeIndex,StaticVector*outbox,
	uint64_t*currentVertex,BubbleData*bubbleData,int theRank,
	int wordSize,
ExtensionData*ed,int minimumCoverage,int maxCoverage,OpenAssemblerChooser*oa,Chooser*chooser,bool*colorSpaceMode,
	vector<vector<uint64_t> >*seeds,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,int size,
int*receivedVertexCoverage,bool*edgesReceived,vector<uint64_t>*receivedOutgoingEdges
){
	// use the seed to extend the thing.
	if(ed->m_EXTENSION_currentPosition<(int)ed->m_EXTENSION_currentSeed.size()){
		//cout<<"Extending with seed, p="<<ed->m_EXTENSION_currentPosition<<" size="<<ed->m_EXTENSION_currentSeed.size()<<endl;

		for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
			if(ed->m_enumerateChoices_outgoingEdges[i]==ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition]){
				(*currentVertex)=ed->m_enumerateChoices_outgoingEdges[i]; 
				ed->m_EXTENSION_choose=true; 
				ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false; 
				ed->m_EXTENSION_directVertexDone=false; 
				ed->m_EXTENSION_VertexAssembled_requested=false; 
				return; 
			}
		}

		#define SHOW_EXTEND_WITH_SEED
		#ifdef SHOW_EXTEND_WITH_SEED
		cout<<"What the hell? position="<<ed->m_EXTENSION_currentPosition<<" "<<idToWord(ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition],wordSize)<<" with "<<ed->m_enumerateChoices_outgoingEdges.size()<<" choices ";
		cout<<endl;
		cout<<"seed size= "<<ed->m_EXTENSION_currentSeed.size()<<endl;
		for(int j=0;j<(int)ed->m_enumerateChoices_outgoingEdges.size();j++){
			cout<<" "<<idToWord(ed->m_enumerateChoices_outgoingEdges[j],wordSize)<<endl;
		}
		cout<<endl;
		cout<<"ComplementSeed="<<ed->m_EXTENSION_complementedSeed<<endl;

		#endif

		#ifdef ASSERT
		assert(false);
		#endif

	// else, do a paired-end or single-end lookup if reads are in range.
	}else{

/*
 *
 *                         min                          seed                       peak
 *             min/2       
 *                                      2min
 *   A         ==============
 *   B                      =============================
 *   C                      =============
 */
		// stuff in the reads to appropriate arcs.
		if(!ed->m_EXTENSION_singleEndResolution && ed->m_EXTENSION_readsInRange->size()>0){
			// try to use single-end reads to resolve the repeat.
			// for each read in range, ask them their vertex at position (CurrentPositionOnContig-StartPositionOfReadOnContig)
			// and cumulate the results in
			// ed->m_EXTENSION_readPositions, which is a map<int,vector<int> > if one of the vertices match
			if(ed->m_EXTENSION_readIterator!=ed->m_EXTENSION_readsInRange->end()){
				m_removedUnfitLibraries=false;
				// we received the vertex for that read,
				// now check if it matches one of 
				// the many choices we have
				uint64_t uniqueId=*(ed->m_EXTENSION_readIterator);
				//cout<<"Iterating on reads "<<uniqueId<<endl;
				ExtensionElement*element=ed->getUsedRead(uniqueId);
				int startPosition=element->getPosition();
				int distance=ed->m_EXTENSION_extension->size()-startPosition;

				#ifdef ASSERT
				assert(startPosition<(int)ed->m_extensionCoverageValues->size());
				#endif

				char*theSequence=element->getSequence();
				ed->m_EXTENSION_receivedLength=strlen(theSequence);
				if(distance>(ed->m_EXTENSION_receivedLength-wordSize)){
					// the read is now out-of-range
					//cout<<"out of range Distance="<<distance<<" Length="<<(ed->m_EXTENSION_receivedLength-wordSize)<<endl;
					ed->m_EXTENSION_readsOutOfRange.push_back(uniqueId);
					ed->m_EXTENSION_readIterator++;	
					return;
				}

				char leftStrand=element->getStrand();
				ed->m_EXTENSION_receivedReadVertex=kmerAtPosition(theSequence,distance,wordSize,leftStrand,*colorSpaceMode);
				//cout<<"Vertex is "<<idToWord(ed->m_EXTENSION_receivedReadVertex,wordSize)<<endl;
				// process each edge separately.
				// got a match!
				if(!element->hasPairedRead()){
					ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(distance);	

					ed->m_EXTENSION_readIterator++;
				}else{	
					//cout<<"has paired read"<<endl;
					PairedRead pairedRead=element->getPairedRead();
					uint64_t uniqueReadIdentifier=pairedRead.getUniqueId();
					int library=pairedRead.getLibrary();
					int expectedFragmentLength=m_parameters->getLibraryAverageLength(library);
					int expectedDeviation=m_parameters->getLibraryStandardDeviation(library);
					//bool leftReadIsLeftInThePair=pairedRead.isLeftRead();
					ExtensionElement*extensionElement=ed->getUsedRead(uniqueReadIdentifier);
					if(extensionElement!=NULL){// use to be via readsPositions
						char rightStrand=extensionElement->getStrand();
						int startingPositionOnPath=extensionElement->getPosition();
			
						int observedFragmentLength=(startPosition-startingPositionOnPath)+ed->m_EXTENSION_receivedLength;
						int multiplier=3;
						if(expectedFragmentLength-multiplier*expectedDeviation<=observedFragmentLength 
						&& observedFragmentLength <= expectedFragmentLength+multiplier*expectedDeviation 
				&&( (rightStrand=='F' && leftStrand=='R')
					||(rightStrand=='R' && leftStrand=='F'))
				/*&& coverageOfLeftVertex<maxCoverage*/){
						// it matches!
							//int theDistance=startPosition-startingPositionOnPath+distance;
							
							ed->m_EXTENSION_pairedReadPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(observedFragmentLength);
							ed->m_EXTENSION_pairedLibrariesForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(library);
							ed->m_EXTENSION_pairedReadsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(uniqueId);
						}else{
							//cout<<"Invalid -> Average="<<expectedFragmentLength<<" Deviation="<<expectedDeviation<<" Observed="<<observedFragmentLength<<endl;					
							// remove the right read from the used set
							ed->m_sequencesToFree.push_back(uniqueId);
						}
					}
							
					// add it anyway as a single-end match too!
					ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(distance);

					ed->m_EXTENSION_readIterator++;
				}
			}else{
				//cout<<"DOne iterating."<<endl;
				if(!ed->m_EXTENSION_readsOutOfRange.empty()){
					//cout<<"Removing reads out-of-range"<<endl;
					// remove reads that are no longer in-range.
					for(int i=0;i<(int)ed->m_EXTENSION_readsOutOfRange.size();i++){
						uint64_t uniqueId=ed->m_EXTENSION_readsOutOfRange[i];
						ed->m_EXTENSION_readsInRange->erase(uniqueId);
						__Free(ed->getUsedRead(uniqueId)->getSequence());
					}
					ed->m_EXTENSION_readsOutOfRange.clear();
					return;
				}

				if(!m_removedUnfitLibraries){
					//cout<<"Removing unfit libraries."<<endl;
					removeUnfitLibraries();
					//cout<<"done."<<endl;
					m_removedUnfitLibraries=true;
					return;
				}

				if(!ed->m_sequencesToFree.empty()){
					for(int i=0;i<(int)ed->m_sequencesToFree.size();i++){
						uint64_t uniqueId=ed->m_sequencesToFree[i];
						//cout<<"Removing "<<uniqueId<<endl;
						__Free(ed->getUsedRead(uniqueId)->getSequence());
						ed->removeSequence(uniqueId);
						ed->m_EXTENSION_readsInRange->erase(uniqueId);
					}
					ed->m_sequencesToFree.clear();
					return;
				}

				ed->m_EXTENSION_singleEndResolution=true;

				//cout<<"choose"<<endl;
				int choice=(*oa).choose(ed,&(*chooser),minimumCoverage,(maxCoverage),m_parameters);
				//cout<<"Choose is done."<<endl;
				if(choice!=IMPOSSIBLE_CHOICE){
					#ifdef SHOW_CHOICE
					int count=0;
					for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
						if(ed->m_EXTENSION_coverages->at(i)>1){
							count++;
						}
					}

					if(ed->m_enumerateChoices_outgoingEdges.size()>1){
						cout<<"Choosing..."<<endl;
						inspect(ed,currentVertex);
						cout<<endl;
						cout<<"Selection: "<<choice+1<<endl;
						cout<<endl;
						cout<<endl;
					}
					#endif

					//cout<<"Choosed "<<choice<<endl;
					#ifdef ASSERT
					assert(choice<(int)ed->m_enumerateChoices_outgoingEdges.size());
					#endif

					(*currentVertex)=ed->m_enumerateChoices_outgoingEdges[choice];
					ed->m_EXTENSION_choose=true;
					ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
					ed->m_EXTENSION_directVertexDone=false;
					ed->m_EXTENSION_VertexAssembled_requested=false;
					return;
				}

				ed->m_doChoice_tips_Detected=false;
				m_dfsData->m_doChoice_tips_Initiated=false;
			}
			return;
		}else if(!ed->m_doChoice_tips_Detected && ed->m_EXTENSION_readsInRange->size()>0){
 			//for each entries in ed->m_enumerateChoices_outgoingEdges, do a dfs of max depth 40.
			//if the reached depth is 40, it is not a tip, otherwise, it is.
			int maxDepth=MAX_DEPTH;
			if(!m_dfsData->m_doChoice_tips_Initiated){
				m_dfsData->m_doChoice_tips_i=0;
				m_dfsData->m_doChoice_tips_newEdges.clear();
				m_dfsData->m_doChoice_tips_dfs_initiated=false;
				m_dfsData->m_doChoice_tips_dfs_done=false;
				m_dfsData->m_doChoice_tips_Initiated=true;
				bubbleData->m_BUBBLE_visitedVertices.clear();
				bubbleData->m_visitedVertices.clear();
				bubbleData->m_BUBBLE_visitedVerticesDepths.clear();
				bubbleData->m_coverages.clear();
			bubbleData->m_coverages[(*currentVertex)]=ed->m_currentCoverage;
			}

			if(m_dfsData->m_doChoice_tips_i<(int)ed->m_enumerateChoices_outgoingEdges.size()){
				if(!m_dfsData->m_doChoice_tips_dfs_done){
					if(ed->m_enumerateChoices_outgoingEdges.size()==1){
						m_dfsData->m_doChoice_tips_dfs_done=true;
					}else{
						m_dfsData->depthFirstSearch((*currentVertex),ed->m_enumerateChoices_outgoingEdges[m_dfsData->m_doChoice_tips_i],maxDepth,edgesRequested,vertexCoverageRequested,vertexCoverageReceived,outboxAllocator,
size,theRank,outbox,receivedVertexCoverage,receivedOutgoingEdges,minimumCoverage,edgesReceived);
					}
				}else{
					// keep the edge if it is not a tip.
					if(m_dfsData->m_depthFirstSearch_maxDepth>=TIP_LIMIT){

						// just don't try that strange graph place for now.
						if(m_dfsData->m_depthFirstSearchVisitedVertices.size()==MAX_VERTICES_TO_VISIT){
							/*
							m_doChoice_tips_Detected=true;
							bubbleData->m_doChoice_bubbles_Detected=true;
							return;
							*/
						}
						m_dfsData->m_doChoice_tips_newEdges.push_back(m_dfsData->m_doChoice_tips_i);
						bubbleData->m_visitedVertices.push_back(m_dfsData->m_depthFirstSearchVisitedVertices);
						// store visited vertices for bubble detection purposes.

						bubbleData->m_BUBBLE_visitedVertices.push_back(m_dfsData->m_depthFirstSearchVisitedVertices_vector);
						for(map<uint64_t,int>::iterator i=m_dfsData->m_coverages.begin();
							i!=m_dfsData->m_coverages.end();i++){
							bubbleData->m_coverages[i->first]=i->second;
						}
						bubbleData->m_BUBBLE_visitedVerticesDepths.push_back(m_dfsData->m_depthFirstSearchVisitedVertices_depths);
					}else{
						#ifdef SHOW_PROGRESS_DEBUG
						cout<<"We have a tip "<<m_dfsData->m_depthFirstSearch_maxDepth<<" LIMIT="<<TIP_LIMIT<<"."<<endl;
						#endif
					}
					m_dfsData->m_doChoice_tips_i++;
					m_dfsData->m_doChoice_tips_dfs_initiated=false;
					m_dfsData->m_doChoice_tips_dfs_done=false;
				}
			}else{
				#ifdef SHOW_PROGRESS
				#endif
				// we have a winner with tips investigation.
				if(m_dfsData->m_doChoice_tips_newEdges.size()==1 && ed->m_EXTENSION_readsInRange->size()>0 
		&& ed->m_EXTENSION_readPositionsForVertices[ed->m_enumerateChoices_outgoingEdges[m_dfsData->m_doChoice_tips_newEdges[0]]].size()>0
){
					// tip watchdog!
					// the watchdog watches Ray to be sure he is up to the task!
					TipWatchdog watchdog;
					bool opinion=watchdog.getApproval(ed,m_dfsData,minimumCoverage,
						(*currentVertex),wordSize,bubbleData);
					if(!opinion){
						ed->m_doChoice_tips_Detected=true;
						bubbleData->m_doChoice_bubbles_Detected=false;
						bubbleData->m_doChoice_bubbles_Initiated=false;
						return;
					}

					(*currentVertex)=ed->m_enumerateChoices_outgoingEdges[m_dfsData->m_doChoice_tips_newEdges[0]];
					#ifdef SHOW_PROGRESS
					//cout<<"We have a win after tip elimination: "<<idToWord((*currentVertex),wordSize)<<endl;
					#endif
					ed->m_EXTENSION_choose=true;
					ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
					ed->m_EXTENSION_directVertexDone=false;
					ed->m_EXTENSION_VertexAssembled_requested=false;
					return;
				}else{
					// no luck..., yet.
					ed->m_doChoice_tips_Detected=true;
					bubbleData->m_doChoice_bubbles_Detected=false;
					bubbleData->m_doChoice_bubbles_Initiated=false;
				}
			}
			return;
		// bubbles detection aims polymorphisms and homopolymers stretches.
		}else if(!bubbleData->m_doChoice_bubbles_Detected && ed->m_EXTENSION_readsInRange->size()>0){
			
			bool isGenuineBubble=m_bubbleTool.isGenuineBubble((*currentVertex),&bubbleData->m_BUBBLE_visitedVertices,
				&bubbleData->m_coverages);

			// support indels of 1 as well as mismatch polymorphisms.
			if(isGenuineBubble){

				(*currentVertex)=m_bubbleTool.getTraversalStartingPoint();

				ed->m_EXTENSION_choose=true;
				ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
				ed->m_EXTENSION_directVertexDone=false;
				ed->m_EXTENSION_VertexAssembled_requested=false;
			}
			bubbleData->m_doChoice_bubbles_Detected=true;
			return;
		}

		// no choice possible...
		// do it for the lulz
		if(!ed->m_EXTENSION_complementedSeed /*|| !ed->m_EXTENSION_complementedSeed2*/){
			if(!ed->m_EXTENSION_complementedSeed){
				ed->m_EXTENSION_complementedSeed=true;
			}

			vector<uint64_t> complementedSeed;

			for(int i=ed->m_EXTENSION_extension->size()-1;i>=0;i--){
				complementedSeed.push_back(complementVertex(ed->m_EXTENSION_extension->at(i),wordSize,(*colorSpaceMode)));
			}

			ed->m_EXTENSION_currentPosition=0;
			ed->m_EXTENSION_currentSeed=complementedSeed;
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
			(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];

			ed->resetStructures();

			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_VertexAssembled_requested=false;

		}else{
			storeExtensionAndGetNextOne(ed,theRank,seeds,currentVertex,bubbleData);
		}
	}
}

void SeedExtender::storeExtensionAndGetNextOne(ExtensionData*ed,int theRank,vector<vector<uint64_t> >*seeds,
uint64_t*currentVertex,BubbleData*bubbleData){
	delete m_cache;
	m_cache=new map<uint64_t,int>;

	if(ed->m_EXTENSION_extension->size()>=100){

		#ifdef SHOW_CHOICE
		cout<<"Choosing... (impossible!)"<<endl;
		inspect(ed,currentVertex);
		cout<<"Stopping extension..."<<endl;
		#endif

		int theCurrentSize=ed->m_EXTENSION_extension->size();
		printf("Rank %i reached %i vertices from seed %i (completed)\n",theRank,theCurrentSize,
			ed->m_EXTENSION_currentSeedIndex+1);
		fflush(stdout);
		showMemoryUsage(theRank);

		int a=ed->getAllocator()->getChunkSize()*ed->getAllocator()->getNumberOfChunks();
		printf("Rank %i: database allocation: %i\n",theRank,a);

		int chunks=m_directionsAllocator->getNumberOfChunks();
		int chunkSize=m_directionsAllocator->getChunkSize();
		uint64_t totalBytes=chunks*chunkSize;
		uint64_t kibibytes=totalBytes/1024;
		printf("Rank %i: memory usage for directions is %lu KiB\n",theRank,kibibytes);
		fflush(stdout);

		ed->m_EXTENSION_contigs.push_back(*(ed->m_EXTENSION_extension));
	
		uint64_t id=ed->m_EXTENSION_currentSeedIndex*MAX_NUMBER_OF_MPI_PROCESSES+theRank;
		ed->m_EXTENSION_identifiers.push_back(id);
	}

	ed->resetStructures();
	printf("After\n");
	fflush(stdout);
	showMemoryUsage(theRank);
	int a=ed->getAllocator()->getChunkSize()*ed->getAllocator()->getNumberOfChunks();
	printf("Rank %i: database allocation: %i\n",theRank,a);

	ed->m_EXTENSION_currentSeedIndex++;

	ed->m_EXTENSION_currentPosition=0;
	if(ed->m_EXTENSION_currentSeedIndex<(int)(*seeds).size()){
		ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
		(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
	}

	ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;

	ed->m_EXTENSION_directVertexDone=false;
	ed->m_EXTENSION_complementedSeed=false;
	ed->m_EXTENSION_complementedSeed2=false;
	ed->m_EXTENSION_VertexAssembled_requested=false;
}

void SeedExtender::checkIfCurrentVertexIsAssembled(ExtensionData*ed,StaticVector*outbox,RingAllocator*outboxAllocator,
  int*outgoingEdgeIndex,int*last_value,uint64_t*currentVertex,int theRank,bool*vertexCoverageRequested,int wordSize,
 bool*colorSpaceMode,int size,vector<vector<uint64_t> >*seeds){
	if(!ed->m_EXTENSION_directVertexDone){
		if(!ed->m_EXTENSION_VertexAssembled_requested){
			//cout<<__func__<<endl;
			delete m_dfsData;
			m_dfsData=new DepthFirstSearchData;

			m_receivedDirections.clear();
			if(ed->m_EXTENSION_currentSeedIndex%10==0 and ed->m_EXTENSION_currentPosition==0 and (*last_value)!=ed->m_EXTENSION_currentSeedIndex){
				(*last_value)=ed->m_EXTENSION_currentSeedIndex;
				printf("Rank %i is extending seeds [%i/%i] \n",theRank,(int)ed->m_EXTENSION_currentSeedIndex+1,(int)(*seeds).size());
				fflush(stdout);
				
			}
			ed->m_EXTENSION_VertexAssembled_requested=true;
			uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(2*sizeof(uint64_t));
			message[0]=(uint64_t)(*currentVertex);
			message[1]=0;
			int destination=vertexRank((*currentVertex),size);
			Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_ASK_IS_ASSEMBLED,theRank);
			(*outbox).push_back(aMessage);
			ed->m_EXTENSION_VertexAssembled_received=false;
		}else if(ed->m_EXTENSION_VertexAssembled_received){
			//cout<<__func__<<" direct is done"<<endl;
			ed->m_EXTENSION_reverseVertexDone=false;
			ed->m_EXTENSION_directVertexDone=true;
			ed->m_EXTENSION_VertexMarkAssembled_requested=false;
			(*vertexCoverageRequested)=false;
			ed->m_EXTENSION_VertexAssembled_requested=false;
			if(ed->m_EXTENSION_vertexIsAssembledResult){
				ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
				ed->m_EXTENSION_markedCurrentVertexAsAssembled=false;
				ed->m_EXTENSION_directVertexDone=false;
			}
		}
	}else if(!ed->m_EXTENSION_reverseVertexDone){
		if(!ed->m_EXTENSION_VertexAssembled_requested){
			//cout<<__func__<<" requests reverse"<<endl;
			m_receivedDirections.clear();
			ed->m_EXTENSION_VertexAssembled_requested=true;
			uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(2*sizeof(uint64_t));
			message[0]=(uint64_t)complementVertex((*currentVertex),wordSize,(*colorSpaceMode));
			message[1]=0;
			int destination=vertexRank(message[0],size);
			Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_ASK_IS_ASSEMBLED,theRank);
			(*outbox).push_back(aMessage);
			ed->m_EXTENSION_VertexAssembled_received=false;
		}else if(ed->m_EXTENSION_VertexAssembled_received){
			//cout<<__func__<<" receives reverse"<<endl;
			//m_earlyStoppingTechnology.addDirections(&m_receivedDirections);
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
			ed->m_EXTENSION_markedCurrentVertexAsAssembled=false;
			ed->m_EXTENSION_directVertexDone=false;
		}
	}
}

void SeedExtender::markCurrentVertexAsAssembled(uint64_t*currentVertex,RingAllocator*outboxAllocator,int*outgoingEdgeIndex, 
StaticVector*outbox,int size,int theRank,ExtensionData*ed,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,bool*edgesRequested,
vector<uint64_t>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,int minimumCoverage,OpenAssemblerChooser*oa,bool*colorSpaceMode,int wordSize
){
	if(!ed->m_EXTENSION_directVertexDone){
		if(!(*vertexCoverageRequested)&&(*m_cache).count(*currentVertex)>0){
			//cout<<__func__<<" Requesting cache"<<endl;
			(*vertexCoverageRequested)=true;
			(*vertexCoverageReceived)=true;
			*receivedVertexCoverage=(*m_cache)[*currentVertex];
		}else if(!(*vertexCoverageRequested)){
			(*vertexCoverageRequested)=true;
			(*vertexCoverageReceived)=false;
			//cout<<__func__<<" Requesting"<<endl;
			
			uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(1*sizeof(uint64_t));
			message[0]=(*currentVertex);
			int destination=vertexRank(message[0],size);
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,theRank);
			(*outbox).push_back(aMessage);
		}else if((*vertexCoverageReceived)){
			if(!ed->m_EXTENSION_VertexMarkAssembled_requested){
				//cout<<__func__<<" marking as assembled"<<endl;
				(*m_cache)[*currentVertex]=*receivedVertexCoverage;
				int theCurrentSize=ed->m_EXTENSION_extension->size();
				if(theCurrentSize%10000==0){
					printf("Rank %i reached %i vertices\n",theRank,theCurrentSize);
					fflush(stdout);

					showMemoryUsage(theRank);
					int a=ed->getAllocator()->getChunkSize()*ed->getAllocator()->getNumberOfChunks();
					printf("Rank %i: database allocation: %i\n",theRank,a);
				}

				ed->m_EXTENSION_extension->push_back((*currentVertex));
				ed->m_extensionCoverageValues->push_back(*receivedVertexCoverage);
				ed->m_currentCoverage=(*receivedVertexCoverage);
				#ifdef ASSERT
				assert(ed->m_currentCoverage<=255);
				#endif

				// save wave progress.
				uint64_t waveId=ed->m_EXTENSION_currentSeedIndex*MAX_NUMBER_OF_MPI_PROCESSES+theRank;
				#ifdef ASSERT
				assert((int)(waveId/MAX_NUMBER_OF_MPI_PROCESSES)==ed->m_EXTENSION_currentSeedIndex);
				assert((int)(waveId%MAX_NUMBER_OF_MPI_PROCESSES)==theRank);
				#endif

				int progression=ed->m_EXTENSION_extension->size()-1;
			
				ed->m_EXTENSION_VertexMarkAssembled_requested=true;
				uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(3*sizeof(uint64_t));
				message[0]=(uint64_t)(*currentVertex);
				message[1]=waveId;
				message[2]=progression;
				int destination=vertexRank((*currentVertex),size);
				Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_SAVE_WAVE_PROGRESSION,theRank);
				(*outbox).push_back(aMessage);
				ed->m_EXTENSION_reverseVertexDone=true;
				ed->m_EXTENSION_reads_requested=false;
				
				int maximumLibrarySize=m_parameters->getMaximumDistance()+1000;

				// we don't really need these reads
				if(ed->m_EXTENSION_currentPosition<(int)ed->m_EXTENSION_currentSeed.size()-maximumLibrarySize){
					ed->m_EXTENSION_reads_requested=true;
					ed->m_EXTENSION_reads_received=true;
					ed->m_EXTENSION_receivedReads.clear();
				}

			// get the reads starting at that position.
			}else if(!ed->m_EXTENSION_reads_requested){
				ed->m_EXTENSION_reads_requested=true;
				ed->m_EXTENSION_reads_received=false;
				uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(1*sizeof(uint64_t));
				message[0]=(uint64_t)(*currentVertex);
				int dest=vertexRank((*currentVertex),size);
				//cout<<__func__<<" Requesting reads from "<<dest<<endl;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,dest,RAY_MPI_TAG_REQUEST_READS,theRank);
				(*outbox).push_back(aMessage);
				m_sequenceIndexToCache=0;
				m_sequenceRequested=false;
			}else if(ed->m_EXTENSION_reads_received){
				if(m_sequenceIndexToCache<(int)ed->m_EXTENSION_receivedReads.size()){
					ReadAnnotation annotation=ed->m_EXTENSION_receivedReads[m_sequenceIndexToCache];
					uint64_t uniqueId=annotation.getUniqueId();
					ExtensionElement*anElement=ed->getUsedRead(uniqueId);
					if(anElement!=NULL){
						m_sequenceIndexToCache++;
					}else if(!m_sequenceRequested){
						m_sequenceRequested=true;
						m_sequenceReceived=false;
						int sequenceRank=ed->m_EXTENSION_receivedReads[m_sequenceIndexToCache].getRank();
						#ifdef ASSERT
						assert(sequenceRank>=0);
						assert(sequenceRank<size);
						#endif
						uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(1*sizeof(uint64_t));
						message[0]=ed->m_EXTENSION_receivedReads[m_sequenceIndexToCache].getReadIndex();
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,sequenceRank,RAY_MPI_TAG_REQUEST_READ_SEQUENCE,theRank);
						outbox->push_back(aMessage);
					}else if(m_sequenceReceived){
						ExtensionElement*element=ed->addUsedRead(uniqueId);
						element->setSequence(m_receivedString.c_str(),ed->getAllocator());
						element->setStartingPosition(ed->m_EXTENSION_extension->size()-1);
						element->setStrand(annotation.getStrand());
						ed->m_EXTENSION_readsInRange->insert(uniqueId);
						m_sequenceReceived=false;

						// received paired read too !
						if(ed->m_EXTENSION_pairedRead.getLibrary()!=DUMMY_LIBRARY){
							element->setPairedRead(ed->m_EXTENSION_pairedRead);
						}
	
						m_sequenceIndexToCache++;
						m_sequenceRequested=false;
					}
				}else{
					//cout<<"Received "<<ed->m_EXTENSION_receivedReads.size()<<" sequences."<<endl;
					ed->m_EXTENSION_directVertexDone=true;
					ed->m_EXTENSION_VertexMarkAssembled_requested=false;
					ed->m_EXTENSION_enumerateChoices=false;
					(*edgesRequested)=false;
					ed->m_EXTENSION_markedCurrentVertexAsAssembled=true;
				}
			}
		}
	}
}

SeedExtender::SeedExtender(){
	m_skippedASeed=false;
}

vector<Direction>*SeedExtender::getDirections(){
	return &m_receivedDirections;
}

set<uint64_t>*SeedExtender::getEliminatedSeeds(){
	return &m_eliminatedSeeds;
}

void SeedExtender::constructor(Parameters*parameters,MyAllocator*m_directionsAllocator,ExtensionData*ed){
	m_dfsData=new DepthFirstSearchData;
	m_cache=new map<uint64_t,int>;
	m_ed=ed;
	this->m_directionsAllocator=m_directionsAllocator;
	m_parameters=parameters;
	m_bubbleTool.constructor(parameters);
}

void SeedExtender::inspect(ExtensionData*ed,uint64_t*currentVertex){
	int wordSize=m_parameters->getWordSize();
	cout<<endl;
	cout<<"*****************************************"<<endl;
	cout<<"CurrentVertex="<<idToWord(*currentVertex,wordSize)<<" @"<<ed->m_EXTENSION_extension->size()<<endl;
	#ifdef ASSERT
	assert(ed->m_currentCoverage<=255);
	#endif
	cout<<"Coverage="<<ed->m_currentCoverage<<endl;
	cout<<" # ReadsInRange: "<<ed->m_EXTENSION_readsInRange->size()<<endl;
	cout<<ed->m_enumerateChoices_outgoingEdges.size()<<" choices ";

	cout<<endl;
	for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
		string vertex=idToWord(ed->m_enumerateChoices_outgoingEdges[i],wordSize);
		uint64_t key=ed->m_enumerateChoices_outgoingEdges[i];
		cout<<endl;
		cout<<"Choice #"<<i+1<<endl;
		cout<<"Vertex: "<<vertex<<endl;
		cout<<"Coverage="<<ed->m_EXTENSION_coverages->at(i)<<endl;
		cout<<"New letter: "<<vertex[wordSize-1]<<endl;
		cout<<"Single-end reads: ("<<ed->m_EXTENSION_readPositionsForVertices[key].size()<<")"<<endl;
		for(int j=0;j<(int)ed->m_EXTENSION_readPositionsForVertices[key].size();j++){
			if(j!=0){
				cout<<" ";
			}
			cout<<ed->m_EXTENSION_readPositionsForVertices[key][j];
		}
		cout<<endl;
		cout<<"Paired-end reads: ("<<ed->m_EXTENSION_pairedReadPositionsForVertices[key].size()<<")"<<endl;
		for(int j=0;j<(int)ed->m_EXTENSION_pairedReadPositionsForVertices[key].size();j++){
			if(j!=0)
				cout<<" ";
			cout<<ed->m_EXTENSION_pairedReadPositionsForVertices[key][j];
		}
		cout<<endl;
	}
}

void SeedExtender::removeUnfitLibraries(){
	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		map<int,vector<int> > classifiedValues;
		map<int,vector<uint64_t> > reads;
		uint64_t vertex=m_ed->m_enumerateChoices_outgoingEdges[i];
		for(int j=0;j<(int)m_ed->m_EXTENSION_pairedReadPositionsForVertices[vertex].size();j++){
			int value=m_ed->m_EXTENSION_pairedReadPositionsForVertices[vertex][j];
			int library=m_ed->m_EXTENSION_pairedLibrariesForVertices[vertex][j];
			uint64_t readId=m_ed->m_EXTENSION_pairedReadsForVertices[vertex][j];
			classifiedValues[library].push_back(value);
			reads[library].push_back(readId);
		}

		vector<int> acceptedValues;

		for(map<int,vector<int> >::iterator j=classifiedValues.begin();j!=classifiedValues.end();j++){
			int library=j->first;
			int averageLength=m_parameters->getLibraryAverageLength(j->first);
			int stddev=m_parameters->getLibraryStandardDeviation(j->first);
			int sum=0;
			int n=0;
			for(int k=0;k<(int)j->second.size();k++){
				int val=j->second[k];
				sum+=val;
				n++;
			}
			int mean=sum/n;
			
			if(mean<=averageLength+stddev
			&& mean>=averageLength-stddev){
				for(int k=0;k<(int)j->second.size();k++){
					int val=j->second[k];
					acceptedValues.push_back(val);
				}
			}else if(j->second.size()>10){// to restore reads for a library, we need at least 5
				for(int k=0;k<(int)j->second.size();k++){
					uint64_t uniqueId=reads[library][k];
					//cout<<"Restoring Value="<<j->second[k]<<" Expected="<<averageLength<<" Dev="<<stddev<<" MeanForLibrary="<<mean<<" n="<<n<<endl;
					m_ed->m_sequencesToFree.push_back(uniqueId);
				}
			}
		}
		m_ed->m_EXTENSION_pairedReadPositionsForVertices[vertex]=acceptedValues;
	}
}
