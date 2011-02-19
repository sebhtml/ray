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

*/

// TYPE: ALGORITHM

#ifndef _SequencesIndexer
#define _SequencesIndexer

#include<common_functions.h>
#include<Message.h>
#include<StaticVector.h>
#include<vector>
#include<ArrayOfReads.h>
#include<BufferedData.h>
#include<MyAllocator.h>
#include<RingAllocator.h>
#include <VirtualCommunicator.h>
#include <Read.h>
#include <IndexerWorker.h>
#include <Parameters.h>

class SequencesIndexer{
	int m_rank;
	int m_size;
	Parameters*m_parameters;
	MyAllocator m_allocator;
	int m_pendingMessages;
	int m_completedJobs;
	int m_maximumAliveWorkers;
	int m_maximumWorkers;

	VirtualCommunicator m_virtualCommunicator;
	set<uint64_t> m_activeWorkers;
	set<uint64_t>::iterator m_activeWorkerIterator;
	map<uint64_t,IndexerWorker> m_aliveWorkers;
	bool m_communicatorWasTriggered;
	vector<uint64_t> m_workersDone;
	vector<uint64_t> m_waitingWorkers;
	vector<uint64_t> m_activeWorkersToRestore;

	bool m_initiatedIterator;
	int m_theSequenceId;

	void updateStates();

public:

	void attachReads(
ArrayOfReads*m_myReads,
				RingAllocator*m_outboxAllocator,
				StaticVector*m_outbox,
				int*m_mode,
				int m_wordSize,
				int m_size,
				int m_rank,
				bool m_colorSpaceMode
);

	void constructor(Parameters*parameters,RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox);
	void setReadiness();
	MyAllocator*getAllocator();
};

#endif
