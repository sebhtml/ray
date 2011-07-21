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

#ifndef _ExtensionData
#define _ExtensionData

#include <structures/PairedRead.h>
#include <structures/Read.h>
#include <core/Parameters.h>
#include <assembler/ExtensionElement.h>
#include <structures/ReadAnnotation.h>
#include <core/common_functions.h>
#include <structures/SplayTree.h>
#include <map>
#include <set>
#include <vector>
using namespace std;

/*
 * Stores the information for a seed extension.
 */
class ExtensionData{
	SplayTree<uint64_t,ExtensionElement>*m_database;
	int m_numberOfBins;
	Parameters*m_parameters;
	MyAllocator m_allocator;

	void createStructures();
	void destroyStructures();

public:

	int m_readType;
	vector<int>*m_EXTENSION_coverages;
	vector<Kmer >*m_EXTENSION_extension;
	vector<int>*m_extensionCoverageValues;
	vector<int>*m_repeatedValues;
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
	vector<uint64_t> m_EXTENSION_identifiers;
	bool m_EXTENSION_complementedSeed;
	bool m_EXTENSION_complementedSeed2;
	vector<Kmer > m_EXTENSION_currentSeed;
	int m_EXTENSION_numberOfRanksDone;
	vector<vector<Kmer > > m_EXTENSION_contigs;
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
	bool m_mode_EXTENSION;
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
	vector<uint64_t> m_sequencesToFree;
	int m_EXTENSION_receivedLength;
	bool m_EXTENSION_reverseVertexDone;
	// reads used so far
	// reads to check (the ones "in range")
	bool m_EXTENSION_singleEndResolution;
	set<uint64_t>::iterator m_EXTENSION_readIterator;
	map<Kmer,vector<int> > m_EXTENSION_readPositionsForVertices;
	map<Kmer,vector<int> > m_EXTENSION_pairedReadPositionsForVertices;
	map<Kmer,vector<int> > m_EXTENSION_pairedLibrariesForVertices;
	map<Kmer,vector<uint64_t> > m_EXTENSION_pairedReadsForVertices;
	int m_currentCoverage;

	set<uint64_t>*m_EXTENSION_readsInRange;
	set<uint64_t>*m_pairedReadsWithoutMate;
	map<int,vector<uint64_t> >*m_expirations;

	void resetStructures();
	ExtensionElement*getUsedRead(uint64_t a);
	ExtensionElement*addUsedRead(uint64_t a);
	void removeSequence(uint64_t a);
	void constructor(Parameters*parameters);

	void destructor();
	MyAllocator*getAllocator();
};

#endif
