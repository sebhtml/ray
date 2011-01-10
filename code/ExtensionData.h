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

#include<vector>
using namespace std;
#include<PairedRead.h>
#include<set>
#include<map>
#include<ReadAnnotation.h>
#include<common_functions.h>

class ExtensionData{
public:
	// EXTENSION MODE
	vector<uint64_t> m_enumerateChoices_outgoingEdges;
	bool m_doChoice_tips_Detected;
	PairedRead m_EXTENSION_pairedRead;
	bool m_EXTENSION_pairedSequenceRequested;
	bool m_EXTENSION_hasPairedReadAnswer;
	bool m_EXTENSION_pairedSequenceReceived;
	int m_EXTENSION_edgeIterator;
	bool m_EXTENSION_hasPairedReadRequested;
	bool m_EXTENSION_hasPairedReadReceived;
	vector<int> m_EXTENSION_identifiers;
	vector<uint64_t> m_EXTENSION_extension;
	vector<int> m_EXTENSION_coverages;
	bool m_EXTENSION_complementedSeed;
	vector<uint64_t> m_EXTENSION_currentSeed;
	int m_EXTENSION_numberOfRanksDone;
	vector<vector<uint64_t> > m_EXTENSION_contigs;
	bool m_EXTENSION_checkedIfCurrentVertexIsAssembled;
	bool m_EXTENSION_VertexMarkAssembled_requested;
	bool m_EXTENSION_reverseComplement_requested;
	bool m_EXTENSION_vertexIsAssembledResult;
	set<ReadAnnotation,ReadAnnotationComparator>::iterator m_EXTENSION_readIterator;
	bool m_EXTENSION_readLength_requested;
	bool m_EXTENSION_readLength_received;
	bool m_EXTENSION_readLength_done;
	bool m_EXTENSION_read_vertex_received;
	bool m_EXTENSION_read_vertex_requested;
	uint64_t m_EXTENSION_receivedReadVertex;
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
	vector<ReadAnnotation> m_EXTENSION_readsOutOfRange;
	int m_EXTENSION_receivedLength;
	bool m_EXTENSION_reverseVertexDone;
	// reads used so far
	set<uint64_t> m_EXTENSION_usedReads;
	// reads to check (the ones "in range")
	set<ReadAnnotation,ReadAnnotationComparator> m_EXTENSION_readsInRange;
	bool m_EXTENSION_singleEndResolution;
	map<int,vector<int> > m_EXTENSION_readPositionsForVertices;
	map<int,vector<int> > m_EXTENSION_pairedReadPositionsForVertices;
	map<uint64_t,int> m_EXTENSION_reads_startingPositionOnContig;
	int m_currentCoverage;

};

#endif
