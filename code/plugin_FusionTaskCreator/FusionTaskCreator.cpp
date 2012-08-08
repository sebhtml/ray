/*
 	Ray
    Copyright (C)  2011, 2012  Sébastien Boisvert

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

#include <plugin_FusionTaskCreator/FusionTaskCreator.h>
#include <plugin_FusionTaskCreator/FusionWorker.h>
#include <core/OperatingSystem.h>

__CreatePlugin(FusionTaskCreator);

 /**/
 /**/
__CreateSlaveModeAdapter(FusionTaskCreator,RAY_SLAVE_MODE_FUSION); /**/
 /**/
 /**/


void FusionTaskCreator::call_RAY_SLAVE_MODE_FUSION(){

	mainLoop();
}

void FusionTaskCreator::constructor(VirtualProcessor*virtualProcessor,StaticVector*outbox,
		RingAllocator*outboxAllocator,int*mode,Parameters*parameters,
		vector<vector<Kmer> >*paths,vector<PathHandle>*pathIdentifiers,
		set<PathHandle>*eliminated,VirtualCommunicator*virtualCommunicator){
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
	/* cout<<"FusionTaskCreator::initializeMethod()"<<endl; */
	m_iterator=0;
	m_currentWorkerIdentifier=0;
	m_reverseStrand=false;
}

/** finalize the whole thing */
void FusionTaskCreator::finalizeMethod(){

	if(m_fastRun && m_finishedInPreviousCycle){

		cout<<"Rank "<<m_parameters->getRank()<<" will not do anything, completion occured previously for fusing."<<endl;
	}

	/** all the paths */
	int numberOfPaths=m_paths->size();

	/** the number of eliminated paths */
	int eliminatedPaths=m_eliminated->size();

	/** make sure this number is at le ast all the rreverse paths */

	bool removedPaths=false;
	
	if(eliminatedPaths>= 1){
		removedPaths = true;
	}

	// nothing was eliminated here.
	if(!removedPaths){
		m_finishedInPreviousCycle=true;
	}

	cout<<"Rank "<<m_parameters->getRank()<<" FusionTaskCreator ["<<m_completedJobs<<"/"<<2*m_paths->size()<<"]"<<endl;
	cout<<"Statistics: all paths: "<<numberOfPaths<<" eliminated during fusing: "<<eliminatedPaths<<endl;

	/* send a message */
	MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(sizeof(MessageUnit));
	message[0]=removedPaths;
	Message aMessage(message,1,MASTER_RANK,RAY_MPI_TAG_FUSION_DONE,m_parameters->getRank());
	m_outbox->push_back(aMessage);

	/* set the mode */
	(*m_slaveMode)=RAY_SLAVE_MODE_DO_NOTHING;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
	}

	m_initialized=false;
}

/** has an unassigned task left to compute */
bool FusionTaskCreator::hasUnassignedTask(){
	
	// we have nothing to do if we finished in the previous cycle.
	if(m_fastRun && m_finishedInPreviousCycle){

		return false;
	}

	return m_iterator < (LargeCount)m_paths->size();
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

	if(m_currentWorkerIdentifier % 10== 0){
		cout<<"Rank "<<m_parameters->getRank()<<" FusionTaskCreator assignNextTask ["<<m_currentWorkerIdentifier<<"/"<<m_paths->size()*2<<"]"<<endl;

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_parameters->getRank());
			showDate();
		}
	}

	FusionWorker*worker=new FusionWorker;
	worker->constructor(m_currentWorkerIdentifier,&(m_paths->at(m_iterator)),m_pathIdentifiers->at(m_iterator),m_reverseStrand,m_virtualCommunicator,m_parameters,m_outboxAllocator,

	RAY_MPI_TAG_ASK_VERTEX_PATH,
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
	RAY_MPI_TAG_GET_PATH_LENGTH
);


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

	if(m_completedJobs % 10== 0){
		cout<<"Rank "<<m_parameters->getRank()<<" FusionTaskCreator processWorkerResult ["<<m_completedJobs<<"/"<<m_paths->size()*2<<"]"<<endl;

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_parameters->getRank());
			showDate();
		}
	}

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

void FusionTaskCreator::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();

	m_plugin=plugin;

	core->setPluginName(plugin,"FusionTaskCreator");
	core->setPluginDescription(plugin,"Contained paths are dumped");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_SLAVE_MODE_FUSION=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_FUSION, __GetAdapter(FusionTaskCreator,RAY_SLAVE_MODE_FUSION));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_FUSION,"RAY_SLAVE_MODE_FUSION");

	RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY=core->allocateMessageTagHandle(plugin);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY");

	m_finishedInPreviousCycle=false;

	// TODO: this is buggy
	m_fastRun=false;
}

void FusionTaskCreator::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_FUSION=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_FUSION");
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");

	RAY_MPI_TAG_ASK_VERTEX_PATH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATH");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE");
	RAY_MPI_TAG_GET_PATH_LENGTH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_LENGTH");
	RAY_MPI_TAG_FUSION_DONE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_FUSION_DONE");
	RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY");

	__BindPlugin(FusionTaskCreator);
}
