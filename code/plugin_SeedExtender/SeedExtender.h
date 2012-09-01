/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _SeedExtender
#define _SeedExtender

class FusionData;
class DepthFirstSearchData;

#include <vector>
#include <fstream>

#include <application_core/common_functions.h>
#include <communication/Message.h>
#include <profiling/Profiler.h>
#include <plugin_SeedExtender/VertexMessenger.h>
#include <plugin_SeedExtender/ExtensionData.h>
#include <application_core/Parameters.h>
#include <plugin_SeedingData/AssemblySeed.h>
#include <memory/RingAllocator.h>
#include <memory/MyAllocator.h>
#include <profiling/Derivative.h>
#include <plugin_SeedExtender/ReadFetcher.h>
#include <plugin_FusionData/FusionData.h>
#include <plugin_SeedExtender/BubbleData.h>
#include <plugin_SeedExtender/DepthFirstSearchData.h>
#include <plugin_SeedExtender/BubbleTool.h>
#include <plugin_SeedExtender/OpenAssemblerChooser.h>
#include <plugin_VerticesExtractor/GridTable.h>
#include <plugin_SeedingData/SeedingData.h>
#include <handlers/SlaveModeHandler.h>
#include <core/ComputeCore.h>

using namespace std;

/*
 * Performs the extension of seeds.
 * \author Sébastien Boisvert
 */
class SeedExtender: public CorePlugin  {

/** hot skipping technology (TM) **/

	int m_redundantProcessingVirtualMachineCycles;
	bool m_hotSkippingMode;
	int m_hotSkippingThreshold;
	bool m_theProcessIsRedundantByAGreaterAndMightyRank;

	void configureTheBeautifulHotSkippingTechnology();


	Rank m_rank;

	MessageTag RAY_MPI_TAG_CONTIG_INFO_REPLY;
	MessageTag RAY_MPI_TAG_GET_CONTIG_CHUNK_REPLY;

	MessageTag RAY_MPI_TAG_ASK_IS_ASSEMBLED;
	MessageTag RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY;
	MessageTag RAY_MPI_TAG_EXTENSION_IS_DONE;
	MessageTag RAY_MPI_TAG_REQUEST_READ_SEQUENCE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_EDGES;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES;
	MessageTag RAY_MPI_TAG_VERTEX_INFO;
	MessageTag RAY_MPI_TAG_VERTEX_INFO_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_READS;
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST;
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_READS_REPLY;
	MessageTag RAY_MPI_TAG_ADD_GRAPH_PATH;

	SlaveMode RAY_SLAVE_MODE_EXTENSION;
	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;



// all these parameters are not attributes.
	vector<AssemblySeed>*m_seeds;
  	Kmer*m_currentVertex;
	FusionData*m_fusionData;
	RingAllocator*m_outboxAllocator;
	bool*m_edgesRequested;
	int*m_outgoingEdgeIndex;
	int m_last_value;
	bool*m_vertexCoverageRequested;
	bool*m_vertexCoverageReceived;
	int*m_receivedVertexCoverage;
	vector<Kmer>*m_receivedOutgoingEdges;
	Chooser*m_chooser;
	BubbleData*m_bubbleData;
	OpenAssemblerChooser*m_oa;
	bool*m_edgesReceived;
	int*m_mode;

	int m_currentPeakCoverage;

	Derivative m_derivative;

	bool m_checkedCheckpoint;

	LargeCount m_sumOfCoveragesInSeed;
	map<int,int> m_localCoverageDistribution;

	Profiler*m_profiler;

	SeedingData*m_seedingData;

	/* for sliced computation */
	AssemblySeed m_complementedSeed;

	void printSeed();

	int m_slicedProgression;
	bool m_slicedComputationStarted;

	map<int,map<int,uint64_t> > m_pairedScores;

	int m_extended;
	bool m_hasPairedSequences;
	bool m_pickedInformation;
	SplayTree<ReadHandle,Read>m_cacheForRepeatedReads;
	SplayTree<uint64_t,ReadAnnotation*> m_cacheForListOfReads;
	MyAllocator m_cacheAllocator;

	vector<int> m_flowedVertices;

	StaticVector*m_inbox;
	StaticVector*m_outbox;

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

	set<ReadHandle> m_matesToMeet;
	bool m_messengerInitiated;
	VertexMessenger m_vertexMessenger;

	set<ReadHandle> m_eliminatedSeeds;
	map<int,vector<ReadHandle> >m_expiredReads;

	void inspect(ExtensionData*ed,Kmer*currentVertex);

	void removeUnfitLibraries();

