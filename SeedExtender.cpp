/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<RepeatedVertexWatchdog.h>
#include<TipWatchdog.h>
#include<SeedExtender.h>
#include<Chooser.h>
#ifdef DEBUG
#include<assert.h>
#endif
#include<BubbleTool.h>

void SeedExtender::extendSeeds(vector<vector<VERTEX_TYPE> >*seeds,ExtensionData*ed,int theRank,vector<Message>*outbox,
  u64*currentVertex,FusionData*fusionData,MyAllocator*outboxAllocator,bool*edgesRequested,int*outgoingEdgeIndex,
int*last_value,bool*vertexCoverageRequested,int wordSize,bool*colorSpaceMode,int size,bool*vertexCoverageReceived,
int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,vector<VERTEX_TYPE>*receivedOutgoingEdges,Chooser*chooser,
ChooserData*cd,BubbleData*bubbleData,DepthFirstSearchData*dfsData,
int minimumCoverage,OpenAssemblerChooser*oa,bool*edgesReceived


){
	if((*seeds).size()==0){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<theRank<<" is extending its seeds. "<<(*seeds).size()<<"/"<<(*seeds).size()<<" (DONE)"<<endl;
		#endif
		ed->m_mode_EXTENSION=false;
		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_IS_DONE,theRank);
		(*outbox).push_back(aMessage);
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
		ed->m_EXTENSION_extension.clear();
		ed->m_EXTENSION_complementedSeed=false;
		ed->m_EXTENSION_reads_startingPositionOnContig.clear();
		ed->m_EXTENSION_readsInRange.clear();
	}else if(ed->m_EXTENSION_currentSeedIndex==(int)(*seeds).size()){
		#ifdef SHOW_PROGRESS
		cout<<"Rank "<<theRank<<" is extending its seeds. "<<(*seeds).size()<<"/"<<(*seeds).size()<<" (DONE)"<<endl;
		#endif
		ed->m_mode_EXTENSION=false;
		
		// store the lengths.
		for(int i=0;i<(int)ed->m_EXTENSION_identifiers.size();i++){
			int id=ed->m_EXTENSION_identifiers[i];
			fusionData->m_FUSION_identifier_map[id]=i;
		}

		Message aMessage(NULL,0,MPI_UNSIGNED_LONG_LONG,MASTER_RANK,TAG_EXTENSION_IS_DONE,theRank);
		#ifdef SHOW_PROGRESS
		#endif
		(*outbox).push_back(aMessage);
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
	
	if(!ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled){
		checkIfCurrentVertexIsAssembled(ed,outbox,outboxAllocator,outgoingEdgeIndex,last_value,
	currentVertex,theRank,vertexCoverageRequested,wordSize,colorSpaceMode,size,seeds);
	}else if(ed->m_EXTENSION_vertexIsAssembledResult and ed->m_EXTENSION_currentPosition==0 and ed->m_EXTENSION_complementedSeed==false){
		ed->m_EXTENSION_currentSeedIndex++;// skip the current one.
		ed->m_EXTENSION_currentPosition=0;
		ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
		ed->m_EXTENSION_directVertexDone=false;
		ed->m_EXTENSION_VertexAssembled_requested=false;
		if(ed->m_EXTENSION_currentSeedIndex<(int)(*seeds).size()){
			ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
			(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
		}
		// TODO: check if the position !=0
		ed->m_EXTENSION_complementedSeed=false;
		ed->m_EXTENSION_extension.clear();
		ed->m_EXTENSION_reads_startingPositionOnContig.clear();
		ed->m_EXTENSION_readsInRange.clear();
	}else if(!ed->m_EXTENSION_markedCurrentVertexAsAssembled){
		markCurrentVertexAsAssembled(currentVertex,outboxAllocator,outgoingEdgeIndex,outbox,
size,theRank,ed,vertexCoverageRequested,vertexCoverageReceived,receivedVertexCoverage,
repeatedLength,maxCoverage,edgesRequested,receivedOutgoingEdges,chooser,cd,bubbleData,minimumCoverage,
oa,colorSpaceMode,wordSize);
	}else if(!ed->m_EXTENSION_enumerateChoices){
		enumerateChoices(edgesRequested,ed,edgesReceived,outboxAllocator,outgoingEdgeIndex,outbox,
		currentVertex,theRank,vertexCoverageRequested,receivedOutgoingEdges,
		vertexCoverageReceived,size,receivedVertexCoverage,chooser,cd,wordSize);
	}else if(!ed->m_EXTENSION_choose){
		doChoice(outboxAllocator,outgoingEdgeIndex,outbox,currentVertex,cd,bubbleData,theRank,
	dfsData,wordSize,
	ed,minimumCoverage,*maxCoverage,oa,chooser,colorSpaceMode,seeds,
edgesRequested,vertexCoverageRequested,vertexCoverageReceived,size,receivedVertexCoverage,edgesReceived,
receivedOutgoingEdges);
	}

}

// upon successful completion, ed->m_EXTENSION_coverages and ed->m_enumerateChoices_outgoingEdges are
// populated variables.
void SeedExtender::enumerateChoices(bool*edgesRequested,ExtensionData*ed,bool*edgesReceived,MyAllocator*outboxAllocator,
	int*outgoingEdgeIndex,vector<Message>*outbox,
VERTEX_TYPE*currentVertex,int theRank,bool*vertexCoverageRequested,vector<VERTEX_TYPE>*receivedOutgoingEdges,
bool*vertexCoverageReceived,int size,int*receivedVertexCoverage,Chooser*chooser,ChooserData*cd,int wordSize
){
	if(!(*edgesRequested)){
		ed->m_EXTENSION_coverages.clear();
		(*edgesReceived)=false;
		(*edgesRequested)=true;
		VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
		message[0]=(VERTEX_TYPE)(*currentVertex);
		Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank((*currentVertex),size),TAG_REQUEST_VERTEX_OUTGOING_EDGES,theRank);
		(*outbox).push_back(aMessage);
		ed->m_EXTENSION_currentPosition++;
		(*vertexCoverageRequested)=false;
		(*outgoingEdgeIndex)=0;
	}else if((*edgesReceived)){
		if((*outgoingEdgeIndex)<(int)(*receivedOutgoingEdges).size()){
			// get the coverage of these.
			if(!(*vertexCoverageRequested)){
				VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
				message[0]=(VERTEX_TYPE)(*receivedOutgoingEdges)[(*outgoingEdgeIndex)];
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],size),
	TAG_REQUEST_VERTEX_COVERAGE,theRank);
				(*outbox).push_back(aMessage);
				(*vertexCoverageRequested)=true;
				(*vertexCoverageReceived)=false;
				(*receivedVertexCoverage)=-1;
			}else if((*vertexCoverageReceived)){
				(*outgoingEdgeIndex)++;
				(*vertexCoverageRequested)=false;
				ed->m_EXTENSION_coverages.push_back((*receivedVertexCoverage));
			}
		}else{
			ed->m_EXTENSION_enumerateChoices=true;
			ed->m_EXTENSION_choose=false;
			ed->m_EXTENSION_singleEndResolution=false;
			ed->m_EXTENSION_readIterator=ed->m_EXTENSION_readsInRange.begin();
			ed->m_EXTENSION_readsOutOfRange.clear();
			ed->m_EXTENSION_readLength_done=false;
			ed->m_EXTENSION_readLength_requested=false;
			ed->m_EXTENSION_readPositionsForVertices.clear();
			ed->m_EXTENSION_pairedReadPositionsForVertices.clear();
			
			(*chooser).clear(cd->m_CHOOSER_theSumsPaired,4);
			#ifdef DEBUG
			for(int i=0;i<4;i++){
				assert(cd->m_CHOOSER_theSumsPaired[i]==0);
			}
			#endif
			(*chooser).clear(cd->m_CHOOSER_theNumbersPaired,4);
			(*chooser).clear(cd->m_CHOOSER_theMaxsPaired,4);
			(*chooser).clear(cd->m_CHOOSER_theMaxs,4);
			(*chooser).clear(cd->m_CHOOSER_theNumbers,4);
			(*chooser).clear(cd->m_CHOOSER_theSums,4);

			ed->m_enumerateChoices_outgoingEdges=(*receivedOutgoingEdges);
			

			// nothing to trim...
			if(ed->m_enumerateChoices_outgoingEdges.size()<=1)
				return;

			// avoid unecessary machine instructions
			if(ed->m_EXTENSION_currentPosition<(int)ed->m_EXTENSION_currentSeed.size()){
				return;
			}

			// only keep those with more than 1 coverage.
			vector<int> filteredCoverages;
			vector<VERTEX_TYPE> filteredVertices;
			for(int i=0;i<(int)(*receivedOutgoingEdges).size();i++){
				int coverage=ed->m_EXTENSION_coverages[i];
				VERTEX_TYPE aVertex=(*receivedOutgoingEdges)[i];
				#ifdef SHOW_PROGRESS
				#endif
				if(coverage>=_MINIMUM_COVERAGE){
					filteredCoverages.push_back(coverage);
					filteredVertices.push_back(aVertex);
				}
			}
			#ifdef SHOW_PROGRESS
			#endif
			#ifdef SHOW_PROGRESS
			if(filteredCoverages.size()==0)
				cout<<"Now Zero"<<endl;
			#endif
			#ifdef DEBUG
			assert(filteredCoverages.size()==filteredVertices.size());
			assert(ed->m_EXTENSION_coverages.size()==(*receivedOutgoingEdges).size());
			assert(ed->m_enumerateChoices_outgoingEdges.size()==(*receivedOutgoingEdges).size());
			#endif
	
			// toss them in vectors
			#ifdef SHOW_FILTER
			cout<<"FILTER says ";
			for(int i=0;i<(int)ed->m_EXTENSION_coverages.size();i++){
				int coverage=ed->m_EXTENSION_coverages[i];
				VERTEX_TYPE aVertex=ed->m_enumerateChoices_outgoingEdges[i];
				cout<<" ("<<idToWord(aVertex,wordSize)<<","<<coverage<<")";
			}
			cout<<" -> ";
			for(int i=0;i<(int)filteredVertices.size();i++){
				int coverage=filteredCoverages[i];
				VERTEX_TYPE aVertex=filteredVertices[i];
				cout<<" ("<<idToWord(aVertex,wordSize)<<","<<coverage<<")";
			}
			cout<<" ."<<endl;
			#endif
			#ifdef DEBUG
			assert(filteredVertices.size()<=ed->m_enumerateChoices_outgoingEdges.size());
			assert(filteredCoverages.size()<=ed->m_EXTENSION_coverages.size());
			#endif
			ed->m_EXTENSION_coverages=filteredCoverages;
			ed->m_enumerateChoices_outgoingEdges=filteredVertices;
			#ifdef DEBUG
			assert(ed->m_EXTENSION_coverages.size()==ed->m_enumerateChoices_outgoingEdges.size());
			#endif
		}
	}
}


