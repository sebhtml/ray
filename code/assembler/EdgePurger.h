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

#ifndef _EdgePurger_H
#define _EdgePurger_H

#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <communication/VirtualCommunicator.h>
#include <map>
#include <graph/GridTable.h>
#include <graph/GridTableIterator.h>
#include <set>
#include <stdint.h>
#include <assembler/EdgePurgerWorker.h>
using namespace std;

/**
 * VerticesExtractor.cpp adds k-mers and ingoing and outgoing edges
 * for everything found in reads. However, GridTable.cpp only considers those with
 * enough coverage in the KmerAcademy.cpp.
 * Thus, there will be some edges that point to nothing in the GridTable.cpp.
 * EdgePurger.cpp remove these edges.
 */
class EdgePurger{
	bool m_initiatedIterator;
	uint64_t m_SEEDING_i;
	set<uint64_t> m_activeWorkers;
	set<uint64_t>::iterator m_activeWorkerIterator;
	int m_completedJobs;
	int m_maximumAliveWorkers;
	int m_maximumWorkers;
	GridTable*m_subgraph;
	GridTableIterator m_graphIterator;
	bool m_communicatorWasTriggered;
	vector<uint64_t> m_workersDone;
	vector<uint64_t> m_waitingWorkers;
	vector<uint64_t> m_activeWorkersToRestore;

	map<uint64_t,EdgePurgerWorker> m_aliveWorkers;

	int m_masterCountFinished;
	Parameters*m_parameters;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	VirtualCommunicator*m_virtualCommunicator;
	int*m_slaveMode;
	int*m_masterMode;
	bool m_done;

	void updateStates();
public:
	void constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
		int*slaveMode,int*masterMode,VirtualCommunicator*vc,GridTable*edgePurger);

	void work();
};

#endif
