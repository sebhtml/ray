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

#ifndef _Scaffolder
#define _Scaffolder

#include <Parameters.h>
#include <StaticVector.h>
#include <vector>
#include <RingAllocator.h>
#include <constants.h>
using namespace std;

class Scaffolder{
	bool m_coverageRequested;
	bool m_coverageReceived;
	int m_receivedCoverage;

	bool m_reverseDone;
	bool m_forwardDone;
	int m_contigId;
	int m_positionOnContig;
	vector<vector<VERTEX_TYPE> > m_contigs;
	vector<uint64_t> m_contigNames;

	Parameters*m_parameters;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	bool m_initialised;
	int*m_slave_mode;
	bool m_ready;
public:
	int m_numberOfRanksFinished;

	void constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
		int*slaveMode);
	void run();
	void addContig(uint64_t name,vector<uint64_t>*vertices);
};

#endif

