/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#ifndef _VirtualProcessor_h
#define _VirtualProcessor_h

#include <structures/StaticVector.h>
#include <memory/RingAllocator.h>
#include <scheduling/Worker.h>
#include <plugins/CorePlugin.h>

class ComputeCore;

#include <set>
#include <vector>
using namespace std;

/* workflow:

design by Sébastien Boisvert
2011-08-25

Input:

- array of tasks to perform
- a VirtualCommunicator object (software)
- a VirtualProcessor object (software)

Output:

- the result of all tasks


pseudo code:


work()

	if has unassigned task to do
		if virtualProcessor.canAddWorker
			worker = worker given the said task (constructor)
			virtualProcessor.addWorker(worker)

			if this is the last task to do
				virtualProcessor.noMoreTasksAreComing
	}

	virtualProcessor.run()
	worker = virtualProcessor.getCurrentWorker

	if worker.isDone
		get the result of the worker
		do something meaningful with the result
		virtualProcessor.removeWorker(worker) (done automatically)
		destroy the worker

	if all tasks are finished
		the is THE END

*/


/** an implementation of a VirtualProcessor
 * It enables the sheduling of many workers on the same process.
 * While some sleep, other work.
 * Workers sleep when they wait for messages 
 *
 * \author Sébastien Boisvert
*/
class VirtualProcessor: public CorePlugin{

	uint64_t m_currentWorker;

	bool m_moreTasksAreComing;

	bool m_initiatedIterator;
	set<uint64_t> m_activeWorkers;
	set<uint64_t>::iterator m_activeWorkerIterator;
	int m_completedJobs;
	int m_maximumAliveWorkers;
	int m_maximumWorkers;
	bool m_communicatorWasTriggered;
	vector<uint64_t> m_workersDone;
	vector<uint64_t> m_waitingWorkers;
	vector<uint64_t> m_activeWorkersToRestore;

	map<uint64_t,Worker*> m_aliveWorkers;

	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	VirtualCommunicator*m_virtualCommunicator;

	void updateStates();

public:
	/** actually initialize the VirtualProcessor */
	void constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,
		VirtualCommunicator*vc);

	/** reset everything in the VirtualProcessor */
	void reset();

	/** make the VirtualProcessor run a little bit */
	/** returns true if a worker worked */
	bool run();

	/** get the current worker */
	Worker*getCurrentWorker();

	/** can a worker be added ? */
	bool canAddWorker();

	/** add a worker */
	void addWorker(Worker*worker);

	/** no more task are coming */
	void noMoreTasksAreComing();

	/** returns true if some workers are alive */
	bool hasWorkToDo();

	void printStatistics();

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif
