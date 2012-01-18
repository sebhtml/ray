/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#ifndef _TaskCreator_H
#define _TaskCreator_H

#include <scheduling/Worker.h>
#include <scheduling/VirtualProcessor.h>
#include <memory/MyAllocator.h>

/** this is an interface that should be implemented by child classes 
 * m_initialized must be set to false in the child class
 * */
class TaskCreator{
protected:
	bool m_initialized;
	VirtualProcessor*m_virtualProcessor;
	uint64_t m_completedJobs;

public:
	/** main function */
	void mainLoop();

	/** initialize the whole thing */
	virtual void initializeMethod() = 0;

	/** finalize the whole thing */
	virtual void finalizeMethod() = 0;

	/** has an unassigned task left to compute */
	virtual bool hasUnassignedTask() = 0;

	/** assign the next task to a worker and return this worker */
	virtual Worker*assignNextTask() = 0;

	/** get the result of a worker */
	virtual void processWorkerResult(Worker*worker) = 0;

	/** destroy a worker */
	virtual void destroyWorker(Worker*worker) = 0;

	virtual ~TaskCreator(){}
};

#endif
