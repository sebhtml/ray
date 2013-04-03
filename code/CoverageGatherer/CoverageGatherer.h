/*
 	Ray
    Copyright (C) 2011, 2012 Sébastien Boisvert

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

#ifndef _CoverageGatherer_H
#define _CoverageGatherer_H

#include <code/Mock/Parameters.h>
#include <code/VerticesExtractor/GridTable.h>

#include <RayPlatform/memory/RingAllocator.h>
#include <RayPlatform/structures/StaticVector.h>
#include <RayPlatform/core/ComputeCore.h>

#include <stdint.h>
#include <map>
using namespace std;

__DeclarePlugin(CoverageGatherer);

__DeclareSlaveModeAdapter(CoverageGatherer,RAY_SLAVE_MODE_SEND_DISTRIBUTION);

/**
 * \author Sébastien Boisvert
 */
class CoverageGatherer : public CorePlugin{

	__AddAdapter(CoverageGatherer,RAY_SLAVE_MODE_SEND_DISTRIBUTION);

	MessageTag RAY_MPI_TAG_COVERAGE_DATA_REPLY;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_DIRECTION_REPLY;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_MARK_REPLY;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY;

	MessageTag RAY_MPI_TAG_COVERAGE_DATA;
	MessageTag RAY_MPI_TAG_COVERAGE_END;

	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;
	SlaveMode RAY_SLAVE_MODE_SEND_DISTRIBUTION;


	map<CoverageDepth,LargeCount> m_distributionOfCoverage;
	map<CoverageDepth,LargeCount>::iterator m_coverageIterator;
	bool m_waiting;
	Parameters*m_parameters;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	int*m_slaveMode;
	GridTable*m_subgraph;
	RingAllocator*m_outboxAllocator;

	void writeHeader(FILE*kmerFile);

public:
	void constructor(Parameters*parameters,StaticVector*inbox,StaticVector*outbox,int*slaveMode,
		GridTable*subgraph,RingAllocator*outboxAllocator);
	void call_RAY_SLAVE_MODE_SEND_DISTRIBUTION();
	void writeKmers();

	void registerPlugin(ComputeCore*core);

	void resolveSymbols(ComputeCore*core);
};

#endif
