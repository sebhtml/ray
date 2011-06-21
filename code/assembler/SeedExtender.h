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

#ifndef _SeedExtender
#define _SeedExtender

class FusionData;
class DepthFirstSearchData;

#include <core/common_functions.h>
#include <communication/Message.h>
#include <vector>
#include <assembler/VertexMessenger.h>
#include <assembler/ExtensionData.h>
#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <memory/MyAllocator.h>
#include <assembler/ReadFetcher.h>
#include <memory/OnDiskAllocator.h>
#include <assembler/FusionData.h>
#include <assembler/BubbleData.h>
#include <assembler/DepthFirstSearchData.h>
#include <assembler/BubbleTool.h>
#include <assembler/OpenAssemblerChooser.h>
#include <graph/GridTable.h>

using namespace std;

/*
 * Performs the extension of seeds.
 */
class SeedExtender{
	int m_extended;
	bool m_hasPairedSequences;
	bool m_pickedInformation;
	SplayTree<uint64_t,Read>m_cacheForRepeatedReads;
	SplayTree<uint64_t,ReadAnnotation*> m_cacheForListOfReads;
	MyAllocator m_cacheAllocator;

	int m_repeatLength;
	StaticVector*m_inbox;

	DepthFirstSearchData*m_dfsData;
	bool m_removedUnfitLibraries;
	SplayTree<Kmer,int> m_cache;
	vector<Direction>m_receivedDirections;
	GridTable*m_subgraph;
	bool m_skippedASeed;
	Parameters*m_parameters;
	BubbleTool m_bubbleTool;
	ExtensionData*m_ed;
	MyAllocator*m_directionsAllocator;

	set<uint64_t> m_matesToMeet;
	bool m_messengerInitiated;
	VertexMessenger m_vertexMessenger;

	set<uint64_t> m_eliminatedSeeds;
	map<int,vector<uint64_t> >m_expiredReads;

	void inspect(ExtensionData*ed,Kmer*currentVertex);

	void removeUnfitLibraries();

	void setFreeUnmatedPairedReads();

	void showReadsInRange();

	void printExtensionStatus(Kmer*currentVertex);

	void printTree(Kmer root,
map<Kmer,set<Kmer> >*arcs,map<Kmer,int>*coverages,int depth,set<Kmer>*visited);
public:
	bool m_sequenceReceived;
	bool m_sequenceRequested;
	string m_receivedString;
	int m_sequenceIndexToCache;

	SeedExtender();

	void enumerateChoices(bool*edgesRequested,ExtensionData*ed,bool*edgesReceived,RingAllocator*outboxAllocator,
		int*outgoingEdgeIndex,StaticVector*outbox,
Kmer*currentVertex,int theRank,bool*vertexCoverageRequested,vector<Kmer>*receivedOutgoingEdges,
bool*vertexCoverageReceived,int size,int*receivedVertexCoverage,Chooser*chooser,
int wordSize);

	void checkIfCurrentVertexIsAssembled(ExtensionData*ed,StaticVector*outbox,RingAllocator*outboxAllocator,
	 int*outgoingEdgeIndex,int*last_value,Kmer*currentVertex,int theRank,bool*vertexCoverageRequested,
	int wordSize,int size,vector<vector<Kmer> >*seeds);

	void markCurrentVertexAsAssembled(Kmer *currentVertex,RingAllocator*outboxAllocator,int*outgoingEdgeIndex,
 StaticVector*outbox,int size,int theRank,ExtensionData*ed,bool*vertexCoverageRequested,
		bool*vertexCoverageReceived,int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,
	bool*edgesRequested,
vector<Kmer>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,int minimumCoverage,OpenAssemblerChooser*oa,int wordSize,vector<vector<Kmer> >*seeds);

	void extendSeeds(vector<vector<Kmer> >*seeds,ExtensionData*ed,int theRank,StaticVector*outbox,Kmer*currentVertex,
	FusionData*fusionData,RingAllocator*outboxAllocator,bool*edgesRequested,int*outgoingEdgeIndex,
int*last_value,bool*vertexCoverageRequested,int wordSize,int size,bool*vertexCoverageReceived,
int*receivedVertexCoverage,int*repeatedLength,int*maxCoverage,vector<Kmer>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,
int minimumCoverage,OpenAssemblerChooser*oa,bool*edgesReceived,int*m_mode);

	void doChoice(RingAllocator*outboxAllocator,int*outgoingEdgeIndex,StaticVector*outbox,Kmer*currentVertex,
BubbleData*bubbleData,int theRank,int wordSize,
ExtensionData*ed,int minimumCoverage,int maxCoverage,OpenAssemblerChooser*oa,Chooser*chooser,
	vector<vector<Kmer> >*seeds,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,int size,
int*receivedVertexCoverage,bool*edgesReceived,vector<Kmer>*receivedOutgoingEdges);

	vector<Direction>*getDirections();

	void storeExtensionAndGetNextOne(ExtensionData*ed,int theRank,vector<vector<Kmer> >*seeds,Kmer*currentVertex,
		BubbleData*bubbleData);

	set<uint64_t>*getEliminatedSeeds();

	void constructor(Parameters*parameters,MyAllocator*m_directionsAllocator,ExtensionData*ed,GridTable*table,StaticVector*inbox);
};

#endif

