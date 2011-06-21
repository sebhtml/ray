/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#ifndef _CoverageGatherer_H
#define _CoverageGatherer_H

#include <core/Parameters.h>
#include <structures/StaticVector.h>
#include <graph/GridTable.h>
#include <stdint.h>
#include <map>
#include <memory/RingAllocator.h>
using namespace std;

class CoverageGatherer{
	map<int,uint64_t> m_distributionOfCoverage;
	map<int,uint64_t>::iterator m_coverageIterator;
	bool m_waiting;
	Parameters*m_parameters;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	int*m_slaveMode;
	GridTable*m_subgraph;
	RingAllocator*m_outboxAllocator;
public:
	void constructor(Parameters*parameters,StaticVector*inbox,StaticVector*outbox,int*slaveMode,
		GridTable*subgraph,RingAllocator*outboxAllocator);
	void work();
};

#endif
