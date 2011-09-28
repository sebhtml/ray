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

/* TODO: free sequence in ExtensionElement objects when they are not needed anymore */

#include <core/constants.h>
#include <memory/malloc_types.h>
#include <string.h>
#include <structures/StaticVector.h>
#include <heuristics/TipWatchdog.h>
#include <assembler/SeedExtender.h>
#include <core/OperatingSystem.h>
#include <fstream>
#include <heuristics/Chooser.h>
#include <sstream>
#include <assert.h>
#include <heuristics/BubbleTool.h>
#include <cryptography/crypto.h>
using namespace std;

/** extend the seeds */
void SeedExtender::extendSeeds(vector<vector<Kmer> >*seeds,ExtensionData*ed,int theRank,StaticVector*outbox,
  Kmer*currentVertex,FusionData*fusionData,RingAllocator*outboxAllocator,bool*edgesRequested,int*outgoingEdgeIndex,
int*last_value,bool*vertexCoverageRequested,int wordSize,int size,bool*vertexCoverageReceived,
int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,vector<Kmer>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,
int minimumCoverage,OpenAssemblerChooser*oa,bool*edgesReceived,int*m_mode){

	MACRO_COLLECT_PROFILING_INFORMATION();
	
	/* read the checkpoint */
	if(!m_checkedCheckpoint){
		m_checkedCheckpoint=true;
		if(m_parameters->hasCheckpoint("Extensions")){

			readCheckpoint();
	
			(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
			Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_EXTENSION_IS_DONE,theRank);
			outbox->push_back(aMessage);
	
			// store the reverse map
			for(int i=0;i<(int)ed->m_EXTENSION_identifiers.size();i++){
				uint64_t id=ed->m_EXTENSION_identifiers[i];
				fusionData->m_FUSION_identifier_map[id]=i;
			}
	
			return;
		}

		MACRO_COLLECT_PROFILING_INFORMATION();
	}

	MACRO_COLLECT_PROFILING_INFORMATION();

	if(ed->m_EXTENSION_currentSeedIndex==(int)(*seeds).size()
		|| seeds->size()==0){


		MACRO_COLLECT_PROFILING_INFORMATION();

		if((*seeds).size()>0)
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
	
		// store the reverse map
		for(int i=0;i<(int)ed->m_EXTENSION_identifiers.size();i++){
			uint64_t id=ed->m_EXTENSION_identifiers[i];
			fusionData->m_FUSION_identifier_map[id]=i;
		}

		#ifdef ASSERT
		assert(m_ed->m_EXTENSION_identifiers.size()==m_ed->m_EXTENSION_contigs.size());
		#endif

		/* write checkpoint */
		if(m_parameters->writeCheckpoints())
			writeCheckpoint();

		(*m_mode)=RAY_SLAVE_MODE_DO_NOTHING;
		Message aMessage(NULL,0,MASTER_RANK,RAY_MPI_TAG_EXTENSION_IS_DONE,theRank);
		outbox->push_back(aMessage);

		/** write extensions for debugging purposes */
		if(m_parameters->hasOption("-write-extensions")){
			ostringstream fileName;
			fileName<<m_parameters->getPrefix()<<"Rank"<<m_parameters->getRank()<<"RayExtensions.fasta";
			ofstream f(fileName.str().c_str());
			for(int i=0;i<(int)ed->m_EXTENSION_identifiers.size();i++){
				uint64_t id=ed->m_EXTENSION_identifiers[i];
				f<<">RayExtension-"<<id<<endl;

				f<<addLineBreaks(convertToString(&(ed->m_EXTENSION_contigs.at(i)),
					m_parameters->getWordSize(),m_parameters->getColorSpaceMode()),
					m_parameters->getColumns());
			}
			f.close();
		}

		return;
	}else if(!ed->m_EXTENSION_initiated){

		MACRO_COLLECT_PROFILING_INFORMATION();

		ed->m_EXTENSION_initiated=true;
		ed->m_EXTENSION_currentSeedIndex=0;
		ed->m_EXTENSION_currentPosition=0;
		ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
		(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
		ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
		ed->m_EXTENSION_directVertexDone=false;
		ed->m_EXTENSION_VertexAssembled_requested=false;
		ed->m_previouslyFlowedVertices=0;
		ed->m_flowNumber=0;
	
		ed->constructor(m_parameters);

		MACRO_COLLECT_PROFILING_INFORMATION();
	}


	MACRO_COLLECT_PROFILING_INFORMATION();

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
		if(!(ed->m_EXTENSION_currentPosition<(int)ed->m_EXTENSION_currentSeed.size())
		&& ed->m_flowNumber==1
 ){
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


		MACRO_COLLECT_PROFILING_INFORMATION();
	}else if(
		/* the first flow */
		ed->m_flowNumber==1 
 		/* vertex is assembled already */
		&& ed->m_EXTENSION_vertexIsAssembledResult 
 		/* we have not exited the seed */
		&& ed->m_EXTENSION_currentPosition<(int)ed->m_EXTENSION_currentSeed.size()
		){
		cout<<"Rank "<<m_parameters->getRank()<<" skips seed ["<<ed->m_EXTENSION_currentSeedIndex<<"/"<<
			(*seeds).size()<<"]"<<endl;

		ed->m_EXTENSION_currentSeedIndex++;// skip the current one.
		ed->m_EXTENSION_currentPosition=0;


		ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
		ed->m_EXTENSION_directVertexDone=false;
		ed->m_EXTENSION_VertexAssembled_requested=false;
		if(ed->m_EXTENSION_currentSeedIndex<(int)(*seeds).size()){
			ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
			(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
		}
		ed->m_previouslyFlowedVertices=0;
		ed->m_flowNumber=0;

		MACRO_COLLECT_PROFILING_INFORMATION();

	}else if(!ed->m_EXTENSION_markedCurrentVertexAsAssembled){
		MACRO_COLLECT_PROFILING_INFORMATION();

		markCurrentVertexAsAssembled(currentVertex,outboxAllocator,outgoingEdgeIndex,outbox,
size,theRank,ed,vertexCoverageRequested,vertexCoverageReceived,receivedVertexCoverage,
repeatedLength,maxCoverage,edgesRequested,receivedOutgoingEdges,chooser,bubbleData,minimumCoverage,
oa,wordSize,seeds);

		MACRO_COLLECT_PROFILING_INFORMATION();
	}else if(!ed->m_EXTENSION_enumerateChoices){
		MACRO_COLLECT_PROFILING_INFORMATION();

		enumerateChoices(edgesRequested,ed,edgesReceived,outboxAllocator,outgoingEdgeIndex,outbox,
		currentVertex,theRank,vertexCoverageRequested,receivedOutgoingEdges,
		vertexCoverageReceived,size,receivedVertexCoverage,chooser,wordSize);
	}else if(!ed->m_EXTENSION_choose){

		MACRO_COLLECT_PROFILING_INFORMATION();

		doChoice(outboxAllocator,outgoingEdgeIndex,outbox,currentVertex,bubbleData,theRank,wordSize,
	ed,minimumCoverage,*maxCoverage,oa,chooser,seeds,
edgesRequested,vertexCoverageRequested,vertexCoverageReceived,size,receivedVertexCoverage,edgesReceived,
receivedOutgoingEdges);
	}

	MACRO_COLLECT_PROFILING_INFORMATION();
}

// upon successful completion, ed->m_EXTENSION_coverages and ed->m_enumerateChoices_outgoingEdges are
// populated variables.
void SeedExtender::enumerateChoices(bool*edgesRequested,ExtensionData*ed,bool*edgesReceived,RingAllocator*outboxAllocator,
	int*outgoingEdgeIndex,StaticVector*outbox,
Kmer*currentVertex,int theRank,bool*vertexCoverageRequested,vector<Kmer>*receivedOutgoingEdges,
bool*vertexCoverageReceived,int size,int*receivedVertexCoverage,Chooser*chooser,int wordSize
){
	MACRO_COLLECT_PROFILING_INFORMATION();;

	if(!(*edgesRequested)){
		ed->m_EXTENSION_coverages.clear();
		ed->m_enumerateChoices_outgoingEdges.clear();
		(*edgesReceived)=true;
		(*edgesRequested)=true;
		ed->m_EXTENSION_currentPosition++;
		(*vertexCoverageRequested)=false;
		(*outgoingEdgeIndex)=0;
	}else if((*edgesReceived)){

		MACRO_COLLECT_PROFILING_INFORMATION();
		if((*outgoingEdgeIndex)<(int)(*receivedOutgoingEdges).size()){
			Kmer kmer=(*receivedOutgoingEdges)[(*outgoingEdgeIndex)];
			Kmer reverseComplement=kmer.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

			// get the coverage of these.
			// try to get in from the cache table to avoid sending
			// a message.
			if(!(*vertexCoverageRequested)&&(m_cache).find(kmer,false)!=NULL){
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=true;
				(*receivedVertexCoverage)=*(m_cache.find(kmer,false)->getValue());

				#ifdef ASSERT
				assert((*receivedVertexCoverage)<=m_parameters->getMaximumAllowedCoverage());
				#endif
			}else if(!(*vertexCoverageRequested)&&(m_cache).find(reverseComplement,false)!=NULL){
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=true;
				(*receivedVertexCoverage)=*(m_cache.find(reverseComplement,false)->getValue());

				#ifdef ASSERT
				assert((*receivedVertexCoverage)<=m_parameters->getMaximumAllowedCoverage());
				#endif

			}else if(!(*vertexCoverageRequested)){
				uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(KMER_U64_ARRAY_SIZE*sizeof(uint64_t));
				int bufferPosition=0;
				kmer.pack(message,&bufferPosition);
				int dest=m_parameters->_vertexRank(&kmer);

				Message aMessage(message,bufferPosition,dest,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,theRank);
				(*outbox).push_back(aMessage);
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=false;
				(*receivedVertexCoverage)=-1;

				MACRO_COLLECT_PROFILING_INFORMATION();

			}else if((*vertexCoverageReceived)){
				bool inserted;
				*((m_cache.insert(kmer,&m_cacheAllocator,&inserted))->getValue())=*receivedVertexCoverage;
				(*outgoingEdgeIndex)++;
				(*vertexCoverageRequested)=false;
				#ifdef ASSERT
				assert((*receivedVertexCoverage)<=m_parameters->getMaximumAllowedCoverage());
				#endif
				int coverageValue=*receivedVertexCoverage;
				if(coverageValue>=m_parameters->getMinimumCoverageToStore()){
					ed->m_EXTENSION_coverages.push_back((*receivedVertexCoverage));
					ed->m_enumerateChoices_outgoingEdges.push_back(kmer);
				}


				MACRO_COLLECT_PROFILING_INFORMATION();
			}
		}else{

			MACRO_COLLECT_PROFILING_INFORMATION();

			receivedOutgoingEdges->clear();
			ed->m_EXTENSION_enumerateChoices=true;
			ed->m_EXTENSION_choose=false;
			ed->m_EXTENSION_singleEndResolution=false;
			ed->m_EXTENSION_readIterator=ed->m_EXTENSION_readsInRange.begin();
			ed->m_EXTENSION_readLength_done=false;
			ed->m_EXTENSION_readPositionsForVertices.clear();
			ed->m_EXTENSION_pairedReadPositionsForVertices.clear();
			ed->m_EXTENSION_pairedLibrariesForVertices.clear();
			ed->m_EXTENSION_pairedReadsForVertices.clear();

			#ifdef ASSERT
			assert(ed->m_EXTENSION_coverages.size()==ed->m_enumerateChoices_outgoingEdges.size());
			#endif
		}
	}
				
	MACRO_COLLECT_PROFILING_INFORMATION();
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

	MACRO_COLLECT_PROFILING_INFORMATION();;

	if(m_expiredReads.count(ed->m_EXTENSION_currentPosition)>0){
		for(int i=0;i<(int)m_expiredReads[ed->m_EXTENSION_currentPosition].size();i++){
			uint64_t uniqueId=m_expiredReads[ed->m_EXTENSION_currentPosition][i];
			ed->m_EXTENSION_readsInRange.erase(uniqueId);

			// free the sequence
			ExtensionElement*element=ed->getUsedRead(uniqueId);
			if(element==NULL){
				continue;
			}
			#ifdef ASSERT
			assert(element!=NULL);
			#endif
/*
			char*read=element->getSequence();
			if(read==NULL){
				continue;
			}
			#ifdef ASSERT
			assert(read!=NULL);
			#endif
			
*/
/*
			element->removeSequence();
			ed->getAllocator()->free(read,strlen(read)+1);
*/
		}
		m_expiredReads.erase(ed->m_EXTENSION_currentPosition);
		ed->m_EXTENSION_readIterator=ed->m_EXTENSION_readsInRange.begin();

		MACRO_COLLECT_PROFILING_INFORMATION();

		return;
	}

	MACRO_COLLECT_PROFILING_INFORMATION();

	// if there is only one choice and reads supporting it
	if(ed->m_enumerateChoices_outgoingEdges.size()==1&&ed->m_EXTENSION_readsInRange.size()>0){
		#ifdef ASSERT
		assert(ed->m_EXTENSION_coverages.at(0)>=m_parameters->getMinimumCoverageToStore());
		#endif
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
		cout<<"Extension length: "<<ed->m_EXTENSION_extension.size()<<" vertices"<<endl;
		cout<<"position="<<ed->m_EXTENSION_currentPosition<<" "<<(ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition]).idToWord(wordSize,m_parameters->getColorSpaceMode())<<" with "<<ed->m_enumerateChoices_outgoingEdges.size()<<" choices ";
		cout<<endl;
		cout<<"Seed length: "<<ed->m_EXTENSION_currentSeed.size()<<" vertices"<<endl;
		cout<<"Choices: ";
		for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
			cout<<" "<<(ed->m_enumerateChoices_outgoingEdges[i]).idToWord(wordSize,m_parameters->getColorSpaceMode());
		}
		cout<<endl;

		#endif

		#ifdef ASSERT
		assert(false);
		#endif

	// else, do a paired-end or single-end lookup if reads are in range.

	}else{

		MACRO_COLLECT_PROFILING_INFORMATION();

		// stuff in the reads to appropriate arcs.
		if(!ed->m_EXTENSION_singleEndResolution && ed->m_EXTENSION_readsInRange.size()>0){
			// try to use single-end reads to resolve the repeat.
			// for each read in range, ask them their vertex at position (CurrentPositionOnContig-StartPositionOfReadOnContig)
			// and cumulate the results in
			if(ed->m_EXTENSION_readIterator!=ed->m_EXTENSION_readsInRange.end()){

				MACRO_COLLECT_PROFILING_INFORMATION();

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

/**
 * indels model for PacBio and 454 reads


algorithm:

for read in reads:
	extensionElement.find(vertices,&index,&distance,positionInContig)
	if index>=0:
		selectedVertex=vertices[index]

what happened:
	the extension element updated its last anchor to (distance,positionInContig) if a vertex from vertices was found
otherwise, index is < 0 and the extension element remains unchanged.

Presently, insertions or deletions up to 8 are supported.
*/

				int distance=ed->m_EXTENSION_extension.size()-startPosition+element->getStrandPosition();

				#ifdef ASSERT
				assert(startPosition<(int)ed->m_extensionCoverageValues.size());
				#endif

				element->getSequence(m_receivedString,m_parameters);
				char*theSequence=m_receivedString;
				#ifdef ASSERT
				assert(theSequence!=NULL);
				#endif
				ed->m_EXTENSION_receivedLength=strlen(theSequence);
				if(distance>(ed->m_EXTENSION_receivedLength-wordSize)){
					cout<<"OutOfRange UniqueId="<<uniqueId<<" Length="<<strlen(theSequence)<<" StartPosition="<<element->getPosition()<<" CurrentPosition="<<ed->m_EXTENSION_extension.size()-1<<" StrandPosition="<<element->getStrandPosition()<<endl;
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
					/* we only use single-end reads on
					non-repeated vertices */
					//if(repeatValueForRightRead<repeatThreshold){
					if(ed->m_currentCoverage<m_parameters->getRepeatCoverage()){
						ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(distance);	
					}
					ed->m_EXTENSION_readIterator++;
				}else{
					PairedRead*pairedRead=element->getPairedRead();
					uint64_t uniqueReadIdentifier=pairedRead->getUniqueId();
/*
					int diff=uniqueId-uniqueReadIdentifier;
					if(uniqueReadIdentifier>uniqueId){
						diff=uniqueReadIdentifier-uniqueId;
					}
*/

					MACRO_COLLECT_PROFILING_INFORMATION();

					int library=pairedRead->getLibrary();
					ExtensionElement*extensionElement=ed->getUsedRead(uniqueReadIdentifier);
					if(extensionElement!=NULL){// use to be via readsPositions
						char theLeftStrand=extensionElement->getStrand();
						int startingPositionOnPath=extensionElement->getPosition();

						//int repeatLengthForLeftRead=ed->m_repeatedValues->at(startingPositionOnPath);
						int observedFragmentLength=(startPosition-startingPositionOnPath)+ed->m_EXTENSION_receivedLength+extensionElement->getStrandPosition()-element->getStrandPosition();
						int multiplier=3;

						/* iterate over all peaks */
						for(int peak=0;peak<m_parameters->getLibraryPeaks(library);peak++){
							int expectedFragmentLength=m_parameters->getLibraryAverageLength(library,peak);
							int expectedDeviation=m_parameters->getLibraryStandardDeviation(library,peak);

							if(expectedFragmentLength-multiplier*expectedDeviation<=observedFragmentLength 
							&& observedFragmentLength <= expectedFragmentLength+multiplier*expectedDeviation 
					&&( (theLeftStrand=='F' && theRightStrand=='R')
						||(theLeftStrand=='R' && theRightStrand=='F'))
					// the bridging pair is meaningless if both start in repeats
					//&&repeatLengthForLeftRead<repeatThreshold){
					/* left read is safe so we don't care if right read is on a
					repeated region really. */){
							// it matches!

								m_pairedScores[library][peak]++;

								ed->m_EXTENSION_pairedReadPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(observedFragmentLength);
								ed->m_EXTENSION_pairedLibrariesForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(library);
								ed->m_EXTENSION_pairedReadsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(uniqueId);

								m_hasPairedSequences=true;

								MACRO_COLLECT_PROFILING_INFORMATION();

								/** only match 1 peak */
								break;
							}
						}
					}

					// add it anyway as a single-end match too!
					/* add it as single-end read if not repeated. */
					//if(repeatValueForRightRead<repeatThreshold){
					if(ed->m_currentCoverage<m_parameters->getRepeatCoverage()){
						ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_receivedReadVertex].push_back(distance);
					}

					ed->m_EXTENSION_readIterator++;
				}
			}else{
				if(!m_removedUnfitLibraries){
					removeUnfitLibraries();
					m_removedUnfitLibraries=true;

					// free reads at this position
					setFreeUnmatedPairedReads();

					return;
				}

				MACRO_COLLECT_PROFILING_INFORMATION();

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
/*
						#ifdef HUNT_INFINITE_BUG
						if(m_ed->m_EXTENSION_extension.size()>10000){
							cout<<"Removing "<<uniqueId<<"  now="<<m_ed->m_EXTENSION_extension.size()-1<<endl;
						}
						#endif
*/
						m_ed->m_pairedReadsWithoutMate.erase(uniqueId);

						// free the sequence
						#ifdef ASSERT
						ExtensionElement*element=ed->getUsedRead(uniqueId);
						if(element==NULL){
							cout<<"element "<<uniqueId<<" not found now="<<m_ed->m_EXTENSION_extension.size()-1<<""<<endl;
						}
						assert(element!=NULL);
						#endif

/*
						char*read=element->getSequence();
						if(read!=NULL){
							#ifdef ASSERT
							assert(read!=NULL);
							#endif
							element->removeSequence();
							ed->getAllocator()->free(read,strlen(read)+1);
						}
*/

						// remove it
						ed->removeSequence(uniqueId);
						ed->m_EXTENSION_readsInRange.erase(uniqueId);
					}
					ed->m_sequencesToFree.clear();
					return;
				}

				ed->m_EXTENSION_singleEndResolution=true;

				if(m_parameters->showExtensionChoice())
					inspect(ed,currentVertex);
				int choice=(*oa).choose(ed,&(*chooser),minimumCoverage,(maxCoverage),m_parameters);
				if(choice!=IMPOSSIBLE_CHOICE){
					if(m_parameters->showExtensionChoice()){
						cout<<"Selection: "<<choice+1<<endl;
					}

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

			MACRO_COLLECT_PROFILING_INFORMATION();

			return;
		}else if(!ed->m_doChoice_tips_Detected && ed->m_EXTENSION_readsInRange.size()>0){
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

			MACRO_COLLECT_PROFILING_INFORMATION();

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
				if(m_dfsData->m_doChoice_tips_newEdges.size()==1 && ed->m_EXTENSION_readsInRange.size()>0 
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
		}else if(!bubbleData->m_doChoice_bubbles_Detected && ed->m_EXTENSION_readsInRange.size()>0){
			
			MACRO_COLLECT_PROFILING_INFORMATION();
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


		MACRO_COLLECT_PROFILING_INFORMATION();

		bool mustFlowAgain=true;

		// check if the next flow is needed at all
		// this may be bad for read reuse,
		// TODO: try with that off
/*
		int currentFlow=ed->m_flowNumber+1;
		if(currentFlow==2){
			mustFlowAgain=false;

			// we may get more range
			if(ed->m_previouslyFlowedVertices < m_parameters->getMaximumDistance() 
			&& (int)ed->m_EXTENSION_extension.size() >= m_parameters->getMaximumDistance()){
				mustFlowAgain=true;
			}
		}
*/

		// no choice possible...
		if((int)ed->m_EXTENSION_extension.size() > ed->m_previouslyFlowedVertices && mustFlowAgain){

			MACRO_COLLECT_PROFILING_INFORMATION();

			if(!m_slicedComputationStarted){
				m_slicedComputationStarted=true;
				m_complementedSeed.clear();
				m_slicedProgression = ed->m_EXTENSION_extension.size()-1;
				//cout<<"INITIATING SLICED COMPUTATION"<<endl;
			}

			int iterations=0;
			int maximumIterations=4;

			MACRO_COLLECT_PROFILING_INFORMATION();

			//cout<<"STARTING SLICED COMPUTATION"<<endl;

			// this code must be run in many slices...
			for(int i= m_slicedProgression;i>=0;i--){
				m_complementedSeed.push_back(ed->m_EXTENSION_extension.at(i).complementVertex(wordSize,
					m_parameters->getColorSpaceMode()));

				iterations ++;
				m_slicedProgression --;

				if(iterations >= maximumIterations)
					break;
			}

			//cout<<"DONE SLICED COMPUTATION"<<endl;

			MACRO_COLLECT_PROFILING_INFORMATION();

			// not done yet
			if(m_complementedSeed.size() < ed->m_EXTENSION_extension.size()){
				//cout<<"Not done yet..."<<endl;
				return;
			}

			/** inspect the local setup */
			if(m_parameters->showEndingContext()){
				inspect(ed,currentVertex);
			}


			cout<<"Rank "<<m_parameters->getRank()<<" is changing direction."<<endl;
			
			ed->m_previouslyFlowedVertices = ed->m_EXTENSION_extension.size();

			m_flowedVertices.push_back(ed->m_EXTENSION_extension.size());

			printExtensionStatus(currentVertex);

			#ifdef ASSERT
			assert(m_complementedSeed.size() == ed->m_EXTENSION_extension.size());
			#endif

			MACRO_COLLECT_PROFILING_INFORMATION();

			/* increment the flow number */
			ed->m_flowNumber++;

			ed->m_EXTENSION_currentPosition=0;
			ed->m_EXTENSION_currentSeed=m_complementedSeed;
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
			(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];

			MACRO_COLLECT_PROFILING_INFORMATION();

			ed->resetStructures(m_profiler,m_runProfiler);

			MACRO_COLLECT_PROFILING_INFORMATION();

			m_matesToMeet.clear();
	
			MACRO_COLLECT_PROFILING_INFORMATION();
/*
			m_cacheAllocator.clear();
			m_cache.clear();
*/
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_VertexAssembled_requested=false;

			MACRO_COLLECT_PROFILING_INFORMATION();
		}else{
			m_flowedVertices.push_back(ed->m_EXTENSION_extension.size());
			storeExtensionAndGetNextOne(ed,theRank,seeds,currentVertex,bubbleData);
			MACRO_COLLECT_PROFILING_INFORMATION();
		}

		MACRO_COLLECT_PROFILING_INFORMATION();
	}
	MACRO_COLLECT_PROFILING_INFORMATION();
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
		string s=child.idToWord(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
		#ifdef ASSERT
		assert(coverages->count(*i)>0);
		#endif
		int coverage=(*coverages)[*i];
		#ifdef ASSERT
		assert(coverages>0);
		#endif
		printf("%s coverage: %i depth: %i\n",s.c_str(),coverage,depth);

		if(coverages->count(*i)==0||coverage==0){
			cout<<"Error: "<<child.idToWord(m_parameters->getWordSize(),m_parameters->getColorSpaceMode())<<" don't have a coverage value"<<endl;
		}

		if(depth==1)
			visited->clear();

		printTree(*i,arcs,coverages,depth+1,visited);
	}
}

/** store the extension and do the next one right now */
void SeedExtender::storeExtensionAndGetNextOne(ExtensionData*ed,int theRank,vector<vector<Kmer> >*seeds,
Kmer *currentVertex,BubbleData*bubbleData){
	if((int)ed->m_EXTENSION_extension.size()+m_parameters->getWordSize()-1>=m_parameters->getMinimumContigLength()){
		if(m_parameters->showEndingContext()){
			cout<<"Choosing... (impossible!)"<<endl;
			inspect(ed,currentVertex);
			cout<<"Stopping extension..."<<endl;
		}

		MACRO_COLLECT_PROFILING_INFORMATION();

		if(ed->m_enumerateChoices_outgoingEdges.size()>1 && ed->m_EXTENSION_readsInRange.size()>0
		&&m_parameters->showEndingContext() && false){ /* don't show this tree. */
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
			string s=currentVertex->idToWord(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
			printf("%s %i\n",s.c_str(),ed->m_currentCoverage);
			set<Kmer> visited;
			printTree(*currentVertex,&arcs,
					&bubbleData->m_coverages,1,&visited);
			printf("\n");

		}

		MACRO_COLLECT_PROFILING_INFORMATION();

		printExtensionStatus(currentVertex);

		MACRO_COLLECT_PROFILING_INFORMATION();

		cout<<"Rank "<<theRank<<" (extension done) NumberOfFlows: "<<ed->m_flowNumber<<endl;

		m_extended++;

		MACRO_COLLECT_PROFILING_INFORMATION();

		cout<<"Rank "<<m_parameters->getRank()<<" FlowedVertices:";
		for(int i=0;i<(int)m_flowedVertices.size();i++){
			cout<<" "<<i<<" "<<m_flowedVertices[i];
		}

		cout<<endl;
	
		MACRO_COLLECT_PROFILING_INFORMATION();

		if(m_parameters->hasOption("-show-distance-summary")){
			/** show the utilised outer distances */
			cout<<"Rank "<<theRank<<" utilised outer distances: "<<endl;
			for(map<int,map<int, uint64_t> >::iterator i=m_pairedScores.begin();i!=m_pairedScores.end();i++){
				for(map<int,uint64_t>::iterator j=i->second.begin();j!=i->second.end();j++){
					int lib=i->first;
					int peak=j->first;
					int average=m_parameters->getLibraryAverageLength(lib,peak);
					int deviation=m_parameters->getLibraryStandardDeviation(lib,peak);
					uint64_t count=j->second;
	
					cout<<"Rank "<<theRank<<" Library: "<<lib<<" LibraryPeak: "<<peak<<" PeakAverage: "<<average<<" PeakDeviation: "<<deviation<<" Pairs: "<<count<<endl;
				}
			}
		}

		MACRO_COLLECT_PROFILING_INFORMATION();

		ed->m_EXTENSION_contigs.push_back((ed->m_EXTENSION_extension));

		MACRO_COLLECT_PROFILING_INFORMATION();
	
		uint64_t id=getPathUniqueId(theRank,ed->m_EXTENSION_currentSeedIndex);
		ed->m_EXTENSION_identifiers.push_back(id);
	}

	/** reset the observations for outer distances utilised during the extension */
	m_pairedScores.clear();

	MACRO_COLLECT_PROFILING_INFORMATION();

	ed->resetStructures(m_profiler,m_runProfiler);
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
	ed->m_EXTENSION_VertexAssembled_requested=false;

	ed->m_flowNumber=0;
	ed->m_previouslyFlowedVertices=0;
}

void SeedExtender::checkIfCurrentVertexIsAssembled(ExtensionData*ed,StaticVector*outbox,RingAllocator*outboxAllocator,
  int*outgoingEdgeIndex,int*last_value,Kmer*currentVertex,int theRank,bool*vertexCoverageRequested,int wordSize,int size,vector<vector<Kmer> >*seeds){

	MACRO_COLLECT_PROFILING_INFORMATION();;

	if(!ed->m_EXTENSION_directVertexDone){
		if(!ed->m_EXTENSION_VertexAssembled_requested){

			MACRO_COLLECT_PROFILING_INFORMATION();;
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
	
			MACRO_COLLECT_PROFILING_INFORMATION();;

			/* if the position is not 0 on flow 0, we don't need to send this message */
			if(!(ed->m_EXTENSION_currentPosition==0 && ed->m_flowNumber==0)){
				ed->m_EXTENSION_vertexIsAssembledResult=false;
				ed->m_EXTENSION_VertexAssembled_received=true;
				return;
			}

			uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(2*sizeof(uint64_t));
			int bufferPosition=0;
			currentVertex->pack(message,&bufferPosition);
			int destination=m_parameters->_vertexRank(currentVertex);
			Message aMessage(message,bufferPosition,destination,RAY_MPI_TAG_ASK_IS_ASSEMBLED,theRank);
			(*outbox).push_back(aMessage);
			ed->m_EXTENSION_VertexAssembled_received=false;

			MACRO_COLLECT_PROFILING_INFORMATION();;
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

	MACRO_COLLECT_PROFILING_INFORMATION();;
}

void SeedExtender::markCurrentVertexAsAssembled(Kmer*currentVertex,RingAllocator*outboxAllocator,int*outgoingEdgeIndex, 
StaticVector*outbox,int size,int theRank,ExtensionData*ed,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,bool*edgesRequested,
vector<Kmer>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,int minimumCoverage,OpenAssemblerChooser*oa,int wordSize,vector<vector<Kmer> >*seeds
){

	MACRO_COLLECT_PROFILING_INFORMATION();;

	if(!m_messengerInitiated){

		MACRO_COLLECT_PROFILING_INFORMATION();;

		m_hasPairedSequences=false;
		*edgesRequested=false;
		m_pickedInformation=false;
		int theCurrentSize=ed->m_EXTENSION_extension.size();

		if(theCurrentSize == 0){
			m_slicedComputationStarted = false;
		}

		int previousPosition=theCurrentSize - 1;

		// don't let things accumulate in this structure...
		// TODO: fix this at the source in the first place...
		// this code does change the result, but reduces the granularity
		if(m_ed->m_expirations.count(previousPosition) > 0){
			vector<uint64_t>*expired=&(m_ed->m_expirations)[previousPosition];

			// erase these reads from the list of reads without mate 
			// because they are expired...
			// this just free some memory and does not change the result.
			for(int i=0;i<(int)expired->size();i++){
				uint64_t readId=expired->at(i);
				m_ed->m_pairedReadsWithoutMate.erase(readId);
			}

			m_ed->m_expirations.erase(previousPosition);
		}

		MACRO_COLLECT_PROFILING_INFORMATION();;

		if(theCurrentSize%10000==0){
			if(theCurrentSize==0 && ed->m_flowNumber ==0){

				m_flowedVertices.clear();

				printf("Rank %i starts on a seed %i, length is %i, flow %i [%i/%i]\n",theRank,
				ed->m_EXTENSION_currentSeedIndex,
				(int)ed->m_EXTENSION_currentSeed.size(),ed->m_flowNumber,
					ed->m_EXTENSION_currentSeedIndex,(int)(*seeds).size());
				fflush(stdout);
				m_flowedVertices.push_back(ed->m_EXTENSION_currentSeed.size());
			
				/* flow #0 is the seed */
				ed->m_flowNumber++;
			}
			printExtensionStatus(currentVertex);
		}
		m_messengerInitiated=true;

		MACRO_COLLECT_PROFILING_INFORMATION();;

		uint64_t waveId=getPathUniqueId(theRank,ed->m_EXTENSION_currentSeedIndex);
		// save wave progress.
		#ifdef ASSERT
		assert((int)getIdFromPathUniqueId(waveId)==ed->m_EXTENSION_currentSeedIndex);
		assert((int)getRankFromPathUniqueId(waveId)==theRank);
		assert(theRank<size);
		#endif

		/**
			Don't fetch read markers if they will not be used.


                        ----------------------------------------      seed
				*			current position
							
							<------>   maximum outer distance


							* threshold position

		Before threshold position, it is useless to fetch read markers.
		*/

		MACRO_COLLECT_PROFILING_INFORMATION();;

		int progression=ed->m_EXTENSION_extension.size()-1;
		int threshold=ed->m_EXTENSION_currentSeed.size()-m_parameters->getMaximumDistance();
		bool getReads=false;
		if(progression>=threshold)
			getReads=true;

		Kmer vertex=*currentVertex;
		m_vertexMessenger.constructor(vertex,waveId,progression,&m_matesToMeet,m_inbox,outbox,outboxAllocator,m_parameters,getReads);

		MACRO_COLLECT_PROFILING_INFORMATION();;

	}else if(!m_vertexMessenger.isDone()){
		m_vertexMessenger.work();

		MACRO_COLLECT_PROFILING_INFORMATION();;
	}else if(!m_pickedInformation){
		m_pickedInformation=true;

		MACRO_COLLECT_PROFILING_INFORMATION();;

		m_sequenceIndexToCache=0;
		ed->m_EXTENSION_receivedReads=m_vertexMessenger.getReadAnnotations();
		*receivedVertexCoverage=m_vertexMessenger.getCoverageValue();
		ed->m_currentCoverage=*receivedVertexCoverage;
		bool inserted;
		*((m_cache.insert(*currentVertex,&m_cacheAllocator,&inserted))->getValue())=ed->m_currentCoverage;
		uint64_t compactEdges=m_vertexMessenger.getEdges();
		*receivedOutgoingEdges=currentVertex->_getOutgoingEdges(compactEdges,m_parameters->getWordSize());
		ed->m_EXTENSION_extension.push_back((*currentVertex));
		ed->m_extensionCoverageValues.push_back(*receivedVertexCoverage);

		#ifdef ASSERT
		assert(ed->m_currentCoverage<=m_parameters->getMaximumAllowedCoverage());
		#endif

		m_sequenceRequested=false;

		MACRO_COLLECT_PROFILING_INFORMATION();;

	}else{
		if(m_sequenceIndexToCache<(int)ed->m_EXTENSION_receivedReads.size()){
			MACRO_COLLECT_PROFILING_INFORMATION();;

			ReadAnnotation annotation=ed->m_EXTENSION_receivedReads[m_sequenceIndexToCache];
			uint64_t uniqueId=annotation.getUniqueId();
			ExtensionElement*anElement=ed->getUsedRead(uniqueId);

			MACRO_COLLECT_PROFILING_INFORMATION();;

			/**
			 * if the read is still within the range of the peak, update it 
			 *
			 * this complicated code add-on avoids the collapsing of
			 * tandemly-repeated repeats.
			 *
			 * instead of outputting collapsed assembled region, the extender module
			 * will let the scaffolder deal with it if it is too difficult.
			 * */
			if(anElement!=NULL){
				if(m_parameters->hasOption("-show-read-placement")){
					int currentPosition=ed->m_EXTENSION_extension.size()-1;
					int previousPosition=anElement->getPosition();
					cout<<"Rank "<<m_parameters->getRank()<<" Notice: Read "<<uniqueId<<" already placed at "<<previousPosition<<", current is "<<currentPosition<<endl;
				}

				if(ed->m_EXTENSION_readsInRange.count(uniqueId)==0 && anElement->hasPairedRead()){
					char theRightStrand=anElement->getStrand();
					PairedRead*pairedRead=anElement->getPairedRead();
					uint64_t mateId=pairedRead->getUniqueId();

					ExtensionElement*extensionElement=ed->getUsedRead(mateId);

					if(extensionElement!=NULL){// use to be via readsPositions
						char theLeftStrand=extensionElement->getStrand();
						int startingPositionOnPath=extensionElement->getPosition();

						int startPosition=ed->m_EXTENSION_extension.size()-1;
						int positionOnStrand=anElement->getStrandPosition();
						anElement->getSequence(m_receivedString,m_parameters);
						int rightReadLength=(int)strlen(m_receivedString);
						int observedFragmentLength=(startPosition-startingPositionOnPath)+rightReadLength+extensionElement->getStrandPosition()-positionOnStrand;
						int multiplier=3;

						int library=ed->m_EXTENSION_pairedRead.getLibrary();

						bool updateRead=false;
						/** : iterate over all peaks */
						/** if there is a mate, choose the good peak for the library */
						for(int peak=0;peak<m_parameters->getLibraryPeaks(library);peak++){
							int expectedFragmentLength=m_parameters->getLibraryAverageLength(library,peak);
							int expectedDeviation=m_parameters->getLibraryStandardDeviation(library,peak);

							if(expectedFragmentLength-multiplier*expectedDeviation<=observedFragmentLength 
							&& observedFragmentLength <= expectedFragmentLength+multiplier*expectedDeviation 
					&&( (theLeftStrand=='F' && theRightStrand=='R')
						||(theLeftStrand=='R' && theRightStrand=='F'))
					// the bridging pair is meaningless if both start in repeats
					/*&&repeatLengthForLeftRead<repeatThreshold*/
					/* left read is safe so we don't care if right read is on a
					repeated region really. */){
								/* as soon as we find something interesting, we stop */
								/* this makes Ray segfault because */
								updateRead=true;
								break;
							}
						}
		
						if(updateRead && anElement->canMove()){
							anElement->setStartingPosition(startPosition);
							ed->m_EXTENSION_readsInRange.insert(uniqueId);
							int expiryPosition=startPosition+rightReadLength-positionOnStrand-m_parameters->getWordSize();
							m_expiredReads[expiryPosition].push_back(uniqueId);

							/** free the mate to avoid infinite loops */
							extensionElement->freezePlacement();

							if(m_parameters->hasOption("-show-read-placement"))
								cout<<"Rank "<<m_parameters->getRank()<<" Updated Read "<<uniqueId<<" to "<<startPosition<<" Mate is "<<mateId<<" at "<<startingPositionOnPath<<endl;
						}
					}
				}

				MACRO_COLLECT_PROFILING_INFORMATION();;

				m_sequenceIndexToCache++;

			/** this case never happens because m_cacheForRepeatedReads is never populated */
			}else if(!m_sequenceRequested
				&&m_cacheForRepeatedReads.find(uniqueId,false)!=NULL){
				SplayNode<uint64_t,Read>*node=m_cacheForRepeatedReads.find(uniqueId,false);
				#ifdef ASSERT
				assert(node!=NULL);
				#endif
				node->getValue()->getSeq(m_receivedString,m_parameters->getColorSpaceMode(),false);
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


				MACRO_COLLECT_PROFILING_INFORMATION();;

			/** send a message to get the read */
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
				Message aMessage(message,1,sequenceRank,RAY_MPI_TAG_REQUEST_READ_SEQUENCE,theRank);
				outbox->push_back(aMessage);

				MACRO_COLLECT_PROFILING_INFORMATION();;

			/* we received a sequence read */
			}else if(m_sequenceReceived){

				bool addRead=true;
				int startPosition=ed->m_EXTENSION_extension.size()-1;
				int readLength=strlen(m_receivedString);
				int position=startPosition;
				int wordSize=m_parameters->getWordSize();
				int positionOnStrand=annotation.getPositionOnStrand();
				char theRightStrand=annotation.getStrand();

				/** only one 1 k-mer is useless for the extension. */
				int availableLength=readLength-positionOnStrand;
				if(availableLength<=wordSize){
					addRead=false;
				}

				MACRO_COLLECT_PROFILING_INFORMATION();;

				// don't add it up if its is marked on a repeated vertex and
				// its mate was not seen yet.
				
				if(addRead && ed->m_currentCoverage>=3*m_parameters->getPeakCoverage()){
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
				if(addRead && ed->m_EXTENSION_pairedRead.getLibrary()!=DUMMY_LIBRARY){
					uint64_t mateId=ed->m_EXTENSION_pairedRead.getUniqueId();
					// the mate is required to allow proper placement
					
					ExtensionElement*extensionElement=ed->getUsedRead(mateId);

					if(extensionElement!=NULL){// use to be via readsPositions
						char theLeftStrand=extensionElement->getStrand();
						int startingPositionOnPath=extensionElement->getPosition();

						//int repeatLengthForLeftRead=ed->m_repeatedValues->at(startingPositionOnPath);
						int observedFragmentLength=(startPosition-startingPositionOnPath)+ed->m_EXTENSION_receivedLength+extensionElement->getStrandPosition()-positionOnStrand;
						int multiplier=3;

						int library=ed->m_EXTENSION_pairedRead.getLibrary();

						//int repeatThreshold=100;

						/** : iterate over all peaks */
						/** if there is a mate, choose the good peak for the library */
						for(int peak=0;peak<m_parameters->getLibraryPeaks(library);peak++){
							int expectedFragmentLength=m_parameters->getLibraryAverageLength(library,peak);
							int expectedDeviation=m_parameters->getLibraryStandardDeviation(library,peak);

							if(expectedFragmentLength-multiplier*expectedDeviation<=observedFragmentLength 
							&& observedFragmentLength <= expectedFragmentLength+multiplier*expectedDeviation 
					&&( (theLeftStrand=='F' && theRightStrand=='R')
						||(theLeftStrand=='R' && theRightStrand=='F'))
					// the bridging pair is meaningless if both start in repeats
					/*&&repeatLengthForLeftRead<repeatThreshold*/
					/* left read is safe so we don't care if right read is on a
					repeated region really. */){
								/* as soon as we find something interesting, we stop */
								/* this makes Ray segfault because */
								addRead=true;
								break;
							}else{
								// remove the right read from the used set
								addRead=false;
							}
						}
					}
				}

				MACRO_COLLECT_PROFILING_INFORMATION();;

				/* after making sure the read is sane, we can add it here for sure */
				if(addRead){
					//cout<<"Adding read "<<uniqueId<<" at "<<position<<endl;
					m_matesToMeet.erase(uniqueId);
					ExtensionElement*element=ed->addUsedRead(uniqueId);

					element->setSequence(m_receivedString,ed->getAllocator());
					element->setStartingPosition(startPosition);
					element->setStrand(annotation.getStrand());
					element->setStrandPosition(annotation.getPositionOnStrand());
					element->setType(ed->m_readType);
					ed->m_EXTENSION_readsInRange.insert(uniqueId);

					MACRO_COLLECT_PROFILING_INFORMATION();;

					#ifdef ASSERT
					element->getSequence(m_receivedString,m_parameters);
					assert(readLength==(int)strlen(m_receivedString));
					#endif
		
					int expiryPosition=position+readLength-positionOnStrand-wordSize;
		
					if(m_parameters->hasOption("-show-read-placement")){
						cout<<"Read "<<uniqueId<<" will expire at "<<expiryPosition<<endl;
					}

					MACRO_COLLECT_PROFILING_INFORMATION();;

					m_expiredReads[expiryPosition].push_back(uniqueId);
					// received paired read too !
					if(ed->m_EXTENSION_pairedRead.getLibrary()!=DUMMY_LIBRARY){
						element->setPairedRead(ed->m_EXTENSION_pairedRead);
						uint64_t mateId=ed->m_EXTENSION_pairedRead.getUniqueId();
						if(ed->getUsedRead(mateId)==NULL){// the mate has not shown up yet
							ed->m_pairedReadsWithoutMate.insert(uniqueId);

							int library=ed->m_EXTENSION_pairedRead.getLibrary();

							/** use the maximum peak for expiry positions */
							int expectedFragmentLength=m_parameters->getLibraryMaxAverageLength(library);
							int expectedDeviation=m_parameters->getLibraryMaxStandardDeviation(library);
							int expiration=startPosition+expectedFragmentLength+3*expectedDeviation;

							(ed->m_expirations)[expiration].push_back(uniqueId);
		
							MACRO_COLLECT_PROFILING_INFORMATION();;

							m_matesToMeet.insert(mateId);
						}else{ // the mate has shown up already and was waiting
							ed->m_pairedReadsWithoutMate.erase(mateId);
						}
					}
				}

				m_sequenceIndexToCache++;
				m_sequenceRequested=false;

				MACRO_COLLECT_PROFILING_INFORMATION();;
			}
		}else{
			ed->m_EXTENSION_directVertexDone=true;
			ed->m_EXTENSION_VertexMarkAssembled_requested=false;
			ed->m_EXTENSION_enumerateChoices=false;
			(*edgesRequested)=false;
			ed->m_EXTENSION_markedCurrentVertexAsAssembled=true;

			MACRO_COLLECT_PROFILING_INFORMATION();;
		}
	}

	MACRO_COLLECT_PROFILING_INFORMATION();
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
	GridTable*subgraph,StaticVector*inbox,Profiler*profiler){

	m_checkedCheckpoint=false;

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

	m_profiler=profiler;

	m_runProfiler = m_parameters->runProfiler();
}

void SeedExtender::inspect(ExtensionData*ed,Kmer*currentVertex){
	#ifdef ASSERT
	assert(ed->m_enumerateChoices_outgoingEdges.size()==ed->m_EXTENSION_coverages.size());
	#endif


	int wordSize=m_parameters->getWordSize();
	cout<<endl;
	cout<<"*****************************************"<<endl;
	cout<<"CurrentVertex="<<currentVertex->idToWord(wordSize,m_parameters->getColorSpaceMode())<<" @"<<ed->m_EXTENSION_extension.size()<<endl;
	#ifdef ASSERT
	assert(ed->m_currentCoverage<=m_parameters->getMaximumAllowedCoverage());
	#endif
	cout<<"Coverage="<<ed->m_currentCoverage<<endl;
	cout<<" # ReadsInRange: "<<ed->m_EXTENSION_readsInRange.size()<<endl;
	cout<<ed->m_enumerateChoices_outgoingEdges.size()<<" choices ";

	cout<<endl;
	for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
		string vertex=ed->m_enumerateChoices_outgoingEdges[i].idToWord(wordSize,m_parameters->getColorSpaceMode());
		Kmer key=ed->m_enumerateChoices_outgoingEdges[i];
		cout<<endl;
		cout<<"Choice #"<<i+1<<endl;
		cout<<"Vertex: "<<vertex<<endl;
		#ifdef ASSERT
		if(i>=(int)ed->m_EXTENSION_coverages.size()){
			cout<<"Error: i="<<i<<" Size="<<ed->m_EXTENSION_coverages.size()<<endl;
		}
		assert(i<(int)ed->m_EXTENSION_coverages.size());
		#endif
		cout<<"Coverage="<<ed->m_EXTENSION_coverages.at(i)<<endl;
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

			/** TODO: iterate over all peaks */
			int averageLength=m_parameters->getLibraryAverageLength(j->first,0);
			int stddev=m_parameters->getLibraryStandardDeviation(j->first,0);
			int sum=0;
			int n=0;
			for(int k=0;k<(int)j->second.size();k++){
				int val=j->second[k];

				/** if there are 2 peaks, we just accept everything */
				if(m_parameters->getLibraryPeaks(j->first)>1)
					acceptedValues.push_back(val);

				sum+=val;
				n++;
			}

			/** if there are 2 peaks, we just accept everything */
			if(m_parameters->getLibraryPeaks(j->first)>1)
				continue;

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
					if(m_ed->m_EXTENSION_extension.size()>10000){
						cout<<"Restoring Value="<<j->second[k]<<" Expected="<<averageLength<<" Dev="<<stddev<<" MeanForLibrary="<<mean<<" n="<<n<<endl;
					}
					#endif
					m_ed->m_sequencesToFree.push_back(uniqueId);
				}
			}
		}
/*
		if(m_ed->m_EXTENSION_pairedReadPositionsForVertices[vertex].size()>0)
			cout<<"Removing unfit, before "<<m_ed->m_EXTENSION_pairedReadPositionsForVertices[vertex].size()<<" after "<<acceptedValues.size()<<endl;
*/

		m_ed->m_EXTENSION_pairedReadPositionsForVertices[vertex]=acceptedValues;
		if(!acceptedValues.empty()){
			hasPairedSequences=true;
		}
	}
	m_hasPairedSequences=hasPairedSequences;
}

void SeedExtender::setFreeUnmatedPairedReads(){
	if(!m_hasPairedSequences){// avoid infinite loops.
		//cout<<"No pairs"<<endl;
		return;
	}

	if(m_ed->m_expirations.count(m_ed->m_EXTENSION_extension.size())==0){
		//cout<<"Nothing expires"<<endl;
		return;
	}

	vector<uint64_t>*expired=&(m_ed->m_expirations)[m_ed->m_EXTENSION_extension.size()];

	//cout<<"Items expiring: "<<expired->size()<<endl;

	for(int i=0;i<(int)expired->size();i++){
		uint64_t readId=expired->at(i);
		if(m_ed->m_pairedReadsWithoutMate.count(readId)>0){
			m_ed->m_sequencesToFree.push_back(readId); // RECYCLING IS desactivated
			#ifdef HUNT_INFINITE_BUG
			if(m_ed->m_EXTENSION_extension.size()>10000){
				cout<<"Expired: Now="<<m_ed->m_EXTENSION_extension.size()-1<<" Id="<<readId<<endl;
			}
			#endif
		}
	}
	m_ed->m_expirations.erase(m_ed->m_EXTENSION_extension.size());
}

void SeedExtender::showReadsInRange(){
	cout<<"Reads in range ("<<m_ed->m_EXTENSION_readsInRange.size()<<"):";
	for(set<uint64_t>::iterator i=m_ed->m_EXTENSION_readsInRange.begin();
		i!=m_ed->m_EXTENSION_readsInRange.end();i++){
		cout<<" "<<*i;
	}
	cout<<endl;
	cout.flush();
}

void SeedExtender::printExtensionStatus(Kmer*currentVertex){
	int theRank=m_parameters->getRank();

	printf("Rank %i reached %i vertices from seed %i, flow %i\n",theRank,
		(int)m_ed->m_EXTENSION_extension.size(),
		m_ed->m_EXTENSION_currentSeedIndex,m_ed->m_flowNumber);

	fflush(stdout);

/*
	cout<<"Expiration.size= "<<(m_ed->m_expirations).size()<<endl;
	cout<<"Entries: "<<endl;
	for(map<int,vector<uint64_t> >::iterator i=m_ed->m_expirations.begin();i!=m_ed->m_expirations.end();i++){
		cout<<i->first<<" "<<i->second.size()<<endl;
	}
*/

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(theRank);
	}

	#ifdef SHOW_READS_IN_RANGE
	showReadsInRange();
	#endif
}

