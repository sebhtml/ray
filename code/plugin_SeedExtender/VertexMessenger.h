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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#ifndef _VertexMessenger
#define _VertexMessenger

#include <stdint.h>
#include <plugin_SequencesIndexer/ReadAnnotation.h>
#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <structures/StaticVector.h>
#include <memory/RingAllocator.h>
#include <application_core/Parameters.h>
#include <vector>
#include <set>
using namespace std;

/*
 * Get the markers for a Kmer and also send progression 
 * information to the appropriate MPI rank.
 * \author Sébastien Boisvert
 */
class VertexMessenger{
	
	MessageTag RAY_MPI_TAG_VERTEX_INFO;
	MessageTag RAY_MPI_TAG_VERTEX_INFO_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_READS;
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST;
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_READS_REPLY;

	CoverageDepth m_peakCoverage;

	set<ReadHandle>::iterator m_mateIterator;
	Parameters*m_parameters;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	uint8_t m_edges;
	CoverageDepth m_coverageValue;
	vector<ReadAnnotation> m_annotations;
	bool m_isDone;
	Kmer m_vertex;
	PathHandle m_waveId;
	int m_wavePosition;
	set<ReadHandle>*m_matesToMeet;
	bool m_receivedBasicInfo;
	bool m_requestedBasicInfo;
	int m_numberOfAnnotations;
	bool m_getReads;
	void*m_pointer;
	bool m_requestedReads;
	Rank m_destination;
	bool m_receivedReads;

	void getReadsForUniqueVertex();
	void getReadsForRepeatedVertex();
public:
	void constructor(Kmer vertex,PathHandle wave,int pos,
		set<ReadHandle>*matesToMeet,StaticVector*inbox,StaticVector*outbox,
	RingAllocator*outboxAllocator,Parameters*parameters,bool getReads,CoverageDepth peakCoverage,
	MessageTag RAY_MPI_TAG_VERTEX_INFO,
	MessageTag RAY_MPI_TAG_VERTEX_INFO_REPLY,
	MessageTag RAY_MPI_TAG_VERTEX_READS,
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST,
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY,
	MessageTag RAY_MPI_TAG_VERTEX_READS_REPLY
);

	bool isDone();
	void work();
	CoverageDepth getCoverageValue();
	uint8_t getEdges();
	vector<ReadAnnotation>getReadAnnotations();
};

#endif