#define _UPDATE_SINGLE_VALUES(d) \
if(distance>cd->m_CHOOSER_theMaxs[ed->m_EXTENSION_edgeIterator])\
	cd->m_CHOOSER_theMaxs[ed->m_EXTENSION_edgeIterator]=distance;\
cd->m_CHOOSER_theNumbers[ed->m_EXTENSION_edgeIterator]++;\
cd->m_CHOOSER_theSums[ed->m_EXTENSION_edgeIterator]+=distance;


/**
 *
 *  This function do a choice:
 *   IF the position is inside the given seed, THEN the seed is used as a backbone to do the choice
 *   IF the position IS NOT inside the given seed, THEN
 *      reads in range are mapped on available choices.
 *      then, Ray attempts to choose with paired-end reads
 *      if this fails, Ray attempts to choose with single-end reads
 *      if this fails, Ray attempts to choose with only the number of reads covering arcs
 *      if this fails, Ray attempts to choose by removing tips.
 *      if this fails, Ray attempts to choose by resolving bubbles (NOT IMPLEMENTED YET)
 */
void SeedExtender::doChoice(MyAllocator*outboxAllocator,int*outgoingEdgeIndex,vector<Message>*outbox,
	VERTEX_TYPE*currentVertex,ChooserData*cd,BubbleData*bubbleData,int theRank,DepthFirstSearchData*dfsData,
	int wordSize,
ExtensionData*ed,int minimumCoverage,int maxCoverage,OpenAssemblerChooser*oa,Chooser*chooser,bool*colorSpaceMode,
	vector<vector<VERTEX_TYPE> >*seeds,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,int size,
int*receivedVertexCoverage,bool*edgesReceived,vector<VERTEX_TYPE>*receivedOutgoingEdges
){
	// use seed information.
	#ifdef SHOW_PROGRESS
	if(ed->m_EXTENSION_currentPosition==1)
		cout<<"Rank "<<theRank<<""<<" starts on a seed, length="<<ed->m_EXTENSION_currentSeed.size()<<endl;
	#endif
	
	// use the seed to extend the thing.
	if(ed->m_EXTENSION_currentPosition<(int)ed->m_EXTENSION_currentSeed.size()){
		#ifdef SHOW_EXTEND_WITH_SEED
		cout<<"Extending with seed, p="<<ed->m_EXTENSION_currentPosition<<endl;
		#endif

		// a speedy test follows, using mighty MACROs
		#define _UNROLLED_LOOP(i) if(ed->m_enumerateChoices_outgoingEdges.size()>=(i+1)){ \
			if(ed->m_enumerateChoices_outgoingEdges[i]==ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition]){ \
				(*currentVertex)=ed->m_enumerateChoices_outgoingEdges[i]; \
				ed->m_EXTENSION_choose=true; \
				ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false; \
				ed->m_EXTENSION_directVertexDone=false; \
				ed->m_EXTENSION_VertexAssembled_requested=false; \
				return; \
			}\
		}
		_UNROLLED_LOOP(0);
		_UNROLLED_LOOP(1);
		_UNROLLED_LOOP(2);
		_UNROLLED_LOOP(3);
		#ifdef SHOW_EXTEND_WITH_SEED
		cout<<"What the hell? position="<<ed->m_EXTENSION_currentPosition<<" "<<idToWord(ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition],wordSize)<<" with choices ";
		for(int j=0;j<(int)ed->m_enumerateChoices_outgoingEdges.size();j++){
			cout<<" "<<idToWord(ed->m_enumerateChoices_outgoingEdges[j],wordSize)<<endl;
		}
		cout<<endl;
		#endif

		#ifdef DEBUG
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
		if(!ed->m_EXTENSION_singleEndResolution and ed->m_EXTENSION_readsInRange.size()>0){
			// try to use single-end reads to resolve the repeat.
			// for each read in range, ask them their vertex at position (CurrentPositionOnContig-StartPositionOfReadOnContig)
			// and cumulate the results in
			// ed->m_EXTENSION_readPositions, which is a map<int,vector<int> > if one of the vertices match
			if(ed->m_EXTENSION_readIterator!=ed->m_EXTENSION_readsInRange.end()){
				if(!ed->m_EXTENSION_readLength_done){
					if(!ed->m_EXTENSION_readLength_requested){
						ed->m_EXTENSION_readLength_requested=true;
						ed->m_EXTENSION_readLength_received=false;

						ReadAnnotation annotation=*ed->m_EXTENSION_readIterator;
						VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
						message[0]=annotation.getReadIndex();
						Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_ASK_READ_LENGTH,theRank);
						(*outbox).push_back(aMessage);
					}else if(ed->m_EXTENSION_readLength_received){
						ReadAnnotation annotation=*ed->m_EXTENSION_readIterator;

						int startPosition=ed->m_EXTENSION_reads_startingPositionOnContig[annotation.getUniqueId()];
						int distance=ed->m_EXTENSION_extension.size()-startPosition;
						if(distance>(ed->m_EXTENSION_receivedLength-wordSize)){
							// the read is now out-of-range.
							ed->m_EXTENSION_readsOutOfRange.push_back(annotation);
							ed->m_EXTENSION_readLength_done=false;
							ed->m_EXTENSION_readLength_requested=false;
							ed->m_EXTENSION_readIterator++;
						}else{
							//the read is in-range
							ed->m_EXTENSION_readLength_done=true;
							ed->m_EXTENSION_read_vertex_requested=false;
						}
					}
				}else if(!ed->m_EXTENSION_read_vertex_requested){
					// request the vertex for the read
					ed->m_EXTENSION_read_vertex_requested=true;
					ReadAnnotation annotation=*ed->m_EXTENSION_readIterator;
					int startPosition=ed->m_EXTENSION_reads_startingPositionOnContig[annotation.getUniqueId()];
					if(!(0<=startPosition and startPosition<(int)ed->m_EXTENSION_extension.size())){
						cout<<"FATAL"<<endl;
						cout<<"The read started at "<<startPosition<<endl;
						cout<<"The extension has "<<ed->m_EXTENSION_extension.size()<<" elements."<<endl;
						cout<<"The read has length="<<ed->m_EXTENSION_receivedLength<<endl;
					}
					int distance=ed->m_EXTENSION_extension.size()-startPosition;
					VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(3*sizeof(VERTEX_TYPE));
					message[0]=annotation.getReadIndex();
					message[1]=distance;
					message[2]=annotation.getStrand();
					Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_ASK_READ_VERTEX_AT_POSITION,theRank);
					(*outbox).push_back(aMessage);
					ed->m_EXTENSION_read_vertex_received=false;
					ed->m_EXTENSION_edgeIterator=0;
					ed->m_EXTENSION_hasPairedReadRequested=false;
				}else if(ed->m_EXTENSION_read_vertex_received){
					// we received the vertex for that read,
					// now check if it matches one of 
					// the many choices we have
					ReadAnnotation annotation=*ed->m_EXTENSION_readIterator;
					int startPosition=ed->m_EXTENSION_reads_startingPositionOnContig[annotation.getUniqueId()];
					int distance=ed->m_EXTENSION_extension.size()-startPosition;

					// process each edge separately.
					if(ed->m_EXTENSION_edgeIterator<(int)ed->m_enumerateChoices_outgoingEdges.size()){
						// got a match!
						if(ed->m_EXTENSION_receivedReadVertex==ed->m_enumerateChoices_outgoingEdges[ed->m_EXTENSION_edgeIterator]){
							ReadAnnotation annotation=*ed->m_EXTENSION_readIterator;
							// check if the current read has a paired read.
							if(!ed->m_EXTENSION_hasPairedReadRequested){
								VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
								message[0]=annotation.getReadIndex();
								Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_HAS_PAIRED_READ,theRank);
								(*outbox).push_back(aMessage);
								ed->m_EXTENSION_hasPairedReadRequested=true;
								ed->m_EXTENSION_hasPairedReadReceived=false;
								ed->m_EXTENSION_pairedSequenceRequested=false;
							}else if(ed->m_EXTENSION_hasPairedReadReceived){
								// vertex matches, but no paired end read found, at last.
								if(!ed->m_EXTENSION_hasPairedReadAnswer){
									ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_edgeIterator].push_back(distance);

									_UPDATE_SINGLE_VALUES(distance);

									ed->m_EXTENSION_edgeIterator++;
									ed->m_EXTENSION_hasPairedReadRequested=false;
								}else{
									// get the paired end read.
									if(!ed->m_EXTENSION_pairedSequenceRequested){
										ed->m_EXTENSION_pairedSequenceRequested=true;
										VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
										message[0]=annotation.getReadIndex();
										Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,annotation.getRank(),TAG_GET_PAIRED_READ,theRank);
										(*outbox).push_back(aMessage);
										ed->m_EXTENSION_pairedSequenceReceived=false;
									}else if(ed->m_EXTENSION_pairedSequenceReceived){
										int expectedFragmentLength=ed->m_EXTENSION_pairedRead.getAverageFragmentLength();
										int expectedDeviation=ed->m_EXTENSION_pairedRead.getStandardDeviation();
										int rank=ed->m_EXTENSION_pairedRead.getRank();
										int id=ed->m_EXTENSION_pairedRead.getId();
										int uniqueReadIdentifier=id*MAX_NUMBER_OF_MPI_PROCESSES+rank;
			
										// it is mandatory for a read to start at 0. at position X on the path.
										if(ed->m_EXTENSION_reads_startingPositionOnContig.count(uniqueReadIdentifier)>0){
											int startingPositionOnPath=ed->m_EXTENSION_reads_startingPositionOnContig[uniqueReadIdentifier];
											int observedFragmentLength=(startPosition-startingPositionOnPath)+ed->m_EXTENSION_receivedLength;
											if(expectedFragmentLength-expectedDeviation<=observedFragmentLength and
											observedFragmentLength <= expectedFragmentLength+expectedDeviation){
											// it matches!
												int theDistance=startPosition-startingPositionOnPath+distance;
												ed->m_EXTENSION_pairedReadPositionsForVertices[ed->m_EXTENSION_edgeIterator].push_back(theDistance);
												if(theDistance>cd->m_CHOOSER_theMaxsPaired[ed->m_EXTENSION_edgeIterator])
													cd->m_CHOOSER_theMaxsPaired[ed->m_EXTENSION_edgeIterator]=theDistance;
												cd->m_CHOOSER_theNumbersPaired[ed->m_EXTENSION_edgeIterator]++;
												cd->m_CHOOSER_theSumsPaired[ed->m_EXTENSION_edgeIterator]+=theDistance;
											}

										}
										
										// add it anyway as a single-end match too!
										ed->m_EXTENSION_readPositionsForVertices[ed->m_EXTENSION_edgeIterator].push_back(distance);
										ed->m_EXTENSION_hasPairedReadRequested=false;

										_UPDATE_SINGLE_VALUES(distance);
										ed->m_EXTENSION_edgeIterator++;
									}
								}
							}else{
							}
						}else{// no match, too bad.
							ed->m_EXTENSION_edgeIterator++;
							ed->m_EXTENSION_hasPairedReadRequested=false;
						}
					}else{
						ed->m_EXTENSION_readLength_done=false;
						ed->m_EXTENSION_readLength_requested=false;
						ed->m_EXTENSION_readIterator++;
					}
				}
			}else{
				// remove reads that are no longer in-range.
				for(int i=0;i<(int)ed->m_EXTENSION_readsOutOfRange.size();i++){
					ed->m_EXTENSION_readsInRange.erase(ed->m_EXTENSION_readsOutOfRange[i]);
				}
				ed->m_EXTENSION_readsOutOfRange.clear();
				ed->m_EXTENSION_singleEndResolution=true;

				// show on-screen dump debug
				#ifdef SHOW_CHOICE
				if(ed->m_enumerateChoices_outgoingEdges.size()>1){
					cout<<endl;
					cout<<"*****************************************"<<endl;
					cout<<"CurrentVertex="<<idToWord((*currentVertex),wordSize)<<" @"<<ed->m_EXTENSION_extension.size()<<endl;
					cout<<"Coverage="<<ed->m_currentCoverage<<endl;
					cout<<" # ReadsInRange: "<<ed->m_EXTENSION_readsInRange.size()<<endl;
					cout<<ed->m_enumerateChoices_outgoingEdges.size()<<" arcs: ";
					for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
						string vertex=idToWord(ed->m_enumerateChoices_outgoingEdges[i],wordSize);
						char letter=vertex[wordSize-1];
						int index=i;
						int coverage=ed->m_EXTENSION_coverages[i];
						int singleEnds=ed->m_EXTENSION_readPositionsForVertices[i].size();
						int pairedEnds=ed->m_EXTENSION_pairedReadPositionsForVertices[i].size();
						cout<<" "<<index<<"/"<<letter<<"/"<<coverage<<"/"<<singleEnds<<"/"<<pairedEnds;
					}
					cout<<endl;
					for(int i=0;i<(int)ed->m_enumerateChoices_outgoingEdges.size();i++){
						string vertex=idToWord(ed->m_enumerateChoices_outgoingEdges[i],wordSize);
						cout<<endl;
						cout<<"Choice #"<<i+1<<endl;
						cout<<"Vertex: "<<vertex<<endl;
						cout<<"Coverage="<<ed->m_EXTENSION_coverages[i]<<endl;
						cout<<"New letter: "<<vertex[wordSize-1]<<endl;
						cout<<"Single-end reads: ("<<ed->m_EXTENSION_readPositionsForVertices[i].size()<<")"<<endl;
						for(int j=0;j<(int)ed->m_EXTENSION_readPositionsForVertices[i].size();j++){
							if(j!=0)
								cout<<" ";
							cout<<ed->m_EXTENSION_readPositionsForVertices[i][j];
						}
						cout<<endl;
						cout<<"Paired-end reads: ("<<ed->m_EXTENSION_pairedReadPositionsForVertices[i].size()<<")"<<endl;
						for(int j=0;j<(int)ed->m_EXTENSION_pairedReadPositionsForVertices[i].size();j++){
							if(j!=0)
								cout<<" ";
							cout<<ed->m_EXTENSION_pairedReadPositionsForVertices[i][j];
						}
						cout<<endl;
					}
				}
				#endif
				
				// watchdog for REPEATs -- the main source of misassemblies!
				RepeatedVertexWatchdog watchdog;
				bool approval=watchdog.getApproval(ed,wordSize,minimumCoverage,(maxCoverage),
		(*currentVertex));
				if(!approval){
					ed->m_doChoice_tips_Detected=false;
					dfsData->m_doChoice_tips_Initiated=false;
					return;
				}

				// select chooser algorithm here.
				#define USE_OPEN_ASSEMBLER_CHOOSER
				//#define USE_TRON_CHOOSER 

				#ifdef USE_OPEN_ASSEMBLER_CHOOSER
				int choice=(*oa).choose(ed,&(*chooser),minimumCoverage,(maxCoverage),cd);
				#endif
				#ifdef USE_TRON_CHOOSER
				int choice=m_tc.choose(ed,&(*chooser),minimumCoverage,(maxCoverage),cd);
				#endif
				if(choice!=IMPOSSIBLE_CHOICE){
					(*currentVertex)=ed->m_enumerateChoices_outgoingEdges[choice];
					ed->m_EXTENSION_choose=true;
					ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
					ed->m_EXTENSION_directVertexDone=false;
					ed->m_EXTENSION_VertexAssembled_requested=false;
					return;
				}

				ed->m_doChoice_tips_Detected=false;
				dfsData->m_doChoice_tips_Initiated=false;
			}
			return;
		}else if(!ed->m_doChoice_tips_Detected and ed->m_EXTENSION_readsInRange.size()>0){
 			//for each entries in ed->m_enumerateChoices_outgoingEdges, do a dfs of max depth 40.
			//if the reached depth is 40, it is not a tip, otherwise, it is.
			int maxDepth=MAX_DEPTH;
			if(!dfsData->m_doChoice_tips_Initiated){
				dfsData->m_doChoice_tips_i=0;
				dfsData->m_doChoice_tips_newEdges.clear();
				dfsData->m_doChoice_tips_dfs_initiated=false;
				dfsData->m_doChoice_tips_dfs_done=false;
				dfsData->m_doChoice_tips_Initiated=true;
				bubbleData->m_BUBBLE_visitedVertices.clear();
				bubbleData->m_visitedVertices.clear();
				bubbleData->m_BUBBLE_visitedVerticesDepths.clear();
				bubbleData->m_coverages.clear();
			}

			if(dfsData->m_doChoice_tips_i<(int)ed->m_enumerateChoices_outgoingEdges.size()){
				if(!dfsData->m_doChoice_tips_dfs_done){
					depthFirstSearch((*currentVertex),ed->m_enumerateChoices_outgoingEdges[dfsData->m_doChoice_tips_i],maxDepth,dfsData,edgesRequested,vertexCoverageRequested,vertexCoverageReceived,outboxAllocator,
size,theRank,outbox,receivedVertexCoverage,receivedOutgoingEdges,minimumCoverage,edgesReceived);
				}else{
					#ifdef SHOW_CHOICE
					cout<<"Choice #"<<dfsData->m_doChoice_tips_i+1<<" : visited "<<dfsData->m_depthFirstSearchVisitedVertices.size()<<", max depth is "<<dfsData->m_depthFirstSearch_maxDepth<<endl;
					#endif
					// keep the edge if it is not a tip.
					if(dfsData->m_depthFirstSearch_maxDepth>=TIP_LIMIT){

						// just don't try that strange graph place for now.
						if(dfsData->m_depthFirstSearchVisitedVertices.size()==MAX_VERTICES_TO_VISIT){
							/*
							m_doChoice_tips_Detected=true;
							bubbleData->m_doChoice_bubbles_Detected=true;
							return;
							*/
						}
						dfsData->m_doChoice_tips_newEdges.push_back(dfsData->m_doChoice_tips_i);
						bubbleData->m_visitedVertices.push_back(dfsData->m_depthFirstSearchVisitedVertices);
						// store visited vertices for bubble detection purposes.
						bubbleData->m_BUBBLE_visitedVertices.push_back(dfsData->m_depthFirstSearchVisitedVertices_vector);
						bubbleData->m_coverages.push_back(dfsData->m_coverages);
						bubbleData->m_BUBBLE_visitedVerticesDepths.push_back(dfsData->m_depthFirstSearchVisitedVertices_depths);
					}else{
						#ifdef SHOW_PROGRESS_DEBUG
						cout<<"We have a tip "<<dfsData->m_depthFirstSearch_maxDepth<<" LIMIT="<<TIP_LIMIT<<"."<<endl;
						#endif
					}
					dfsData->m_doChoice_tips_i++;
					dfsData->m_doChoice_tips_dfs_initiated=false;
					dfsData->m_doChoice_tips_dfs_done=false;
				}
			}else{
				#ifdef SHOW_PROGRESS
				#endif
				// we have a winner with tips investigation.
				if(dfsData->m_doChoice_tips_newEdges.size()==1 and ed->m_EXTENSION_readsInRange.size()>0 
		and ed->m_EXTENSION_readPositionsForVertices[dfsData->m_doChoice_tips_newEdges[0]].size()>0
){

					// tip watchdog!
					// the watchdog watches Ray to be sure he is up to the task!
					TipWatchdog watchdog;
					bool opinion=watchdog.getApproval(ed,dfsData,minimumCoverage,
						(*currentVertex),wordSize,bubbleData);
					if(!opinion){
						ed->m_doChoice_tips_Detected=true;
						bubbleData->m_doChoice_bubbles_Detected=false;
						bubbleData->m_doChoice_bubbles_Initiated=false;
						return;
					}

					(*currentVertex)=ed->m_enumerateChoices_outgoingEdges[dfsData->m_doChoice_tips_newEdges[0]];
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
		}else if(!bubbleData->m_doChoice_bubbles_Detected and ed->m_EXTENSION_readsInRange.size()>0){
			BubbleTool tool;
			bool isGenuineBubble=tool.isGenuineBubble((*currentVertex),&bubbleData->m_BUBBLE_visitedVertices);

			// support indels of 1 as well as mismatch polymorphisms.
			if(isGenuineBubble){
				cout<<"Forcing next choice "<<endl;
				(*currentVertex)=ed->m_enumerateChoices_outgoingEdges[dfsData->m_doChoice_tips_newEdges[0]];
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
		if(!ed->m_EXTENSION_complementedSeed){
			#ifdef SHOW_PROGRESS_DEBUG
			cout<<"Rank "<<theRank<<": Switching to reverse complement."<<endl;
			#endif
			ed->m_EXTENSION_complementedSeed=true;
			vector<VERTEX_TYPE> complementedSeed;
			for(int i=ed->m_EXTENSION_extension.size()-1;i>=0;i--){
				complementedSeed.push_back(complementVertex(ed->m_EXTENSION_extension[i],wordSize,(*colorSpaceMode)));
			}
			ed->m_EXTENSION_currentPosition=0;
			ed->m_EXTENSION_currentSeed=complementedSeed;
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
			(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
			ed->m_EXTENSION_extension.clear();
			ed->m_EXTENSION_usedReads.clear();
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_VertexAssembled_requested=false;
			ed->m_EXTENSION_reads_startingPositionOnContig.clear();
			ed->m_EXTENSION_readsInRange.clear();
		}else{
			if(ed->m_EXTENSION_extension.size()>=100){
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<theRank<<" stores an extension, "<<ed->m_EXTENSION_extension.size()<<" vertices."<<endl;
				#endif
				ed->m_EXTENSION_contigs.push_back(ed->m_EXTENSION_extension);

				int id=ed->m_EXTENSION_currentSeedIndex*MAX_NUMBER_OF_MPI_PROCESSES+theRank;
				ed->m_EXTENSION_identifiers.push_back(id);
			}
			ed->m_EXTENSION_currentSeedIndex++;
			ed->m_EXTENSION_currentPosition=0;
			if(ed->m_EXTENSION_currentSeedIndex<(int)(*seeds).size()){
				ed->m_EXTENSION_currentSeed=(*seeds)[ed->m_EXTENSION_currentSeedIndex];
				(*currentVertex)=ed->m_EXTENSION_currentSeed[ed->m_EXTENSION_currentPosition];
			}
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=false;
			ed->m_EXTENSION_extension.clear();
			ed->m_EXTENSION_reads_startingPositionOnContig.clear();
			ed->m_EXTENSION_readsInRange.clear();
			ed->m_EXTENSION_usedReads.clear();
			ed->m_EXTENSION_directVertexDone=false;
			ed->m_EXTENSION_complementedSeed=false;
			ed->m_EXTENSION_VertexAssembled_requested=false;
		}
	}
}

/*
 * do a depth first search with max depth of maxDepth;
 */
void SeedExtender::depthFirstSearch(VERTEX_TYPE root,VERTEX_TYPE a,int maxDepth,DepthFirstSearchData*dfsData,
	bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	MyAllocator*outboxAllocator,int size,int theRank,vector<Message>*outbox,
 int*receivedVertexCoverage,vector<VERTEX_TYPE>*receivedOutgoingEdges,
		int minimumCoverage,bool*edgesReceived){
	if(!dfsData->m_doChoice_tips_dfs_initiated){
		dfsData->m_depthFirstSearchVisitedVertices.clear();
		dfsData->m_depthFirstSearchVisitedVertices_vector.clear();
		dfsData->m_depthFirstSearchVisitedVertices_depths.clear();
		while(dfsData->m_depthFirstSearchVerticesToVisit.size()>0)
			dfsData->m_depthFirstSearchVerticesToVisit.pop();
		while(dfsData->m_depthFirstSearchDepths.size()>0)
			dfsData->m_depthFirstSearchDepths.pop();

		dfsData->m_depthFirstSearchVerticesToVisit.push(a);
		dfsData->m_depthFirstSearchVisitedVertices.insert(a);
		dfsData->m_depthFirstSearchDepths.push(0);
		dfsData->m_depthFirstSearch_maxDepth=0;
		dfsData->m_doChoice_tips_dfs_initiated=true;
		dfsData->m_doChoice_tips_dfs_done=false;
		dfsData->m_coverages.clear();
		(*edgesRequested)=false;
		(*vertexCoverageRequested)=false;
		#ifdef SHOW_MINI_GRAPH
		cout<<"<MiniGraph>"<<endl;
		cout<<idToWord(root,wordSize)<<" -> "<<idToWord(a,wordSize)<<endl;
		#endif
	}
	if(dfsData->m_depthFirstSearchVerticesToVisit.size()>0){
		VERTEX_TYPE vertexToVisit=dfsData->m_depthFirstSearchVerticesToVisit.top();
		if(!(*vertexCoverageRequested)){
			(*vertexCoverageRequested)=true;
			(*vertexCoverageReceived)=false;
			
			VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
			message[0]=vertexToVisit;
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],size),
		TAG_REQUEST_VERTEX_COVERAGE,theRank);
			(*outbox).push_back(aMessage);
		}else if((*vertexCoverageReceived)){
			if(!(*edgesRequested)){
				dfsData->m_coverages[vertexToVisit]=(*receivedVertexCoverage);
				if((*receivedVertexCoverage)>1){
					dfsData->m_depthFirstSearchVisitedVertices.insert(vertexToVisit);
				}else{
					// don't visit it.
					dfsData->m_depthFirstSearchVerticesToVisit.pop();
					dfsData->m_depthFirstSearchDepths.pop();
					(*edgesRequested)=false;
					(*vertexCoverageRequested)=false;
					return;
				}
				int theDepth=dfsData->m_depthFirstSearchDepths.top();
				if(dfsData->m_depthFirstSearchVisitedVertices.size()>=MAX_VERTICES_TO_VISIT){
					// quit this strange place.
	
					dfsData->m_doChoice_tips_dfs_done=true;
					#ifdef SHOW_TIP_LOST
					cout<<"Exiting, I am lost. "<<dfsData->m_depthFirstSearchVisitedVertices.size()<<""<<endl;
					#endif
					return;
				}
				// too far away.
				if(theDepth> dfsData->m_depthFirstSearch_maxDepth){
					dfsData->m_depthFirstSearch_maxDepth=theDepth;
				}
			
				// visit the vertex, and ask next edges.
				VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
				message[0]=vertexToVisit;
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(vertexToVisit,size),
	TAG_REQUEST_VERTEX_OUTGOING_EDGES,theRank);
			(*outbox).push_back(aMessage);
				(*edgesRequested)=true;
				(*edgesReceived)=false;
			}else if((*edgesReceived)){
				VERTEX_TYPE vertexToVisit=dfsData->m_depthFirstSearchVerticesToVisit.top();
				int theDepth=dfsData->m_depthFirstSearchDepths.top();
				#ifdef DEBUG
				assert(theDepth>=0);
				assert(theDepth<=maxDepth);
				#endif
				int newDepth=theDepth+1;

				dfsData->m_depthFirstSearchVerticesToVisit.pop();
				dfsData->m_depthFirstSearchDepths.pop();


				#ifdef SHOW_MINI_GRAPH
				if(dfsData->(*chooser)overages[vertexToVisit]>=minimumCoverage/2){
					string b=idToWord(vertexToVisit,wordSize);
					cout<<b<<" [label=\""<<b<<" "<<(*receivedVertexCoverage)<<"\" ]"<<endl;
				}
				#endif

				for(int i=0;i<(int)(*receivedOutgoingEdges).size();i++){
					VERTEX_TYPE nextVertex=(*receivedOutgoingEdges)[i];
					if(dfsData->m_depthFirstSearchVisitedVertices.count(nextVertex)>0)
						continue;
					if(newDepth>maxDepth)
						continue;
					dfsData->m_depthFirstSearchVerticesToVisit.push(nextVertex);
					dfsData->m_depthFirstSearchDepths.push(newDepth);
					if(dfsData->m_coverages[vertexToVisit]>=minimumCoverage/2){
						dfsData->m_depthFirstSearchVisitedVertices_vector.push_back(vertexToVisit);
						dfsData->m_depthFirstSearchVisitedVertices_vector.push_back(nextVertex);
						dfsData->m_depthFirstSearchVisitedVertices_depths.push_back(newDepth);

						#ifdef SHOW_MINI_GRAPH
						cout<<idToWord(vertexToVisit,wordSize)<<" -> "<<idToWord(nextVertex,wordSize)<<endl;
						#endif
					}
				}
				(*edgesRequested)=false;
				(*vertexCoverageRequested)=false;
			}
		}
	}else{
		dfsData->m_doChoice_tips_dfs_done=true;
		#ifdef SHOW_MINI_GRAPH
		cout<<"</MiniGraph>"<<endl;
		#endif
	}
}


