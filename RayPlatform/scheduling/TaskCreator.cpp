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

#include <scheduling/TaskCreator.h>
#include <stdlib.h> /* for NULL */

#ifdef ASSERT
#include <assert.h>
#endif

/* #define DEBUG_TASK_CREATOR */

/** the main loop for a task creator */
void TaskCreator::mainLoop(){
	if(!m_initialized){
		#ifdef DEBUG_TASK_CREATOR
		cout<<"Initializing"<<endl;
		#endif
		initializeMethod();
		m_initialized=true;
		m_completedJobs=0;
	}

	if(hasUnassignedTask()){
		if(m_virtualProcessor->canAddWorker()){
			Worker*worker=assignNextTask();


			#ifdef DEBUG_TASK_CREATOR
			cout<<"Adding worker to pool worker= "<<worker<<" processor="<<m_virtualProcessor<<endl;
			#endif

			#ifdef ASSERT
			assert(worker != NULL);
			assert(m_virtualProcessor != NULL);
			#endif

			m_virtualProcessor->addWorker(worker);
	
			/* tell the VirtualProcessor that no more tasks will be created */
			if(!hasUnassignedTask()){
				#ifdef DEBUG_TASK_CREATOR
				cout<<"No more task are coming."<<endl;
				#endif
				m_virtualProcessor->noMoreTasksAreComing();
			}
		}
	}

	Worker*worker=NULL;

	bool aWorkerWorked=m_virtualProcessor->run();


	if(aWorkerWorked){
		worker=m_virtualProcessor->getCurrentWorker();
	}

	if(worker!=NULL && worker->isDone()){
		#ifdef DEBUG_TASK_CREATOR
		cout<<"Current worker is done"<<endl;
		#endif
		processWorkerResult(worker);
		destroyWorker(worker);
		m_completedJobs++;
	}

	if(!hasUnassignedTask() && !m_virtualProcessor->hasWorkToDo()){
		finalizeMethod();

		m_virtualProcessor->printStatistics();
		m_virtualProcessor->reset();

		#ifdef DEBUG_TASK_CREATOR
		cout<<"calling finalizeMethod()"<<endl;
		#endif

	}
}
