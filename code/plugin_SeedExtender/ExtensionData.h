/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#ifndef _ExtensionData
#define _ExtensionData

#include "ExtensionElement.h"

#include <code/plugin_SequencesIndexer/PairedRead.h>
#include <code/plugin_SequencesIndexer/ReadAnnotation.h>
#include <code/plugin_SequencesLoader/Read.h>
#include <code/plugin_Mock/Parameters.h>
#include <code/plugin_Mock/common_functions.h>
#include <code/plugin_SeedingData/GraphPath.h>

#include <RayPlatform/structures/SplayTree.h>
#include <RayPlatform/profiling/Profiler.h>

#include <map>
#include <set>
#include <vector>
using namespace std;

/*
 * Stores the information for a seed extension.
 *
 * This class is really just a helper for SeedExtender.
 * There are so many things to store to implement the heuristics
 * correctly.
 *
 * \author Sébastien Boisvert
 */
class ExtensionData{
	SplayTree<ReadHandle,ExtensionElement>*m_database;
	int m_numberOfBins;
	Parameters*m_parameters;
	MyAllocator m_allocator;

	void createStructures();
	void destroyStructures(Profiler*m_profiler);

public:

	int m_readType;
	vector<CoverageDepth> m_EXTENSION_coverages;
	GraphPath m_EXTENSION_extension;
	vector<CoverageDepth> m_extensionCoverageValues;
	vector<int> m_repeatedValues;

	// EXTENSION MODE
	vector<Kmer> m_enumerateChoices_outgoingEdges;
	bool m_doChoice_tips_Detected;
	PairedRead m_EXTENSION_pairedRead;
	bool m_EXTENSION_pairedSequenceRequested;
	bool m_EXTENSION_hasPairedReadAnswer;
	bool m_EXTENSION_pairedSequenceReceived;
	int m_EXTENSION_edgeIterator;
	bool m_EXTENSION_hasPairedReadRequested;
	bool m_EXTENSION_hasPairedReadReceived;
	vector<PathHandle> m_EXTENSION_identifiers;

	/**
	*	bidirectional flow algorithm using Ray's many heuristics
	*/
	/** the number of vertices flowed previously */
	int m_previouslyFlowedVertices;

	/** the current flow number */
	int m_flowNumber;

	/** TODO could be a pointer to the original thing... */
	GraphPath m_EXTENSION_currentSeed;

	int m_EXTENSION_numberOfRanksDone;
	vector<GraphPath> m_EXTENSION_contigs;
	bool m_EXTENSION_checkedIfCurrentVertexIsAssembled;
	bool m_EXTENSION_VertexMarkAssembled_requested;
	bool m_EXTENSION_reverseComplement_requested;
	bool m_EXTENSION_vertexIsAssembledResult;
	bool m_EXTENSION_readLength_requested;
	bool m_EXTENSION_readLength_received;
	bool m_EXTENSION_readLength_done;
	bool m_EXTENSION_read_vertex_received;
	bool m_EXTENSION_read_vertex_requested;
	Kmer m_EXTENSION_receivedReadVertex;
	bool m_EXTENSION_currentRankIsDone;
	bool m_EXTENSION_currentRankIsSet;
	bool m_EXTENSION_currentRankIsStarted;
	int m_EXTENSION_rank;
	bool m_EXTENSION_initiated;
	int m_EXTENSION_currentSeedIndex;
	bool m_EXTENSION_VertexAssembled_received;
	int m_EXTENSION_currentPosition;
	bool m_EXTENSION_VertexMarkAssembled_received;
	bool m_EXTENSION_markedCurrentVertexAsAssembled;
	bool m_EXTENSION_enumerateChoices;
	bool m_EXTENSION_choose;
	bool m_EXTENSION_directVertexDone;
	bool m_EXTENSION_VertexAssembled_requested;
	bool m_EXTENSION_receivedAssembled;
	bool m_EXTENSION_reverseComplement_received;
	vector<ReadAnnotation> m_EXTENSION_receivedReads;
	bool m_EXTENSION_reads_requested;
	bool m_EXTENSION_reads_received;
	vector<ReadHandle> m_sequencesToFree;
	int m_EXTENSION_receivedLength;
	bool m_EXTENSION_reverseVertexDone;
	// reads used so far
	// reads to check (the ones "in range")
	bool m_EXTENSION_singleEndResolution;
	set<ReadHandle>::iterator m_EXTENSION_readIterator;
	map<Kmer,vector<int> > m_EXTENSION_readPositionsForVertices;
	map<Kmer,vector<int> > m_EXTENSION_pairedReadPositionsForVertices;
	map<Kmer,vector<int> > m_EXTENSION_pairedLibrariesForVertices;
	map<Kmer,vector<ReadHandle> > m_EXTENSION_pairedReadsForVertices;
	CoverageDepth m_currentCoverage;

	set<ReadHandle> m_EXTENSION_readsInRange;
	set<ReadHandle> m_pairedReadsWithoutMate;
	map<int,vector<ReadHandle> > m_expirations;

	void lazyDestructor();

	void resetStructures(Profiler*m_profiler);
	ExtensionElement*getUsedRead(ReadHandle a);
	ExtensionElement*addUsedRead(ReadHandle a);
	void removeSequence(ReadHandle a);
	void constructor(Parameters*parameters);

	void destructor();
	MyAllocator*getAllocator();
};

#endif