void SeedExtender::checkIfCurrentVertexIsAssembled(ExtensionData*ed,vector<Message>*outbox,MyAllocator*outboxAllocator,
  int*outgoingEdgeIndex,int*last_value,u64*currentVertex,int theRank,bool*vertexCoverageRequested,int wordSize,
 bool*colorSpaceMode,int size,vector<vector<VERTEX_TYPE> >*seeds){
	if(!ed->m_EXTENSION_directVertexDone){
		if(!ed->m_EXTENSION_VertexAssembled_requested){
			if(ed->m_EXTENSION_currentSeedIndex%10==0 and ed->m_EXTENSION_currentPosition==0 and (*last_value)!=ed->m_EXTENSION_currentSeedIndex){
				(*last_value)=ed->m_EXTENSION_currentSeedIndex;
				#ifdef SHOW_PROGRESS
				cout<<"Rank "<<theRank<<" is extending its seeds. "<<ed->m_EXTENSION_currentSeedIndex+1<<"/"<<(*seeds).size()<<endl;
				#endif
			}
			ed->m_EXTENSION_VertexAssembled_requested=true;
			VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
			message[0]=(VERTEX_TYPE)(*currentVertex);
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank((*currentVertex),size),TAG_ASK_IS_ASSEMBLED,theRank);
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
			}else{
			}
		}else{
		}
	}else if(!ed->m_EXTENSION_reverseVertexDone){
		if(!ed->m_EXTENSION_VertexAssembled_requested){
			ed->m_EXTENSION_VertexAssembled_requested=true;
			VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
			message[0]=(VERTEX_TYPE)complementVertex((*currentVertex),wordSize,(*colorSpaceMode));
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],size),
	TAG_ASK_IS_ASSEMBLED,theRank);
			(*outbox).push_back(aMessage);
			ed->m_EXTENSION_VertexAssembled_received=false;
		}else if(ed->m_EXTENSION_VertexAssembled_received){
			ed->m_EXTENSION_checkedIfCurrentVertexIsAssembled=true;
			ed->m_EXTENSION_markedCurrentVertexAsAssembled=false;
			ed->m_EXTENSION_directVertexDone=false;
		}else{
		}
	}
}

