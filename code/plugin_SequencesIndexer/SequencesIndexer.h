/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#include <plugin_SequencesIndexer/IndexerWorker.h>
#include <communication/Message.h>
#include <communication/BufferedData.h>
#include <communication/VirtualCommunicator.h>
#include <application_core/Parameters.h>
#include <application_core/common_functions.h>
#include <memory/MyAllocator.h>
#include <memory/RingAllocator.h>
#include <structures/SplayTree.h>
#include <structures/SplayTreeIterator.h>
#include <structures/StaticVector.h>
#include <plugin_SequencesLoader/ArrayOfReads.h>
#include <profiling/Derivative.h>
#include <plugin_SequencesLoader/Read.h>
#include <core/ComputeCore.h>

#include <map>
#include <vector>
#include <fstream>
using namespace std;


/*
 * Computes optimal read markers using workers.
 * \author Sébastien Boisvert
 */
class SequencesIndexer: public CorePlugin{

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
