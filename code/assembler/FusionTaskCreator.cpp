/*
 	Ray
    Copyright (C)  2011  Sébastien Boisvert

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

#include <assembler/FusionTaskCreator.h>
#include <assembler/FusionWorker.h>
#include <core/OperatingSystem.h>

void FusionTaskCreator::makeFusions(){

	mainLoop();
}

void FusionTaskCreator::constructor(VirtualProcessor*virtualProcessor,StaticVector*outbox,
		RingAllocator*outboxAllocator,int*mode,Parameters*parameters,
		vector<vector<Kmer> >*paths,vector<uint64_t>*pathIdentifiers,
		set<uint64_t>*eliminated,VirtualCommunicator*virtualCommunicator){
	m_virtualCommunicator=virtualCommunicator;

	m_eliminated=eliminated;
	m_paths=paths;
	m_pathIdentifiers=pathIdentifiers;

	m_outboxAllocator=outboxAllocator;
	m_outbox=outbox;
	m_slaveMode=mode;
	m_parameters=parameters;

	/* for TaskCreator */
	m_initialized=false;
	m_virtualProcessor=virtualProcessor;
}

/** initialize the whole thing */
void FusionTaskCreator::initializeMethod(){
	cout<<"FusionTaskCreator::initializeMethod()"<<endl;
	m_iterator=0;
	m_currentWorkerIdentifier=0;
	m_reverseStrand=false;
}

/** finalize the whole thing */
void FusionTaskCreator::finalizeMethod(){

	cout<<"Rank "<<m_parameters->getRank()<<" FusionTaskCreator ["<<m_completedJobs<<"/"<<2*m_paths->size()<<"]"<<endl;

	/* send a message */
	uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(sizeof(uint64_t));
	message[0]=false;
	Message aMessage(message,1,MASTER_RANK,RAY_MPI_TAG_FUSION_DONE,m_parameters->getRank());
	m_outbox->push_back(aMessage);

	/* set the mode */
	(*m_slaveMode)=RAY_SLAVE_MODE_DO_NOTHING;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
	}

}

/** has an unassigned task left to compute */
bool FusionTaskCreator::hasUnassignedTask(){
	return m_iterator < (uint64_t)m_paths->size();
}

/** assign the next task to a worker and return this worker 
 *
 * \author Sébastien Boisvert
 *
 * Code reviews
 *
 * 2011-08-30 -- Code review by Élénie Godzaridis (found bug with new/malloc)
 */
Worker*FusionTaskCreator::assignNextTask(){
	FusionWorker*worker=new FusionWorker;
	worker->constructor(m_currentWorkerIdentifier,&(m_paths->at(m_iterator)),m_pathIdentifiers->at(m_iterator),m_reverseStrand,m_virtualCommunicator,m_parameters,m_outboxAllocator);


	m_currentWorkerIdentifier++;
	if(m_reverseStrand){
		m_iterator++;
		m_reverseStrand=false;
	}else{
		m_reverseStrand=true;
	}

	return worker;
}

/** get the result of a worker */
void FusionTaskCreator::processWorkerResult(Worker*worker){
	if(m_completedJobs % 10== 0)
		cout<<"Rank "<<m_parameters->getRank()<<" FusionTaskCreator ["<<m_completedJobs<<"/"<<m_paths->size()*2<<"]"<<endl;

	FusionWorker*worker2=(FusionWorker*)worker;
	if(worker2->isPathEliminated()){
		if(m_parameters->hasOption("-debug-fusions"))
			cout<<"eliminated !"<<endl;
		m_eliminated->insert(worker2->getPathIdentifier());
	}else{
		if(m_parameters->hasOption("-debug-fusions"))
			cout<<"kept !"<<endl;
	}
}

/** destroy a worker */
void FusionTaskCreator::destroyWorker(Worker*worker){
	delete worker;
}

