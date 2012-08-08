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

#ifndef _Library
#define _Library

#include <communication/BufferedData.h>
#include <plugin_SeedExtender/ExtensionData.h>
#include <application_core/common_functions.h>
#include <structures/StaticVector.h>
#include <profiling/TimePrinter.h>
#include <plugin_SeedExtender/ReadFetcher.h>
#include <communication/VirtualCommunicator.h>
#include <application_core/Parameters.h>
#include <plugin_SeedingData/SeedingData.h>
#include <memory/RingAllocator.h>
#include <plugin_Library/LibraryWorker.h>
#include <handlers/SlaveModeHandler.h>
#include <handlers/MasterModeHandler.h>
#include <core/ComputeCore.h>

#include <map>
using namespace std;


/*
 * This class computes the average outer distances 
 * for paired reads and mate-pairs.
 * To do so, it spawns a pool of workers.
 * These workers will push messages and these
 * messages are grouped by the virtual communicator.
 * \author Sébastien Boisvert
 */
class Library :  public CorePlugin{

	MessageTag RAY_MPI_TAG_ASK_LIBRARY_DISTANCES_FINISHED;
	MessageTag RAY_MPI_TAG_AUTOMATIC_DISTANCE_DETECTION_IS_DONE;
	MessageTag RAY_MPI_TAG_LIBRARY_DISTANCE;
	MessageTag RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION;
	MessageTag RAY_MPI_TAG_UPDATE_LIBRARY_INFORMATION_REPLY;
	MessageTag RAY_MPI_TAG_GET_READ_MATE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_READS_REPLY;

	MasterMode RAY_MASTER_MODE_TRIGGER_EXTENSIONS;
	MasterMode RAY_MASTER_MODE_UPDATE_DISTANCES;
	MasterMode RAY_MASTER_MODE_TRIGGER_FUSIONS;

	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;
	SlaveMode RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION;
	SlaveMode RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES;
	

	int m_currentLibrary;
	int m_ranksThatReplied;
	int m_informationSent;

	int m_detectedDistances;
	vector<int> m_libraryIndexes;
	VirtualCommunicator*m_virtualCommunicator;
	BufferedData m_bufferedData;
	int m_ready;
	int m_rank;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	ExtensionData*m_ed;
	RingAllocator*m_outboxAllocator;
	int m_size;
	TimePrinter*m_timePrinter;
	int*m_mode;
	int*m_master_mode;
	Parameters*m_parameters;
	SeedingData*m_seedingData;
	int m_libraryIterator;
	map<int,map<int,int> > m_libraryDistances;
	map<int,int>::iterator m_libraryIndex;
	bool m_libraryIndexInitiated;
	bool m_workerInitiated;
	bool m_initiatedIterator;
	LargeIndex m_SEEDING_i;
	set<WorkerHandle> m_activeWorkers;
	set<WorkerHandle>::iterator m_activeWorkerIterator;
	int m_completedJobs;
	int m_maximumAliveWorkers;
	int m_maximumWorkers;

	map<WorkerHandle,LibraryWorker> m_aliveWorkers;
	bool m_communicatorWasTriggered;
	vector<WorkerHandle> m_workersDone;
	vector<WorkerHandle> m_waitingWorkers;
	vector<WorkerHandle> m_activeWorkersToRestore;

	MyAllocator m_allocator;
	void updateStates();

	void completeSlaveMode();

public:
	Library();
	void allocateBuffers();
	void call_RAY_MASTER_MODE_UPDATE_DISTANCES();
	void setReadiness();
	int getRank();
	int getSize();
	void call_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION();

	void call_RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES();

	void constructor(int m_rank,StaticVector*m_outbox,RingAllocator*m_outboxAllocator,
	ExtensionData*m_ed,int m_size,
TimePrinter*m_timePrinter,int*m_mode,int*m_master_mode,
Parameters*m_parameters,SeedingData*m_seedingData,StaticVector*inbox,VirtualCommunicator*vc
);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif

