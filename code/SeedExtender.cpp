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

#include <malloc_types.h>
#include <string.h>
#include <StaticVector.h>
#include <TipWatchdog.h>
#include <SeedExtender.h>
#include <Chooser.h>
#include <sstream>
#include <assert.h>
#include <BubbleTool.h>
#include <crypto.h>
using namespace std;

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
		ed->getAllocator()->clear();
		printf("Rank %i is extending seeds [%i/%i] (completed)\n",theRank,(int)(*seeds).size(),(int)(*seeds).size());
		fflush(stdout);
		showMemoryUsage(theRank);
		fflush(stdout);
		ed->m_mode_EXTENSION=false;
		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,RAY_MPI_TAG_EXTENSION_IS_DONE,theRank);
		(*outbox).push_back(aMessage);
		m_cacheAllocator.clear();
		m_cacheForRepeatedReads.clear();
		//m_cacheHashTable.clear();
		m_cacheForListOfReads.clear();

		m_cache.clear();
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
		ed->m_EXTENSION_complementedSeed2=true;
	
		ed->constructor(m_parameters);

	}else if(ed->m_EXTENSION_currentSeedIndex==(int)(*seeds).size()){
		ed->destructor();
		ed->getAllocator()->clear();
		m_cacheAllocator.clear();
		m_cache.clear();

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
			//cout<<"Preset m_EXTENSION_markedCurrentVertexAsAssembled <- false"<<endl;

			ed->m_EXTENSION_reads_requested=false;
			m_messengerInitiated=false;
			
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_VertexMarkAssembled_requested=false;
			(*vertexCoverageRequested)=false;
		}else{
			checkIfCurrentVertexIsAssembled(ed,outbox,outboxAllocator,outgoingEdgeIndex,last_value,
	currentVertex,theRank,vertexCoverageRequested,wordSize,colorSpaceMode,size,seeds);
		}
	}else if(ed->m_EXTENSION_vertexIsAssembledResult && ed->m_EXTENSION_currentPosition==0 && ed->m_EXTENSION_complementedSeed==false){
		//cout<<"Rank "<<theRank<<": Ray Early-Stopping Technology was triggered, Case 1: seed is already processed at p=0."<<endl;
		ed->m_EXTENSION_currentSeedIndex++;// skip the current one.
		ed->m_EXTENSION_currentPosition=0;


		ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
		ed->m_EXTENSION_directVertexDone=false;
		ed->m_EXTENSION_VertexAssembled_requested=false;
		if(ed->m_EXTENSION_currentSeedIndex<(int)(*seeds).size()){
			ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
			(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
		}
		ed->m_EXTENSION_complementedSeed=false;
		ed->m_EXTENSION_complementedSeed2=true;

	}else if(!ed->m_EXTENSION_markedCurrentVertexAsAssembled){
		markCurrentVertexAsAssembled(currentVertex,outboxAllocator,outgoingEdgeIndex,outbox,
size,theRank,ed,vertexCoverageRequested,vertexCoverageReceived,receivedVertexCoverage,
repeatedLength,maxCoverage,edgesRequested,receivedOutgoingEdges,chooser,bubbleData,minimumCoverage,
oa,colorSpaceMode,wordSize,seeds);
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
		//cout<<__func__<<" init"<<endl;
		ed->m_EXTENSION_coverages->clear();
		ed->m_enumerateChoices_outgoingEdges.clear();
		(*edgesReceived)=true;
		(*edgesRequested)=true;
		ed->m_EXTENSION_currentPosition++;
		(*vertexCoverageRequested)=false;
		(*outgoingEdgeIndex)=0;
	}else if((*edgesReceived)){
		if((*outgoingEdgeIndex)<(int)(*receivedOutgoingEdges).size()){
			uint64_t kmer=(*receivedOutgoingEdges)[(*outgoingEdgeIndex)];
			// get the coverage of these.
			if(!(*vertexCoverageRequested)&&(m_cache).find(kmer,false)!=NULL){
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=true;
				(*receivedVertexCoverage)=*(m_cache.find(kmer,false)->getValue());

				#ifdef ASSERT
				assert((*receivedVertexCoverage)<=m_parameters->getMaximumAllowedCoverage());
				#endif
			}else if(!(*vertexCoverageRequested)){
				uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(1*sizeof(uint64_t));
				message[0]=(uint64_t)kmer;
				int dest=vertexRank(message[0],size,wordSize);
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,dest,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,theRank);
				(*outbox).push_back(aMessage);
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=false;
				(*receivedVertexCoverage)=-1;
			}else if((*vertexCoverageReceived)){
				bool inserted;
				*((m_cache.insert(kmer,&m_cacheAllocator,&inserted))->getValue())=*receivedVertexCoverage;
				(*outgoingEdgeIndex)++;
				(*vertexCoverageRequested)=false;
				#ifdef ASSERT
				assert((*receivedVertexCoverage)<=m_parameters->getMaximumAllowedCoverage());
				#endif
				int coverageValue=*receivedVertexCoverage;
				if(coverageValue>1){
					ed->m_EXTENSION_coverages->push_back((*receivedVertexCoverage));
					ed->m_enumerateChoices_outgoingEdges.push_back(kmer);
				}
			}
		}else{
			receivedOutgoingEdges->clear();
			//cout<<__func__<<" Got "<<ed->m_enumerateChoices_outgoingEdges.size()<<" choices."<<endl;
			ed->m_EXTENSION_enumerateChoices=true;
			ed->m_EXTENSION_choose=false;
			ed->m_EXTENSION_singleEndResolution=false;
			ed->m_EXTENSION_readIterator=ed->m_EXTENSION_readsInRange->begin();
			ed->m_EXTENSION_readLength_done=false;
			ed->m_EXTENSION_readPositionsForVertices.clear();
			ed->m_EXTENSION_pairedReadPositionsForVertices.clear();
			ed->m_EXTENSION_pairedLibrariesForVertices.clear();
			ed->m_EXTENSION_pairedReadsForVertices.clear();

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
	if(m_expiredReads.count(ed->m_EXTENSION_currentPosition)>0){
		for(int i=0;i<(int)m_expiredReads[ed->m_EXTENSION_currentPosition].size();i++){
			uint64_t uniqueId=m_expiredReads[ed->m_EXTENSION_currentPosition][i];
			//cout<<"Expires: "<<uniqueId<<" Position="<<ed->m_EXTENSION_currentPosition<<endl;
			ed->m_EXTENSION_readsInRange->erase(uniqueId);

			// free the sequence
			ExtensionElement*element=ed->getUsedRead(uniqueId);
			if(element==NULL){
				continue;
			}
			#ifdef ASSERT
			assert(element!=NULL);
			#endif
			char*read=element->getSequence();
			if(read==NULL){
				continue;
			}
			#ifdef ASSERT
			assert(read!=NULL);
			#endif
			
			element->removeSequence();
			ed->getAllocator()->free(read,strlen(read)+1);
		}
		m_expiredReads.erase(ed->m_EXTENSION_currentPosition);
		ed->m_EXTENSION_readIterator=ed->m_EXTENSION_readsInRange->begin();
		return;
	}

	// if there is only one choice and reads supporting it
	if(ed->m_enumerateChoices_outgoingEdges.size()==1&&ed->m_EXTENSION_readsInRange->size()>0){
		(*currentVertex)=ed->m_enumerateChoices_outgoingEdges[0]; 
		ed->m_EXTENSION_choose=true; 
		ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false; 
		ed->m_EXTENSION_directVertexDone=false; 
		ed->m_EXTENSION_VertexAssembled_requested=false; 
		return; 
	}

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
		cout<<"Choices: ";
		for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
			cout<<" "<<idToWord(ed->m_enumerateChoices_outgoingEdges[i],wordSize);
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
				#ifdef ASSERT
				assert(element!=NULL);
				#endif
				int startPosition=element->getPosition();
				int distance=ed->m_EXTENSION_extension->size()-startPosition+element->getStrandPosition();

				//int coverageForRightRead=ed->m_extensionCoverageValues->at(startPosition);
				int repeatValueForRightRead=ed->m_repeatedValues->at(startPosition);
				#ifdef ASSERT
				assert(startPosition<(int)ed->m_extensionCoverageValues->size());
				#endif

				int repeatThreshold=100;

				char*theSequence=element->getSequence();
				#ifdef ASSERT
				assert(theSequence!=NULL);
				#endif
				ed->m_EXTENSION_receivedLength=strlen(theSequence);
				if(distance>(ed->m_EXTENSION_receivedLength-wordSize)){
					cout<<"OutOfRange UniqueId="<<uniqueId<<" Length="<<strlen(theSequence)<<" StartPosition="<<element->getPosition()<<" CurrentPosition="<<ed->m_EXTENSION_extension->size()-1<<" StrandPosition="<<element->getStrandPosition()<<endl;
					#ifdef ASSERT
					assert(false);
					#endif
					// the read is now out-of-range
					//cout<<"out of range Distance="<<distance<<" Length="<<(ed->m_EXTENSION_receivedLength-wordSize)<<endl;
					ed->m_EXTENSION_readIterator++;	
					return;
				}

				char theRightStrand=element->getStrand();
				#ifdef ASSERT
				assert(theRightStrand=='R'||theRightStrand=='F');
				assert(element->getType()==TYPE_SINGLE_END||element->getType()==TYPE_RIGHT_END||element->getType()==TYPE_LEFT_END);
				#endif
				//bool isARightSequence=(theRightStrand=='R' && element->isRightEnd())||(theRightStrand=='F' && element->isLeftEnd());
				//isARightSequence=true;// TODO: remove

				ed->m_EXTENSION_receivedReadVertex=kmerAtPosition(theSequence,distance,wordSize,theRightStrand,*colorSpaceMode);
				//cout<<"Vertex is "<<idToWord(ed->m_EXTENSION_receivedReadVertex,wordSize)<<endl;
				// process each edge separately.
				// got a match!
				//cout<<"Strand="<<theRightStrand<<" Type="<<element->getType()<<" isRight="<<isARightSequence<<endl;

				if(!element->hasPairedRead()){
					if(repeatValueForRightRead<repeatThreshold){
						ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(distance);	
					}

					ed->m_EXTENSION_readIterator++;
				}else{
					PairedRead*pairedRead=element->getPairedRead();
					uint64_t uniqueReadIdentifier=pairedRead->getUniqueId();
					int diff=uniqueId-uniqueReadIdentifier;
					if(uniqueReadIdentifier>uniqueId){
						diff=uniqueReadIdentifier-uniqueId;
					}
					//cout<<"has paired read rightStrand="<<theRightStrand<<" RightId="<<uniqueId<<" LeftId="<<uniqueReadIdentifier<<" Diff="<<diff<<endl;
					int library=pairedRead->getLibrary();
					int expectedFragmentLength=m_parameters->getLibraryAverageLength(library);
					int expectedDeviation=m_parameters->getLibraryStandardDeviation(library);
					//bool leftReadIsLeftInThePair=pairedRead.isLeftRead();
					ExtensionElement*extensionElement=ed->getUsedRead(uniqueReadIdentifier);
					if(extensionElement!=NULL){// use to be via readsPositions
						char theLeftStrand=extensionElement->getStrand();
						int startingPositionOnPath=extensionElement->getPosition();


						//int coverageForLeftRead=ed->m_extensionCoverageValues->at(startingPositionOnPath);
						int repeatLengthForLeftRead=ed->m_repeatedValues->at(startingPositionOnPath);
						int observedFragmentLength=(startPosition-startingPositionOnPath)+ed->m_EXTENSION_receivedLength+extensionElement->getStrandPosition()-element->getStrandPosition();
						//cout<<"Observed="<<observedFragmentLength<<endl;
						int multiplier=3;

						//int theDistance=startPosition-startingPositionOnPath;
						if(expectedFragmentLength-multiplier*expectedDeviation<=observedFragmentLength 
						&& observedFragmentLength <= expectedFragmentLength+multiplier*expectedDeviation 
				&&( (theLeftStrand=='F' && theRightStrand=='R')
					||(theLeftStrand=='R' && theRightStrand=='F'))
				// the bridging pair is meaningless if both start in repeats
				&&repeatLengthForLeftRead<repeatThreshold){
						// it matches!
							//int theDistance=startPosition-startingPositionOnPath+distance;
							
							//cout<<"Found element: LeftStrand="<<theLeftStrand<<" RightStrand="<<theRightStrand<<" LeftType="<<extensionElement->getType()<<" RightType="<<element->getType()<<endl;
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
					if(repeatValueForRightRead<repeatThreshold){
						ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(distance);
					}

					ed->m_EXTENSION_readIterator++;
				}
			}else{
				//cout<<"DOne iterating."<<endl;

				if(!m_removedUnfitLibraries){
					//cout<<"Removing unfit libraries."<<endl;
					removeUnfitLibraries();
					//cout<<"done."<<endl;
					m_removedUnfitLibraries=true;
					//cout<<"PairedReadsWithoutMate="<<m_ed->m_pairedReadsWithoutMate->size()<<endl;
					setFreeUnmatedPairedReads();
					return;
				}

				// reads will be set free in 3 cases:
				//
				// 1. the distance did not match for a pair
				// 2. the read has not met its mate
				// 3. the library population indicates a wrong placement
				if(!ed->m_sequencesToFree.empty()){
					for(int i=0;i<(int)ed->m_sequencesToFree.size();i++){
						uint64_t uniqueId=ed->m_sequencesToFree[i];
						//cout<<"Removing "<<uniqueId<<"  now="<<m_ed->m_EXTENSION_extension->size()-1<<endl;
						m_ed->m_pairedReadsWithoutMate->erase(uniqueId);
						// free the sequence
						ExtensionElement*element=ed->getUsedRead(uniqueId);
						#ifdef ASSERT
						if(element==NULL){
							cout<<"element "<<uniqueId<<" not found now="<<m_ed->m_EXTENSION_extension->size()-1<<""<<endl;
						}
						assert(element!=NULL);
						#endif
						char*read=element->getSequence();
						if(read!=NULL){
							#ifdef ASSERT
							assert(read!=NULL);
							#endif
							element->removeSequence();
							ed->getAllocator()->free(read,strlen(read)+1);
						}

						// remove it
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
			int maxDepth=2*m_parameters->getWordSize();
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
size,theRank,outbox,receivedVertexCoverage,receivedOutgoingEdges,minimumCoverage,edgesReceived,wordSize);
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
		if(!ed->m_EXTENSION_complementedSeed || !ed->m_EXTENSION_complementedSeed2){
			vector<uint64_t> complementedSeed;

			for(int i=ed->m_EXTENSION_extension->size()-1;i>=0;i--){
				complementedSeed.push_back(complementVertex(ed->m_EXTENSION_extension->at(i),wordSize,(*colorSpaceMode)));
			}

			ed->m_EXTENSION_currentPosition=0;
			ed->m_EXTENSION_currentSeed=complementedSeed;
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
			(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];

			ed->resetStructures();
			m_matesToMeet.clear();

			m_cacheAllocator.clear();
			m_cache.clear();
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_VertexAssembled_requested=false;

			if(!ed->m_EXTENSION_complementedSeed){
				ed->m_EXTENSION_complementedSeed=true;
				if(ed->m_EXTENSION_currentSeed.size()<1000){
					ed->m_EXTENSION_complementedSeed2=false;
				}
			}else if(!ed->m_EXTENSION_complementedSeed2){
				ed->m_EXTENSION_complementedSeed2=true;
			}
		}else{
			storeExtensionAndGetNextOne(ed,theRank,seeds,currentVertex,bubbleData);
		}
	}
}

void SeedExtender::storeExtensionAndGetNextOne(ExtensionData*ed,int theRank,vector<vector<uint64_t> >*seeds,
uint64_t*currentVertex,BubbleData*bubbleData){
	if(ed->m_EXTENSION_extension->size()>=100){

		#ifdef SHOW_CHOICE
		cout<<"Choosing... (impossible!)"<<endl;
		inspect(ed,currentVertex);
		cout<<"Stopping extension..."<<endl;
		#endif

		printExtensionStatus(currentVertex);
		cout<<"Rank "<<theRank<<" (extension done)"<<endl;
		ed->m_EXTENSION_contigs.push_back(*(ed->m_EXTENSION_extension));
	
		uint64_t id=getPathUniqueId(theRank,ed->m_EXTENSION_currentSeedIndex);
		ed->m_EXTENSION_identifiers.push_back(id);
	}

	ed->resetStructures();
	m_matesToMeet.clear();

	m_cacheAllocator.clear();
	m_cache.clear();

	fflush(stdout);
	showMemoryUsage(theRank);
/*
	int a=ed->getAllocator()->getChunkSize()*ed->getAllocator()->getNumberOfChunks();
	printf("Rank %i: database allocation: %i\n",theRank,a);
*/

	ed->m_EXTENSION_currentSeedIndex++;

	ed->m_EXTENSION_currentPosition=0;
	if(ed->m_EXTENSION_currentSeedIndex<(int)(*seeds).size()){
		ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
		(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
	}

	ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;

	ed->m_EXTENSION_directVertexDone=false;
	ed->m_EXTENSION_complementedSeed=false;
	ed->m_EXTENSION_complementedSeed2=true;
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
			int destination=vertexRank((*currentVertex),size,wordSize);
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
				//cout<<"1289312 m_EXTENSION_markedCurrentVertexAsAssembled <- false"<<endl;
				ed->m_EXTENSION_directVertexDone=false;
				ed->m_EXTENSION_reads_requested=false;
				m_messengerInitiated=false;
			
			}
		}
	}else if(!ed->m_EXTENSION_reverseVertexDone){
/*
		if(!ed->m_EXTENSION_VertexAssembled_requested){
			//cout<<__func__<<" requests reverse"<<endl;
			m_receivedDirections.clear();
			ed->m_EXTENSION_VertexAssembled_requested=true;
			uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(2*sizeof(uint64_t));
			message[0]=(uint64_t)complementVertex((*currentVertex),wordSize,(*colorSpaceMode));
			message[1]=0;
			int destination=vertexRank(message[0],size,wordSize);
			Message aMessage(message,2,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_ASK_IS_ASSEMBLED,theRank);
			(*outbox).push_back(aMessage);
			ed->m_EXTENSION_VertexAssembled_received=false;
		}else if(ed->m_EXTENSION_VertexAssembled_received){
*/
			//cout<<__func__<<" receives reverse"<<endl;
			//m_earlyStoppingTechnology.addDirections(&m_receivedDirections);
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
			ed->m_EXTENSION_markedCurrentVertexAsAssembled=false;
			//cout<<"12312 m_EXTENSION_markedCurrentVertexAsAssembled <- false"<<endl;
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_reads_requested=false;
			m_messengerInitiated=false;
		//}
	}
}

void SeedExtender::markCurrentVertexAsAssembled(uint64_t*currentVertex,RingAllocator*outboxAllocator,int*outgoingEdgeIndex, 
StaticVector*outbox,int size,int theRank,ExtensionData*ed,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,bool*edgesRequested,
vector<uint64_t>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,int minimumCoverage,OpenAssemblerChooser*oa,bool*colorSpaceMode,int wordSize,vector<vector<uint64_t> >*seeds
){
	if(!m_messengerInitiated){
		*edgesRequested=false;
		m_pickedInformation=false;
		int theCurrentSize=ed->m_EXTENSION_extension->size();
		if(theCurrentSize%10000==0){
			if(theCurrentSize==0 && !ed->m_EXTENSION_complementedSeed){
				printf("Rank %i starts on a seed, length is %i [%i/%i]\n",theRank,
				(int)ed->m_EXTENSION_currentSeed.size(),
					ed->m_EXTENSION_currentSeedIndex,(int)(*seeds).size());
				fflush(stdout);
			}
			printExtensionStatus(currentVertex);
		}
		m_messengerInitiated=true;

		uint64_t waveId=getPathUniqueId(theRank,ed->m_EXTENSION_currentSeedIndex);
		// save wave progress.
		#ifdef ASSERT
		assert((int)getIdFromPathUniqueId(waveId)==ed->m_EXTENSION_currentSeedIndex);
		assert((int)getRankFromPathUniqueId(waveId)==theRank);
		assert(theRank<size);
		#endif

		int progression=ed->m_EXTENSION_extension->size()-1;
		uint64_t vertex=*currentVertex;
		m_vertexMessenger.constructor(vertex,waveId,progression,&m_matesToMeet,m_inbox,outbox,outboxAllocator,m_parameters);
	}else if(!m_vertexMessenger.isDone()){
		m_vertexMessenger.work();
	}else if(!m_pickedInformation){
		m_pickedInformation=true;
		m_sequenceIndexToCache=0;
		ed->m_EXTENSION_receivedReads=m_vertexMessenger.getReadAnnotations();
		*receivedVertexCoverage=m_vertexMessenger.getCoverageValue();
		ed->m_currentCoverage=*receivedVertexCoverage;
		bool inserted;
		*((m_cache.insert(*currentVertex,&m_cacheAllocator,&inserted))->getValue())=ed->m_currentCoverage;
		uint64_t compactEdges=m_vertexMessenger.getEdges();
		*receivedOutgoingEdges=_getOutgoingEdges(*currentVertex,compactEdges,m_parameters->getWordSize());
		ed->m_EXTENSION_extension->push_back((*currentVertex));
		ed->m_extensionCoverageValues->push_back(*receivedVertexCoverage);

		if(ed->m_currentCoverage<m_parameters->getMaxCoverage()){
			m_repeatLength=0;
		}else{
			m_repeatLength++;
		}

		#ifdef ASSERT
		assert(ed->m_currentCoverage<=m_parameters->getMaximumAllowedCoverage());
		#endif

		ed->m_repeatedValues->push_back(m_repeatLength);

		m_sequenceRequested=false;
	}else{
		if(m_sequenceIndexToCache<(int)ed->m_EXTENSION_receivedReads.size()){
			ReadAnnotation annotation=ed->m_EXTENSION_receivedReads[m_sequenceIndexToCache];
			uint64_t uniqueId=annotation.getUniqueId();
			ExtensionElement*anElement=ed->getUsedRead(uniqueId);

			if(anElement!=NULL){
				m_sequenceIndexToCache++;
			}else if(!m_sequenceRequested
				&&m_cacheForRepeatedReads.find(uniqueId,false)!=NULL){
				//cout<<"Retrieving from cache "<<uniqueId<<endl;
				char buffer[4000];
				SplayNode<uint64_t,Read>*node=m_cacheForRepeatedReads.find(uniqueId,false);
				#ifdef ASSERT
				assert(node!=NULL);
				#endif
				node->getValue()->getSeq(buffer);
				m_receivedString=buffer;
				PairedRead*pr=node->getValue()->getPairedRead();

				PairedRead dummy;
				dummy.constructor(0,0,DUMMY_LIBRARY);
				if(pr==NULL){
					pr=&dummy;
				}

				#ifdef ASSERT
				assert(pr!=NULL);
				#endif
				ed->m_EXTENSION_pairedRead=*pr;
				m_sequenceRequested=true;
				m_sequenceReceived=true;
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

				bool addRead=true;
				int startPosition=ed->m_EXTENSION_extension->size()-1;
				int readLength=m_receivedString.length();
				int position=startPosition;
				int wordSize=m_parameters->getWordSize();
				int positionOnStrand=annotation.getPositionOnStrand();

				int availableLength=readLength-positionOnStrand;
				if(availableLength<=wordSize){
					addRead=false;
				}

				// don't add it up if its is marked on a repeated vertex and
				// its mate was not seen yet.
				
				if(ed->m_currentCoverage>=3*m_parameters->getPeakCoverage()){
					// the vertex is repeated
					if(ed->m_EXTENSION_pairedRead.getLibrary()!=DUMMY_LIBRARY){
						uint64_t mateId=ed->m_EXTENSION_pairedRead.getUniqueId();
						// the mate is required to allow proper placement
						if(ed->getUsedRead(mateId)==NULL){
							addRead=false;
							//cout<<"Not using read: coverage="<<ed->m_currentCoverage<<" peak="<<m_parameters->getPeakCoverage()<<endl;
						}
					}
				}

				if(addRead){
					m_matesToMeet.erase(uniqueId);
					ExtensionElement*element=ed->addUsedRead(uniqueId);
					element->setSequence(m_receivedString.c_str(),ed->getAllocator());
					element->setStartingPosition(startPosition);
					element->setStrand(annotation.getStrand());
					element->setStrandPosition(annotation.getPositionOnStrand());
					element->setType(ed->m_readType);
					ed->m_EXTENSION_readsInRange->insert(uniqueId);

					#ifdef ASSERT
					assert(readLength==(int)strlen(element->getSequence()));
					#endif
		
					int expiryPosition=position+readLength-positionOnStrand-wordSize;
					m_expiredReads[expiryPosition].push_back(uniqueId);
					//cout<<"Read="<<uniqueId<<" Length="<<readLength<<" WordSize="<<wordSize<<" PositionOnStrand="<<positionOnStrand<<" Position="<<position<<" ExpiryPosition="<<expiryPosition<<endl;
					// received paired read too !
					if(ed->m_EXTENSION_pairedRead.getLibrary()!=DUMMY_LIBRARY){
						element->setPairedRead(ed->m_EXTENSION_pairedRead);
						uint64_t mateId=ed->m_EXTENSION_pairedRead.getUniqueId();
						if(ed->getUsedRead(mateId)==NULL){// the mate has not shown up yet
							ed->m_pairedReadsWithoutMate->insert(uniqueId);

							int library=ed->m_EXTENSION_pairedRead.getLibrary();
							int expectedFragmentLength=m_parameters->getLibraryAverageLength(library);
							int expectedDeviation=m_parameters->getLibraryStandardDeviation(library);
							int expiration=startPosition+expectedFragmentLength+3*expectedDeviation;

							//cout<<"adding expiration Now="<<startPosition<<" Expiration="<<expiration<<" Id="<<uniqueId<<endl;
							(*ed->m_expirations)[expiration].push_back(uniqueId);
		
							m_matesToMeet.insert(mateId);
						}else{ // the mate has shown up already and was waiting
							ed->m_pairedReadsWithoutMate->erase(mateId);
						}
					}
				}

				m_sequenceIndexToCache++;
				m_sequenceRequested=false;
			}
		}else{
/*
			int position=ed->m_EXTENSION_extension->size()-1;
			cout<<"Rank "<<m_parameters->getRank()<<" Vertex: "<<idToWord(*currentVertex,m_parameters->getWordSize())<<
				" Coverage: "<<ed->m_currentCoverage<<" ReadsInRange: "<<ed->m_EXTENSION_readsInRange->size()<<
				" MatesToMeet: "<<m_matesToMeet.size()<<
				" Position: "<<position<<" Received "<<ed->m_EXTENSION_receivedReads.size()<<" sequences.";
			if(ed->m_currentCoverage>=3*m_parameters->getPeakCoverage()){
				cout<<" REPEAT";
			}
			cout<<endl;
*/

			ed->m_EXTENSION_directVertexDone=true;
			ed->m_EXTENSION_VertexMarkAssembled_requested=false;
			ed->m_EXTENSION_enumerateChoices=false;
			(*edgesRequested)=false;
			ed->m_EXTENSION_markedCurrentVertexAsAssembled=true;
			//cout<<"m_EXTENSION_markedCurrentVertexAsAssembled <- true"<<endl;
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

void SeedExtender::constructor(Parameters*parameters,MyAllocator*m_directionsAllocator,ExtensionData*ed,
	GridTable*subgraph,StaticVector*inbox){
	m_cacheForRepeatedReads.constructor();
	//m_cacheHashTable.constructor();
	m_cacheForListOfReads.constructor();
	ostringstream prefixFull;
	m_parameters=parameters;
	prefixFull<<m_parameters->getMemoryPrefix()<<"_SeedExtender";
	m_cacheAllocator.constructor(4194304,RAY_MALLOC_TYPE_SEED_EXTENDER_CACHE);
	m_inbox=inbox;
	m_subgraph=subgraph;
	m_dfsData=new DepthFirstSearchData;
	m_cache.constructor();
	m_ed=ed;
	this->m_directionsAllocator=m_directionsAllocator;
	m_bubbleTool.constructor(parameters);
}

void SeedExtender::inspect(ExtensionData*ed,uint64_t*currentVertex){
	int wordSize=m_parameters->getWordSize();
	cout<<endl;
	cout<<"*****************************************"<<endl;
	cout<<"CurrentVertex="<<idToWord(*currentVertex,wordSize)<<" @"<<ed->m_EXTENSION_extension->size()<<endl;
	#ifdef ASSERT
	assert(ed->m_currentCoverage<=m_parameters->getMaximumAllowedCoverage());
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
			
			int minimumNumberOfBridges=2;

			if(averageLength>=5000){
				minimumNumberOfBridges=4;
			}

			if(
			(mean<=averageLength+stddev&& mean>=averageLength-stddev)){
				if(n>=minimumNumberOfBridges){// required links
					for(int k=0;k<(int)j->second.size();k++){
						int val=j->second[k];
						acceptedValues.push_back(val);
					}
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

void SeedExtender::setFreeUnmatedPairedReads(){
	if(m_ed->m_expirations->count(m_ed->m_EXTENSION_extension->size())==0){
		return;
	}
	vector<uint64_t>*expired=&(*m_ed->m_expirations)[m_ed->m_EXTENSION_extension->size()];
	for(int i=0;i<(int)expired->size();i++){
		uint64_t readId=expired->at(i);
		if(m_ed->m_pairedReadsWithoutMate->count(readId)>0){
			m_ed->m_sequencesToFree.push_back(readId); // RECYCLING IS desactivated
			//cout<<"Expired: Now="<<m_ed->m_EXTENSION_extension->size()-1<<" Id="<<readId<<endl;
		}
	}
	m_ed->m_expirations->erase(m_ed->m_EXTENSION_extension->size());
}

void SeedExtender::showReadsInRange(){
	cout<<"Reads in range ("<<m_ed->m_EXTENSION_readsInRange->size()<<"):";
	for(set<uint64_t>::iterator i=m_ed->m_EXTENSION_readsInRange->begin();
		i!=m_ed->m_EXTENSION_readsInRange->end();i++){
		cout<<" "<<*i;
	}
	cout<<endl;
	cout.flush();
}

void SeedExtender::printExtensionStatus(uint64_t*currentVertex){
	int theRank=m_parameters->getRank();
	int theCurrentSize=m_ed->m_EXTENSION_extension->size();
	now();
	// stop the infinite loop
	if(m_ed->m_EXTENSION_extension->size()>200000){
		cout<<"Error: Infinite loop"<<endl;
		exit(0);
	}
	printf("Rank %i reached %i vertices (%s) from seed %i\n",theRank,theCurrentSize,
		idToWord(*currentVertex,m_parameters->getWordSize()).c_str(),
		m_ed->m_EXTENSION_currentSeedIndex+1);

	fflush(stdout);
	showMemoryUsage(theRank);
	showReadsInRange();

	int chunks=m_directionsAllocator->getNumberOfChunks();
	int chunkSize=m_directionsAllocator->getChunkSize();
	uint64_t totalBytes=chunks*chunkSize;
	uint64_t kibibytes=totalBytes/1024;
	printf("Rank %i: memory usage for directions is %lu KiB\n",theRank,kibibytes);
	fflush(stdout);

	int readsInCache=m_cacheForRepeatedReads.size();
	//int hashEntries=m_cacheHashTable.size();
	int listsInCache=m_cacheForListOfReads.size();
	printf("Rank %i: lists of reads in cache: %i\n",theRank,listsInCache);
	printf("Rank %i: reads in cache: %i\n",theRank,readsInCache);
	fflush(stdout);
}