void SeedExtender::markCurrentVertexAsAssembled(u64*currentVertex,MyAllocator*outboxAllocator,int*outgoingEdgeIndex, 
vector<Message>*outbox,int size,int theRank,ExtensionData*ed,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,bool*edgesRequested,
vector<VERTEX_TYPE>*receivedOutgoingEdges,Chooser*chooser,ChooserData*cd,
BubbleData*bubbleData,int minimumCoverage,OpenAssemblerChooser*oa,bool*colorSpaceMode,int wordSize
){
	if(!ed->m_EXTENSION_directVertexDone){
		if(!(*vertexCoverageRequested)){
			(*vertexCoverageRequested)=true;
			(*vertexCoverageReceived)=false;
			
			VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
			message[0]=(*currentVertex);
			Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank(message[0],size),
				TAG_REQUEST_VERTEX_COVERAGE,theRank);
			(*outbox).push_back(aMessage);
		}else if((*vertexCoverageReceived)){
			if(!ed->m_EXTENSION_VertexMarkAssembled_requested){
				// watch out for repeats!!!
				if((*receivedVertexCoverage)==(*maxCoverage)){
					(*(repeatedLength))++;
				}else{
					(*(repeatedLength))=0;
				}

				ed->m_EXTENSION_extension.push_back((*currentVertex));
				ed->m_currentCoverage=(*receivedVertexCoverage);
				// save wave progress.
	
				int waveId=ed->m_EXTENSION_currentSeedIndex*MAX_NUMBER_OF_MPI_PROCESSES+theRank;
				int progression=ed->m_EXTENSION_extension.size()-1;
			

				ed->m_EXTENSION_VertexMarkAssembled_requested=true;
				VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(3*sizeof(VERTEX_TYPE));
				message[0]=(VERTEX_TYPE)(*currentVertex);
				message[1]=waveId;
				message[2]=progression;
				Message aMessage(message,3,MPI_UNSIGNED_LONG_LONG,vertexRank((*currentVertex),size),TAG_SAVE_WAVE_PROGRESSION,theRank);
				(*outbox).push_back(aMessage);
				ed->m_EXTENSION_reverseVertexDone=true;
				ed->m_EXTENSION_reads_requested=false;
				
				// we don't really need these reads
				if(ed->m_EXTENSION_complementedSeed and 
				ed->m_EXTENSION_currentPosition<(int)ed->m_EXTENSION_currentSeed.size()-1000){
					#ifdef SHOW_PROGRESS
					#endif
					ed->m_EXTENSION_reads_requested=true;
					ed->m_EXTENSION_reads_received=true;
					ed->m_EXTENSION_receivedReads.clear();
				}
			// get the reads starting at that position.
			}else if(!ed->m_EXTENSION_reads_requested){
				ed->m_EXTENSION_reads_requested=true;
				ed->m_EXTENSION_reads_received=false;
				VERTEX_TYPE*message=(VERTEX_TYPE*)(*outboxAllocator).allocate(1*sizeof(VERTEX_TYPE));
				message[0]=(VERTEX_TYPE)(*currentVertex);
				Message aMessage(message,1,MPI_UNSIGNED_LONG_LONG,vertexRank((*currentVertex),size),TAG_REQUEST_READS,theRank);
				(*outbox).push_back(aMessage);
			}else if(ed->m_EXTENSION_reads_received){
				for(int i=0;i<(int)ed->m_EXTENSION_receivedReads.size();i++){
					int uniqueId=ed->m_EXTENSION_receivedReads[i].getUniqueId();
					// check that the complete sequence of (*currentVertex) correspond to
					// the one of the start of the read on the good strand.
					// this is important when USE_DISTANT_SEGMENTS_GRAPH is set.

					if(ed->m_EXTENSION_usedReads.count(uniqueId)==0 ){
						// use all reads available.
						// avoid vertices with too much coverage.
						if((*(repeatedLength))<_REPEATED_LENGTH_ALARM_THRESHOLD){
							ed->m_EXTENSION_usedReads.insert(uniqueId);
							ed->m_EXTENSION_reads_startingPositionOnContig[uniqueId]=ed->m_EXTENSION_extension.size()-1;
							ed->m_EXTENSION_readsInRange.insert(ed->m_EXTENSION_receivedReads[i]);
							#ifdef DEBUG
							assert(ed->m_EXTENSION_readsInRange.count(ed->m_EXTENSION_receivedReads[i])>0);
							#endif
						}else{
							#ifdef SHOW_REPEATED_VERTEX
							cout<<"Repeated vertex: "<<idToWord((*currentVertex),wordSize)<<endl;
							#endif
							#ifdef DEBUG
							assert((*receivedVertexCoverage)>=(*maxCoverage));
							#endif
						}
					}
				}
				ed->m_EXTENSION_directVertexDone=true;
				ed->m_EXTENSION_VertexMarkAssembled_requested=false;
				ed->m_EXTENSION_enumerateChoices=false;
				(*edgesRequested)=false;
				ed->m_EXTENSION_markedCurrentVertexAsAssembled=true;
			}
		}
	}
}