	void setFreeUnmatedPairedReads();

	void showReadsInRange();

	void printExtensionStatus(Kmer*currentVertex);

	void printTree(Kmer root,
map<Kmer,set<Kmer> >*arcs,map<Kmer,int>*coverages,int depth,set<Kmer>*visited);

	void readCheckpoint(FusionData*fusionData);
	void writeCheckpoint();

	void showSequences();

	void processExpiredReads();
	int chooseWithSeed();

	void initializeExtensions(vector<AssemblySeed>*seeds);
	void finalizeExtensions(vector<AssemblySeed>*seeds,FusionData*fusionData);
	void checkedCurrentVertex();
	void skipSeed(vector<AssemblySeed>*seeds);

/** store the current extension and fetch the next one **/
	void storeExtensionAndGetNextOne(ExtensionData*ed,int theRank,vector<AssemblySeed>*seeds,Kmer*currentVertex,
		BubbleData*bubbleData);

/** given the current vertex, enumerate the choices **/
	void enumerateChoices(bool*edgesRequested,ExtensionData*ed,bool*edgesReceived,RingAllocator*outboxAllocator,
		int*outgoingEdgeIndex,StaticVector*outbox,
Kmer*currentVertex,int theRank,bool*vertexCoverageRequested,vector<Kmer>*receivedOutgoingEdges,
bool*vertexCoverageReceived,int size,int*receivedVertexCoverage,Chooser*chooser,
int wordSize);

/** check if the current vertex is already assembled **/
	void checkIfCurrentVertexIsAssembled(ExtensionData*ed,StaticVector*outbox,RingAllocator*outboxAllocator,
	 int*outgoingEdgeIndex,int*last_value,Kmer*currentVertex,int theRank,bool*vertexCoverageRequested,
	int wordSize,int size,vector<AssemblySeed>*seeds);

/** mark the current vertex as assembled **/
	void markCurrentVertexAsAssembled(Kmer *currentVertex,RingAllocator*outboxAllocator,int*outgoingEdgeIndex,
 StaticVector*outbox,int size,int theRank,ExtensionData*ed,bool*vertexCoverageRequested,
		bool*vertexCoverageReceived,int*receivedVertexCoverage,
	bool*edgesRequested,
vector<Kmer>*receivedOutgoingEdges,Chooser*chooser,
BubbleData*bubbleData,int minimumCoverage,OpenAssemblerChooser*oa,int wordSize,vector<AssemblySeed>*seeds);

/** choose where to go next **/
	void doChoice(RingAllocator*outboxAllocator,int*outgoingEdgeIndex,StaticVector*outbox,Kmer*currentVertex,
BubbleData*bubbleData,int theRank,int wordSize,
ExtensionData*ed,int minimumCoverage,OpenAssemblerChooser*oa,Chooser*chooser,
	vector<AssemblySeed>*seeds,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,int size,
int*receivedVertexCoverage,bool*edgesReceived,vector<Kmer>*receivedOutgoingEdges);


	// bug hunting
	uint8_t m_compactEdges;


	// storage for parallel paths
	ofstream m_pathFile;
	ostringstream m_pathFileBuffer;

	SwitchMan*m_switchMan;

public:
	bool m_sequenceReceived;
	bool m_sequenceRequested;
	char m_receivedString[RAY_MAXIMUM_READ_LENGTH];
	int m_sequenceIndexToCache;

	SeedExtender();

	vector<Direction>*getDirections();

	set<PathHandle>*getEliminatedSeeds();

	void constructor(Parameters*parameters,MyAllocator*m_directionsAllocator,ExtensionData*ed,GridTable*table,StaticVector*inbox,
	Profiler*profiler,StaticVector*outbox,SeedingData*seedingData,int*mode,
	bool*vertexCoverageRequested,bool*vertexCoverageReceived,RingAllocator*outboxAllocator,
		FusionData*fusionData,vector<AssemblySeed>*seeds,BubbleData*bubbleData,
		bool*edgesRequested,bool*edgesReceived,int*outgoingEdgeIndex,Kmer*currentVertex,
	int*receivedVertexCoverage,vector<Kmer>*receivedOutgoingEdges,Chooser*chooser,
		OpenAssemblerChooser*oa);

	void closePathFile();

	void call_RAY_SLAVE_MODE_EXTENSION();

	void call_RAY_MPI_TAG_ADD_GRAPH_PATH(Message*message);
	void call_RAY_MPI_TAG_ASK_IS_ASSEMBLED(Message*message);
	void call_RAY_MPI_TAG_ASK_IS_ASSEMBLED_REPLY(Message*message);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};


#endif

