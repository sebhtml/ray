/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#ifndef _SequencesIndexer
#define _SequencesIndexer

#include "IndexerWorker.h"

#include <code/Mock/Parameters.h>
#include <code/Mock/common_functions.h>
#include <code/SequencesLoader/ArrayOfReads.h>
#include <code/SequencesLoader/Read.h>

#include <RayPlatform/profiling/Derivative.h>
#include <RayPlatform/core/ComputeCore.h>
#include <RayPlatform/communication/Message.h>
#include <RayPlatform/memory/MyAllocator.h>
#include <RayPlatform/memory/RingAllocator.h>
#include <RayPlatform/structures/SplayTree.h>
#include <RayPlatform/structures/SplayTreeIterator.h>
#include <RayPlatform/structures/StaticVector.h>
#include <RayPlatform/communication/BufferedData.h>
#include <RayPlatform/communication/VirtualCommunicator.h>

#include <map>
#include <vector>
#include <fstream>
using namespace std;

__DeclarePlugin(SequencesIndexer);

__DeclareSlaveModeAdapter(SequencesIndexer,RAY_SLAVE_MODE_INDEX_SEQUENCES);

/*
 * Computes optimal read markers using workers.
 * \author Sébastien Boisvert
 */
class SequencesIndexer: public CorePlugin{

	__AddAdapter(SequencesIndexer,RAY_SLAVE_MODE_INDEX_SEQUENCES);

	MessageTag RAY_MPI_TAG_GET_READ_MARKERS_REPLY;
	MessageTag RAY_MPI_TAG_GET_READ_MATE_REPLY;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_READS_FROM_LIST_REPLY;
	MessageTag RAY_MPI_TAG_VERTEX_READS_REPLY;

	MessageTag RAY_MPI_TAG_MASTER_IS_DONE_ATTACHING_READS_REPLY;
	MessageTag RAY_MPI_TAG_ATTACH_SEQUENCE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;

	SlaveMode RAY_SLAVE_MODE_INDEX_SEQUENCES;
	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;


	Derivative m_derivative;

	ofstream m_readMarkerFile;

	map<int,map<int,int> >m_forwardStatistics;
	map<int,map<int,int> >m_reverseStatistics;

	/** for checkpointing */
	bool m_checkedCheckpoint;

	int m_rank;
	int m_size;
	Parameters*m_parameters;
	MyAllocator m_allocator;
	MyAllocator m_workAllocator;
	int m_pendingMessages;
	int m_completedJobs;
	int m_maximumAliveWorkers;
	int m_maximumWorkers;

	SlaveMode*m_mode;
	StaticVector*m_outbox;

	VirtualCommunicator*m_virtualCommunicator;
	SplayTree<WorkerHandle,char> m_activeWorkers;
	SplayTreeIterator<WorkerHandle,char> m_activeWorkerIterator;

	SplayTree<WorkerHandle,IndexerWorker> m_aliveWorkers;
	
	bool m_communicatorWasTriggered;
	vector<WorkerHandle> m_workersDone;
	vector<WorkerHandle> m_waitingWorkers;
	vector<WorkerHandle> m_activeWorkersToRestore;

	ArrayOfReads*m_myReads;
	RingAllocator*m_outboxAllocator;

	bool m_initiatedIterator;
	int m_theSequenceId;
	void updateStates();

public:

	void call_RAY_SLAVE_MODE_INDEX_SEQUENCES();

	void constructor(Parameters*parameters,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,
	VirtualCommunicator*vc,SlaveMode*mode,ArrayOfReads*myReads);

	void setReadiness();
	MyAllocator*getAllocator();

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif
