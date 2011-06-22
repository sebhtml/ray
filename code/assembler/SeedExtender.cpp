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

#define SHOW_IMPOSSIBLE_CHOICE

#include <core/constants.h>
#include <memory/malloc_types.h>
#include <string.h>
#include <structures/StaticVector.h>
#include <assembler/TipWatchdog.h>
#include <assembler/SeedExtender.h>
#include <assembler/Chooser.h>
#include <sstream>
#include <assert.h>
#include <assembler/BubbleTool.h>
#include <cryptography/crypto.h>
using namespace std;

void SeedExtender::extendSeeds(vector<vector<Kmer> >*seeds,ExtensionData*ed,int theRank,StaticVector*outbox,
  Kmer*currentVertex,FusionData*fusionData,RingAllocator*outboxAllocator,bool*edgesRequested,int*outgoingEdgeIndex,
int*last_value,bool*vertexCoverageRequested,int wordSize,int size,bool*vertexCoverageReceived,
int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,vector<Kmer>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,
int minimumCoverage,OpenAssemblerChooser*oa,bool*edgesReceived,int*m_mode){
	if(ed->m_EXTENSION_currentSeedIndex==(int)(*seeds).size()){
		ed->destructor();
		ed->getAllocator()->clear();
		m_cacheAllocator.clear();
		m_cache.clear();

		printf("Rank %i is extending seeds [%i/%i] (completed)\n",theRank,(int)(*seeds).size(),(int)(*seeds).size());
		double ratio=(0.0+m_extended)/seeds->size()*100.0;
		printf("Rank %i extended %i seeds out of %i (%.2f%%)\n",theRank,m_extended,(int)seeds->size(),ratio);
		fflush(stdout);

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(theRank);
		}

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
	}else if(!ed->m_EXTENSION_initiated){
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

			ed->m_EXTENSION_reads_requested=false;
			m_messengerInitiated=false;
			
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_VertexMarkAssembled_requested=false;
			(*vertexCoverageRequested)=false;
		}else{
			checkIfCurrentVertexIsAssembled(ed,outbox,outboxAllocator,outgoingEdgeIndex,last_value,
	currentVertex,theRank,vertexCoverageRequested,wordSize,size,seeds);
		}
	}else if(ed->m_EXTENSION_vertexIsAssembledResult && ed->m_EXTENSION_currentPosition==0 && ed->m_EXTENSION_complementedSeed==false){
		printf("Rank %i skips a seed, length is %i [%i/%i]\n",theRank,
			(int)ed->m_EXTENSION_currentSeed.size(),
			ed->m_EXTENSION_currentSeedIndex,(int)(*seeds).size());

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
oa,wordSize,seeds);
	}else if(!ed->m_EXTENSION_enumerateChoices){
		enumerateChoices(edgesRequested,ed,edgesReceived,outboxAllocator,outgoingEdgeIndex,outbox,
		currentVertex,theRank,vertexCoverageRequested,receivedOutgoingEdges,
		vertexCoverageReceived,size,receivedVertexCoverage,chooser,wordSize);
	}else if(!ed->m_EXTENSION_choose){
		doChoice(outboxAllocator,outgoingEdgeIndex,outbox,currentVertex,bubbleData,theRank,wordSize,
	ed,minimumCoverage,*maxCoverage,oa,chooser,seeds,
edgesRequested,vertexCoverageRequested,vertexCoverageReceived,size,receivedVertexCoverage,edgesReceived,
receivedOutgoingEdges);
	}
}

// upon successful completion, ed->m_EXTENSION_coverages and ed->m_enumerateChoices_outgoingEdges are
// populated variables.
void SeedExtender::enumerateChoices(bool*edgesRequested,ExtensionData*ed,bool*edgesReceived,RingAllocator*outboxAllocator,
	int*outgoingEdgeIndex,StaticVector*outbox,
Kmer*currentVertex,int theRank,bool*vertexCoverageRequested,vector<Kmer>*receivedOutgoingEdges,
bool*vertexCoverageReceived,int size,int*receivedVertexCoverage,Chooser*chooser,int wordSize
){
	if(!(*edgesRequested)){
		ed->m_EXTENSION_coverages->clear();
		ed->m_enumerateChoices_outgoingEdges.clear();
		(*edgesReceived)=true;
		(*edgesRequested)=true;
		ed->m_EXTENSION_currentPosition++;
		(*vertexCoverageRequested)=false;
		(*outgoingEdgeIndex)=0;
	}else if((*edgesReceived)){
		if((*outgoingEdgeIndex)<(int)(*receivedOutgoingEdges).size()){
			Kmer kmer=(*receivedOutgoingEdges)[(*outgoingEdgeIndex)];
			// get the coverage of these.
			if(!(*vertexCoverageRequested)&&(m_cache).find(kmer,false)!=NULL){
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=true;
				(*receivedVertexCoverage)=*(m_cache.find(kmer,false)->getValue());

				#ifdef ASSERT
				assert((*receivedVertexCoverage)<=m_parameters->getMaximumAllowedCoverage());
				#endif
			}else if(!(*vertexCoverageRequested)){
				uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(KMER_U64_ARRAY_SIZE*sizeof(uint64_t));
				int bufferPosition=0;
				kmer.pack(message,&bufferPosition);
				int dest=m_parameters->_vertexRank(&kmer);

				Message aMessage(message,bufferPosition,MPI_UNSIGNED_LONG_LONG,dest,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,theRank);
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
	Kmer*currentVertex,BubbleData*bubbleData,int theRank,
	int wordSize,
ExtensionData*ed,int minimumCoverage,int maxCoverage,OpenAssemblerChooser*oa,Chooser*chooser,
	vector<vector<Kmer> >*seeds,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,int size,
int*receivedVertexCoverage,bool*edgesReceived,vector<Kmer>*receivedOutgoingEdges
){
	if(m_expiredReads.count(ed->m_EXTENSION_currentPosition)>0){
		for(int i=0;i<(int)m_expiredReads[ed->m_EXTENSION_currentPosition].size();i++){
			uint64_t uniqueId=m_expiredReads[ed->m_EXTENSION_currentPosition][i];
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
		cout<<"Error: The seed contains a choice not supported by the graph."<<endl;
		cout<<"Extension length: "<<ed->m_EXTENSION_extension->size()<<" vertices"<<endl;
		cout<<"position="<<ed->m_EXTENSION_currentPosition<<" "<<idToWord(&(ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition]),wordSize,m_parameters->getColorSpaceMode())<<" with "<<ed->m_enumerateChoices_outgoingEdges.size()<<" choices ";
		cout<<endl;
		cout<<"Seed length: "<<ed->m_EXTENSION_currentSeed.size()<<" vertices"<<endl;
		cout<<"Choices: ";
		for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
			cout<<" "<<idToWord(&(ed->m_enumerateChoices_outgoingEdges[i]),wordSize,m_parameters->getColorSpaceMode());
		}
		cout<<endl;
		cout<<"ComplementSeed="<<ed->m_EXTENSION_complementedSeed<<endl;

		#endif

		#ifdef ASSERT
		assert(false);
		#endif

	// else, do a paired-end or single-end lookup if reads are in range.

	}else{
		// stuff in the reads to appropriate arcs.
		if(!ed->m_EXTENSION_singleEndResolution && ed->m_EXTENSION_readsInRange->size()>0){
			// try to use single-end reads to resolve the repeat.
			// for each read in range, ask them their vertex at position (CurrentPositionOnContig-StartPositionOfReadOnContig)
			// and cumulate the results in
			if(ed->m_EXTENSION_readIterator!=ed->m_EXTENSION_readsInRange->end()){
				m_removedUnfitLibraries=false;
				// we received the vertex for that read,
				// now check if it matches one of 
				// the many choices we have
				uint64_t uniqueId=*(ed->m_EXTENSION_readIterator);
				ExtensionElement*element=ed->getUsedRead(uniqueId);
				#ifdef ASSERT
				assert(element!=NULL);
				#endif
				int startPosition=element->getPosition();
				int distance=ed->m_EXTENSION_extension->size()-startPosition+element->getStrandPosition();

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
					ed->m_EXTENSION_readIterator++;	
					return;
				}

				char theRightStrand=element->getStrand();
				#ifdef ASSERT
				assert(theRightStrand=='R'||theRightStrand=='F');
				assert(element->getType()==TYPE_SINGLE_END||element->getType()==TYPE_RIGHT_END||element->getType()==TYPE_LEFT_END);
				#endif

				ed->m_EXTENSION_receivedReadVertex=kmerAtPosition(theSequence,distance,wordSize,theRightStrand,m_parameters->getColorSpaceMode());
				// process each edge separately.
				// got a match!

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
					int library=pairedRead->getLibrary();
					int expectedFragmentLength=m_parameters->getLibraryAverageLength(library);
					int expectedDeviation=m_parameters->getLibraryStandardDeviation(library);
					ExtensionElement*extensionElement=ed->getUsedRead(uniqueReadIdentifier);
					if(extensionElement!=NULL){// use to be via readsPositions
						char theLeftStrand=extensionElement->getStrand();
						int startingPositionOnPath=extensionElement->getPosition();

						int repeatLengthForLeftRead=ed->m_repeatedValues->at(startingPositionOnPath);
						int observedFragmentLength=(startPosition-startingPositionOnPath)+ed->m_EXTENSION_receivedLength+extensionElement->getStrandPosition()-element->getStrandPosition();
						int multiplier=3;

						if(expectedFragmentLength-multiplier*expectedDeviation<=observedFragmentLength 
						&& observedFragmentLength <= expectedFragmentLength+multiplier*expectedDeviation 
				&&( (theLeftStrand=='F' && theRightStrand=='R')
					||(theLeftStrand=='R' && theRightStrand=='F'))
				// the bridging pair is meaningless if both start in repeats
				&&repeatLengthForLeftRead<repeatThreshold){
						// it matches!
							ed->m_EXTENSION_pairedReadPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(observedFragmentLength);
							ed->m_EXTENSION_pairedLibrariesForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(library);
							ed->m_EXTENSION_pairedReadsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(uniqueId);
							m_hasPairedSequences=true;
						}
					}

					// add it anyway as a single-end match too!
					if(repeatValueForRightRead<repeatThreshold){
						ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(distance);
					}

					ed->m_EXTENSION_readIterator++;
				}
			}else{
				if(!m_removedUnfitLibraries){
					removeUnfitLibraries();
					m_removedUnfitLibraries=true;

					// there is a bug with this on human genome infinite loop)
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
						if(!m_hasPairedSequences){
							break;// can'T free if there are no pairs
						}
						uint64_t uniqueId=ed->m_sequencesToFree[i];
						#ifdef HUNT_INFINITE_BUG
						if(m_ed->m_EXTENSION_extension->size()>10000){
							cout<<"Removing "<<uniqueId<<"  now="<<m_ed->m_EXTENSION_extension->size()-1<<endl;
						}
						#endif
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

				int choice=(*oa).choose(ed,&(*chooser),minimumCoverage,(maxCoverage),m_parameters);
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
				bubbleData->m_coverages.clear();
				bubbleData->m_coverages[(*currentVertex)]=ed->m_currentCoverage;

			}

			if(m_dfsData->m_doChoice_tips_i<(int)ed->m_enumerateChoices_outgoingEdges.size()){
				if(!m_dfsData->m_doChoice_tips_dfs_done){
					if(ed->m_enumerateChoices_outgoingEdges.size()==1){
						m_dfsData->m_doChoice_tips_dfs_done=true;
					}else{
						m_dfsData->depthFirstSearch((*currentVertex),ed->m_enumerateChoices_outgoingEdges[m_dfsData->m_doChoice_tips_i],maxDepth,edgesRequested,vertexCoverageRequested,vertexCoverageReceived,outboxAllocator,
size,theRank,outbox,receivedVertexCoverage,receivedOutgoingEdges,minimumCoverage,edgesReceived,wordSize,
	m_parameters);
					}
				}else{
					#ifdef ASSERT
					assert(!m_dfsData->m_depthFirstSearchVisitedVertices_vector.empty());
					#endif

					// store visited vertices for bubble detection purposes.
					bubbleData->m_BUBBLE_visitedVertices.push_back(m_dfsData->m_depthFirstSearchVisitedVertices_vector);
					for(map<Kmer,int>::iterator i=m_dfsData->m_coverages.begin();
						i!=m_dfsData->m_coverages.end();i++){
						bubbleData->m_coverages[i->first]=i->second;
					}

					bubbleData->m_visitedVertices.push_back(m_dfsData->m_depthFirstSearchVisitedVertices);
					// keep the edge if it is not a tip.
					if(m_dfsData->m_depthFirstSearch_maxDepth>=TIP_LIMIT){
						m_dfsData->m_doChoice_tips_newEdges.push_back(m_dfsData->m_doChoice_tips_i);
					}

					m_dfsData->m_doChoice_tips_i++;
					m_dfsData->m_doChoice_tips_dfs_initiated=false;
					m_dfsData->m_doChoice_tips_dfs_done=false;
				}
			}else{
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
			vector<Kmer> complementedSeed;
			int theCurrentSize=ed->m_EXTENSION_currentSeed.size();
			printf("Rank %i reached %i vertices from seed %i (changing direction)\n",theRank,theCurrentSize,
				m_ed->m_EXTENSION_currentSeedIndex+1);
			fflush(stdout);

			for(int i=ed->m_EXTENSION_extension->size()-1;i>=0;i--){
				complementedSeed.push_back(complementVertex(&(ed->m_EXTENSION_extension->at(i)),wordSize,
					m_parameters->getColorSpaceMode()));
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

void SeedExtender::printTree(Kmer root,
map<Kmer,set<Kmer> >*arcs,map<Kmer,int>*coverages,int depth,set<Kmer>*visited){
	if(arcs->count(root)==0)
		return;
	if(visited->count(root)>0)
		return;
	visited->insert(root);
	set<Kmer> children=(*arcs)[root];
	for(set<Kmer>::iterator i=children.begin();i!=children.end();++i){
		for(int j=0;j<depth;j++)
			printf(" ");
		Kmer child=*i;
		string s=idToWord(&child,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
		#ifdef ASSERT
		assert(coverages->count(*i)>0);
		#endif
		int coverage=(*coverages)[*i];
		#ifdef ASSERT
		assert(coverages>0);
		#endif
		printf("%s coverage: %i depth: %i\n",s.c_str(),coverage,depth);

		if(coverages->count(*i)==0||coverage==0){
			cout<<"Error: "<<idToWord(&child,m_parameters->getWordSize(),m_parameters->getColorSpaceMode())<<" don't have a coverage value"<<endl;
		}

		if(depth==1)
			visited->clear();

		printTree(*i,arcs,coverages,depth+1,visited);
	}
}

void SeedExtender::storeExtensionAndGetNextOne(ExtensionData*ed,int theRank,vector<vector<Kmer> >*seeds,
Kmer *currentVertex,BubbleData*bubbleData){
	if((int)ed->m_EXTENSION_extension->size()+m_parameters->getWordSize()-1>=m_parameters->getMinimumContigLength()){
		if(m_parameters->showEndingContext()){
			cout<<"Choosing... (impossible!)"<<endl;
			inspect(ed,currentVertex);
			cout<<"Stopping extension..."<<endl;
		}

		if(ed->m_enumerateChoices_outgoingEdges.size()>1 && ed->m_EXTENSION_readsInRange->size()>0
		&&m_parameters->showEndingContext()){
			map<Kmer,set<Kmer> >arcs;
			for(int i=0;i<(int)bubbleData->m_BUBBLE_visitedVertices.size();i++){
				Kmer root=*currentVertex;
				Kmer child=ed->m_enumerateChoices_outgoingEdges[i];
				arcs[root].insert(child);
				for(int j=0;j<(int)bubbleData->m_BUBBLE_visitedVertices[i].size();j+=2){
					Kmer first=bubbleData->m_BUBBLE_visitedVertices[i][j];
					Kmer second=bubbleData->m_BUBBLE_visitedVertices[i][j+1];
					arcs[first].insert(second);
				}
			}
			printf("\n");
			printf("Tree\n");
			string s=idToWord(currentVertex,m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
			printf("%s %i\n",s.c_str(),ed->m_currentCoverage);
			set<Kmer> visited;
			printTree(*currentVertex,&arcs,
					&bubbleData->m_coverages,1,&visited);
			printf("\n");

		}

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

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(theRank);
	}

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
  int*outgoingEdgeIndex,int*last_value,Kmer*currentVertex,int theRank,bool*vertexCoverageRequested,int wordSize,int size,vector<vector<Kmer> >*seeds){
	if(!ed->m_EXTENSION_directVertexDone){
		if(!ed->m_EXTENSION_VertexAssembled_requested){
			delete m_dfsData;
			m_dfsData=new DepthFirstSearchData;

			m_receivedDirections.clear();
			if(ed->m_EXTENSION_currentSeedIndex%10==0 && ed->m_EXTENSION_currentPosition==0 
		&&(*last_value)!=ed->m_EXTENSION_currentSeedIndex){
				(*last_value)=ed->m_EXTENSION_currentSeedIndex;
				printf("Rank %i is extending seeds [%i/%i] \n",theRank,(int)ed->m_EXTENSION_currentSeedIndex+1,(int)(*seeds).size());
				fflush(stdout);
				
			}
			ed->m_EXTENSION_VertexAssembled_requested=true;
			uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(2*sizeof(uint64_t));
			int bufferPosition=0;
			currentVertex->pack(message,&bufferPosition);
			int destination=m_parameters->_vertexRank(currentVertex);
			Message aMessage(message,bufferPosition,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_ASK_IS_ASSEMBLED,theRank);
			(*outbox).push_back(aMessage);
			ed->m_EXTENSION_VertexAssembled_received=false;
		}else if(ed->m_EXTENSION_VertexAssembled_received){
			ed->m_EXTENSION_reverseVertexDone=false;
			ed->m_EXTENSION_directVertexDone=true;
			ed->m_EXTENSION_VertexMarkAssembled_requested=false;
			(*vertexCoverageRequested)=false;
			ed->m_EXTENSION_VertexAssembled_requested=false;
			if(ed->m_EXTENSION_vertexIsAssembledResult){
				ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
				ed->m_EXTENSION_markedCurrentVertexAsAssembled=false;
				ed->m_EXTENSION_directVertexDone=false;
				ed->m_EXTENSION_reads_requested=false;
				m_messengerInitiated=false;
			
			}
		}
	}else if(!ed->m_EXTENSION_reverseVertexDone){
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
			ed->m_EXTENSION_markedCurrentVertexAsAssembled=false;
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_reads_requested=false;
			m_messengerInitiated=false;
	}
}

void SeedExtender::markCurrentVertexAsAssembled(Kmer*currentVertex,RingAllocator*outboxAllocator,int*outgoingEdgeIndex, 
StaticVector*outbox,int size,int theRank,ExtensionData*ed,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,bool*edgesRequested,
vector<Kmer>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,int minimumCoverage,OpenAssemblerChooser*oa,int wordSize,vector<vector<Kmer> >*seeds
){
	if(!m_messengerInitiated){
		m_hasPairedSequences=false;
		*edgesRequested=false;
		m_pickedInformation=false;
		int theCurrentSize=ed->m_EXTENSION_extension->size();
		if(theCurrentSize%10000==0){
			if(theCurrentSize==0 && !ed->m_EXTENSION_complementedSeed){
				m_extended++;
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
		Kmer vertex=*currentVertex;
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
		*receivedOutgoingEdges=_getOutgoingEdges(currentVertex,compactEdges,m_parameters->getWordSize());
		ed->m_EXTENSION_extension->push_back((*currentVertex));
		ed->m_extensionCoverageValues->push_back(*receivedVertexCoverage);

		if(ed->m_currentCoverage<m_parameters->getRepeatCoverage()){
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
				char buffer[4000];
				SplayNode<uint64_t,Read>*node=m_cacheForRepeatedReads.find(uniqueId,false);
				#ifdef ASSERT
				assert(node!=NULL);
				#endif
				node->getValue()->getSeq(buffer,m_parameters->getColorSpaceMode(),false);
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
				char theRightStrand=annotation.getStrand();

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
						

						ExtensionElement*extensionElement=ed->getUsedRead(mateId);

						if(extensionElement==NULL){
							addRead=false;
						}

					}
				}

				// check the distance.
				if(ed->m_EXTENSION_pairedRead.getLibrary()!=DUMMY_LIBRARY){
					uint64_t mateId=ed->m_EXTENSION_pairedRead.getUniqueId();
					// the mate is required to allow proper placement
					
					ExtensionElement*extensionElement=ed->getUsedRead(mateId);

					if(extensionElement!=NULL){// use to be via readsPositions
						char theLeftStrand=extensionElement->getStrand();
						int startingPositionOnPath=extensionElement->getPosition();

						int repeatLengthForLeftRead=ed->m_repeatedValues->at(startingPositionOnPath);
						int observedFragmentLength=(startPosition-startingPositionOnPath)+ed->m_EXTENSION_receivedLength+extensionElement->getStrandPosition()-positionOnStrand;
						int multiplier=3;

						int library=ed->m_EXTENSION_pairedRead.getLibrary();
						int expectedFragmentLength=m_parameters->getLibraryAverageLength(library);
						int expectedDeviation=m_parameters->getLibraryStandardDeviation(library);

						int repeatThreshold=100;
						if(expectedFragmentLength-multiplier*expectedDeviation<=observedFragmentLength 
						&& observedFragmentLength <= expectedFragmentLength+multiplier*expectedDeviation 
				&&( (theLeftStrand=='F' && theRightStrand=='R')
					||(theLeftStrand=='R' && theRightStrand=='F'))
				// the bridging pair is meaningless if both start in repeats
				&&repeatLengthForLeftRead<repeatThreshold){
							
						}else{
							// remove the right read from the used set
							addRead=false;
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
		
					#ifdef HUNT_INFINITE_BUG
					if(m_ed->m_EXTENSION_extension->size()>10000){
						cout<<"Add SeqInfo Seq="<<m_receivedString<<" Id="<<uniqueId<<" ExpiryPosition="<<expiryPosition<<endl;
					}
					#endif
					m_expiredReads[expiryPosition].push_back(uniqueId);
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

							#ifdef HUNT_INFINITE_BUG
							if(m_ed->m_EXTENSION_extension->size()>10000){
								cout<<"adding expiration (to meet mate) Now="<<startPosition<<" Expiration="<<expiration<<" Id="<<uniqueId<<" "<<"muL="<<expectedFragmentLength<<" sigmaL="<<expectedDeviation<<endl;
							}
							#endif
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
			ed->m_EXTENSION_directVertexDone=true;
			ed->m_EXTENSION_VertexMarkAssembled_requested=false;
			ed->m_EXTENSION_enumerateChoices=false;
			(*edgesRequested)=false;
			ed->m_EXTENSION_markedCurrentVertexAsAssembled=true;
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
	m_cacheForListOfReads.constructor();
	ostringstream prefixFull;
	m_parameters=parameters;
	prefixFull<<m_parameters->getMemoryPrefix()<<"_SeedExtender";
	m_cacheAllocator.constructor(4194304,RAY_MALLOC_TYPE_SEED_EXTENDER_CACHE,m_parameters->showMemoryAllocations());
	m_inbox=inbox;
	m_subgraph=subgraph;
	m_dfsData=new DepthFirstSearchData;
	m_cache.constructor();
	m_ed=ed;
	this->m_directionsAllocator=m_directionsAllocator;
	m_bubbleTool.constructor(parameters);
}

void SeedExtender::inspect(ExtensionData*ed,Kmer*currentVertex){
	#ifdef ASSERT
	assert(ed->m_enumerateChoices_outgoingEdges.size()==ed->m_EXTENSION_coverages->size());
	#endif

	int wordSize=m_parameters->getWordSize();
	cout<<endl;
	cout<<"*****************************************"<<endl;
	cout<<"CurrentVertex="<<idToWord(currentVertex,wordSize,m_parameters->getColorSpaceMode())<<" @"<<ed->m_EXTENSION_extension->size()<<endl;
	#ifdef ASSERT
	assert(ed->m_currentCoverage<=m_parameters->getMaximumAllowedCoverage());
	#endif
	cout<<"Coverage="<<ed->m_currentCoverage<<endl;
	cout<<" # ReadsInRange: "<<ed->m_EXTENSION_readsInRange->size()<<endl;
	cout<<ed->m_enumerateChoices_outgoingEdges.size()<<" choices ";

	cout<<endl;
	for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
		string vertex=idToWord(&(ed->m_enumerateChoices_outgoingEdges[i]),wordSize,m_parameters->getColorSpaceMode());
		Kmer key=ed->m_enumerateChoices_outgoingEdges[i];
		cout<<endl;
		cout<<"Choice #"<<i+1<<endl;
		cout<<"Vertex: "<<vertex<<endl;
		#ifdef ASSERT
		if(i>=(int)ed->m_EXTENSION_coverages->size()){
			cout<<"Error: i="<<i<<" Size="<<ed->m_EXTENSION_coverages->size()<<endl;
		}
		assert(i<(int)ed->m_EXTENSION_coverages->size());
		#endif
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
	bool hasPairedSequences=false;
	for(int i=0;i<(int)m_ed->m_enumerateChoices_outgoingEdges.size();i++){
		map<int,vector<int> > classifiedValues;
		map<int,vector<uint64_t> > reads;
		Kmer vertex=m_ed->m_enumerateChoices_outgoingEdges[i];
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
					#ifdef HUNT_INFINITE_BUG
					if(m_ed->m_EXTENSION_extension->size()>10000){
						cout<<"Restoring Value="<<j->second[k]<<" Expected="<<averageLength<<" Dev="<<stddev<<" MeanForLibrary="<<mean<<" n="<<n<<endl;
					}
					#endif
					m_ed->m_sequencesToFree.push_back(uniqueId);
				}
			}
		}
		m_ed->m_EXTENSION_pairedReadPositionsForVertices[vertex]=acceptedValues;
		if(!acceptedValues.empty()){
			hasPairedSequences=true;
		}
	}
	m_hasPairedSequences=hasPairedSequences;
}

void SeedExtender::setFreeUnmatedPairedReads(){
	if(!m_hasPairedSequences){// avoid infinite loops.
		return;
	}
	if(m_ed->m_expirations->count(m_ed->m_EXTENSION_extension->size())==0){
		return;
	}
	vector<uint64_t>*expired=&(*m_ed->m_expirations)[m_ed->m_EXTENSION_extension->size()];
	for(int i=0;i<(int)expired->size();i++){
		uint64_t readId=expired->at(i);
		if(m_ed->m_pairedReadsWithoutMate->count(readId)>0){
			m_ed->m_sequencesToFree.push_back(readId); // RECYCLING IS desactivated
			#ifdef HUNT_INFINITE_BUG
			if(m_ed->m_EXTENSION_extension->size()>10000){
				cout<<"Expired: Now="<<m_ed->m_EXTENSION_extension->size()-1<<" Id="<<readId<<endl;
			}
			#endif
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

void SeedExtender::printExtensionStatus(Kmer*currentVertex){
	int theRank=m_parameters->getRank();
	int theCurrentSize=m_ed->m_EXTENSION_extension->size();
	// stop the infinite loop
	#ifdef HUNT_INFINITE_BUG
	if(m_ed->m_EXTENSION_extension->size()>200000){
		cout<<"Error: Infinite loop"<<endl;
		exit(0);
	}
	#endif
	printf("Rank %i reached %i vertices (%s) from seed %i\n",theRank,theCurrentSize,
		idToWord(currentVertex,m_parameters->getWordSize(),m_parameters->getColorSpaceMode()).c_str(),
		m_ed->m_EXTENSION_currentSeedIndex+1);

	fflush(stdout);

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(theRank);
	}

	#ifdef SHOW_READS_IN_RANGE
	showReadsInRange();
	#endif
	int chunks=m_directionsAllocator->getNumberOfChunks();
	int chunkSize=m_directionsAllocator->getChunkSize();
	uint64_t totalBytes=chunks*chunkSize;
	uint64_t kibibytes=totalBytes/1024;
	if(m_parameters->showMemoryUsage()){
		printf("Rank %i: memory usage for directions is %lu KiB\n",theRank,kibibytes);
		fflush(stdout);
	}
}


