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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _EdgePurger_H
#define _EdgePurger_H

#include <application_core/Parameters.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <communication/VirtualCommunicator.h>
#include <plugin_VerticesExtractor/GridTable.h>
#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <profiling/Profiler.h>
#include <profiling/Derivative.h>
#include <plugin_EdgePurger/EdgePurgerWorker.h>
#include <scheduling/TaskCreator.h>
#include <core/ComputeCore.h>

#include <map>
#include <set>
#include <stdint.h>
using namespace std;


/**
 * VerticesExtractor.cpp adds k-mers and ingoing and outgoing edges
 * for everything found in reads. However, GridTable.cpp only considers those with
 * enough coverage in the KmerAcademy.cpp.
 * Thus, there will be some edges that point to nothing in the GridTable.cpp.
 * EdgePurger.cpp remove these edges.
 * \author Sébastien Boisvert
 */
class EdgePurger : public TaskCreator, public CorePlugin {

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT_REPLY;

	MessageTag RAY_MPI_TAG_PURGE_NULL_EDGES_REPLY;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;

	MasterMode RAY_MASTER_MODE_WRITE_KMERS;
	SlaveMode RAY_SLAVE_MODE_PURGE_NULL_EDGES;



	Profiler*m_profiler;

	/** checkpointing */
	bool m_checkedCheckpoint;

	Derivative m_derivative;

	uint64_t m_SEEDING_i;
	GridTable*m_subgraph;
	GridTableIterator m_graphIterator;
	int m_masterCountFinished;

	Parameters*m_parameters;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	int*m_slaveMode;
	VirtualCommunicator*m_virtualCommunicator;
	MyAllocator m_workerAllocator;
	int*m_masterMode;
	bool m_done;

public:
	void constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
		int*slaveMode,int*masterMode,VirtualCommunicator*vc,GridTable*graph,VirtualProcessor*virtualProcessor);

	void call_RAY_SLAVE_MODE_PURGE_NULL_EDGES();

	/** initialize the whole thing */
	virtual void initializeMethod();

	virtual void finalizeMethod();
	virtual bool hasUnassignedTask();
	virtual Worker* assignNextTask();
	virtual void processWorkerResult(Worker*);
	virtual void destroyWorker(Worker*);

	void setProfiler(Profiler*profiler);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif
