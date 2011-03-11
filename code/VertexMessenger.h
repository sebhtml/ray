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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#ifndef _VertexMessenger
#define _VertexMessenger

#include <stdint.h>
#include <ReadAnnotation.h>
#include <StaticVector.h>
#include <RingAllocator.h>
#include <Parameters.h>
#include <vector>
#include <set>
using namespace std;

class VertexMessenger{
	Parameters*m_parameters;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	uint8_t m_edges;
	uint16_t m_coverageValue;
	vector<ReadAnnotation> m_annotations;
	bool m_isDone;
	uint64_t m_vertex;
	uint64_t m_waveId;
	int m_wavePosition;
	set<uint64_t>*m_matesToMeet;
	bool m_receivedBasicInfo;
	bool m_requestedBasicInfo;
	int m_numberOfAnnotations;
	void*m_pointer;
	bool m_requestedReads;
	int m_destination;
	bool m_receivedReads;

	void getReadsForUniqueVertex();
public:
	void constructor(uint64_t vertex,uint64_t wave,int pos,set<uint64_t>*matesToMeet,StaticVector*inbox,StaticVector*outbox,
	RingAllocator*outboxAllocator,Parameters*parameters);
	bool isDone();
	void work();
	uint16_t getCoverageValue();
	uint8_t getEdges();
	vector<ReadAnnotation>getReadAnnotations();
};

#endif
