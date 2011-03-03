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

#ifndef _ReadFetcher
#define _ReadFetcher

#include <vector>
#include <ReadAnnotation.h>
#include <StaticVector.h>
#include <Parameters.h>
#include <stdint.h>
#include <RingAllocator.h>
#include <VirtualCommunicator.h>
using namespace std;

class ReadFetcher{
	uint64_t m_workerId;
	VirtualCommunicator*m_virtualCommunicator;
	Parameters*m_parameters;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	vector<ReadAnnotation> m_reads;
	uint64_t m_vertex;
	bool m_readsRequested;
	void*m_pointer;
	bool m_done;
public:
	void constructor(uint64_t vertex,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,VirtualCommunicator*vc,uint64_t workerId);
	bool isDone();
	void work();
	vector<ReadAnnotation>*getResult();
};

#endif