void SeedExtender::writeCheckpoint(){
	cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint Extensions"<<endl;
	ofstream f(m_parameters->getCheckpointFile("Extensions").c_str());

	int count=m_ed->m_EXTENSION_contigs.size();
	f.write((char*)&count,sizeof(int));

	for(int i=0;i<count;i++){
		int length=m_ed->m_EXTENSION_contigs[i].size();
		f.write((char*)&length,sizeof(int));
		for(int j=0;j<length;j++){
			m_ed->m_EXTENSION_contigs[i][j].write(&f);
		}
	}
	f.close();
}

void SeedExtender::readCheckpoint(){
	cout<<"Rank "<<m_parameters->getRank()<<" is reading checkpoint Extensions"<<endl;
	ifstream f(m_parameters->getCheckpointFile("Extensions").c_str());

	#ifdef ASSERT
	assert(m_ed->m_EXTENSION_contigs.size()==0);
	#endif

	int count=0;
	f.read((char*)&count,sizeof(int));

	for(int i=0;i<count;i++){
		int length=0;
		f.read((char*)&length,sizeof(int));
		#ifdef ASSERT
		assert(length>0);
		#endif
		vector<Kmer> extension;
		for(int j=0;j<length;j++){
			Kmer kmer;
			kmer.read(&f);
			extension.push_back(kmer);
		}
		m_ed->m_EXTENSION_contigs.push_back(extension);

		/* add the identifier */
		uint64_t id=getPathUniqueId(m_parameters->getRank(),m_ed->m_EXTENSION_contigs.size()-1);
		m_ed->m_EXTENSION_identifiers.push_back(id);
	}

	f.close();

	cout<<"Rank "<<m_parameters->getRank()<<" loaded "<<count<<" extensions from checkpoint Extensions"<<endl;
	
	#ifdef ASSERT
	assert(count==(int)m_ed->m_EXTENSION_contigs.size());
	assert(m_ed->m_EXTENSION_identifiers.size()==m_ed->m_EXTENSION_contigs.size());
	#endif

}
