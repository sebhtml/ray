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

#include <plugin_JoinerTaskCreator/JoinerTaskCreator.h>
#include <plugin_JoinerTaskCreator/JoinerWorker.h>
#include <core/OperatingSystem.h>

__CreatePlugin(JoinerTaskCreator);

 /**/
 /**/
__CreateSlaveModeAdapter(JoinerTaskCreator,RAY_SLAVE_MODE_FINISH_FUSIONS); /**/
 /**/
 /**/


void JoinerTaskCreator::call_RAY_SLAVE_MODE_FINISH_FUSIONS(){
	mainLoop();
}

void JoinerTaskCreator::constructor(VirtualProcessor*virtualProcessor,StaticVector*outbox,
		RingAllocator*outboxAllocator,int*mode,Parameters*parameters,
		vector<vector<Kmer> >*paths,vector<PathHandle>*pathIdentifiers,
		set<PathHandle>*eliminated,VirtualCommunicator*virtualCommunicator,
		vector<vector<Kmer> >*newPaths){
	m_newPaths=newPaths;

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
void JoinerTaskCreator::initializeMethod(){
	/* cout<<"JoinerTaskCreator::initializeMethod()"<<endl; */
	m_iterator=0;
	m_currentWorkerIdentifier=0;
	m_reverseStrand=false;
}

/** finalize the whole thing */
void JoinerTaskCreator::finalizeMethod(){

	if(m_fastRun && m_previouslyDone){

		cout<<"Rank "<<m_parameters->getRank()<<" will not do anything, completion occured previously for joining."<<endl;
	}

	/** all the paths */
	int numberOfPaths=m_paths->size();

	/** the number of eliminated paths */
	int eliminatedPaths=m_eliminated->size();

	/** make sure this number is at le ast all the rreverse paths */

	bool removedPaths=false;
	
	if(eliminatedPaths >= 1){
		removedPaths = true;
	}
	
	// nothing was eliminated
	if(!removedPaths){
		m_previouslyDone=true;
	}

	cout<<"Rank "<<m_parameters->getRank()<<" JoinerTaskCreator ["<<m_completedJobs<<"/"<<2*m_paths->size()<<"]"<<endl;
	cout<<"Statistics: all paths: "<<numberOfPaths<<" eliminated during joining: "<<eliminatedPaths<<endl;

	/* send a message */
	MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(sizeof(MessageUnit));
	message[0]=removedPaths;
	Message aMessage(message,1,MASTER_RANK,RAY_MPI_TAG_FINISH_FUSIONS_FINISHED,m_parameters->getRank());
	m_outbox->push_back(aMessage);

	/* set the mode */
	(*m_slaveMode)=RAY_SLAVE_MODE_DO_NOTHING;

	if(m_parameters->showMemoryUsage()){
		showMemoryUsage(m_parameters->getRank());
		showDate();
	}

	m_initialized=false;
}

/** has an unassigned task left to compute */
bool JoinerTaskCreator::hasUnassignedTask(){

	if(m_fastRun && m_previouslyDone){
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
Worker*JoinerTaskCreator::assignNextTask(){

	if(m_currentWorkerIdentifier % 10== 0){
		cout<<"Rank "<<m_parameters->getRank()<<" JoinerTaskCreator assignNextTask ["<<m_currentWorkerIdentifier<<"/"<<m_paths->size()*2<<"]"<<endl;

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_parameters->getRank());
			showDate();
		}
	}

	JoinerWorker*worker=new JoinerWorker;
	worker->constructor(m_currentWorkerIdentifier,&(m_paths->at(m_iterator)),m_pathIdentifiers->at(m_iterator),m_reverseStrand,m_virtualCommunicator,m_parameters,m_outboxAllocator,
	m_newPaths,

	RAY_MPI_TAG_ASK_VERTEX_PATH,
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
	RAY_MPI_TAG_GET_PATH_LENGTH,
	RAY_MPI_TAG_GET_PATH_VERTEX
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
void JoinerTaskCreator::processWorkerResult(Worker*worker){

	if(m_completedJobs % 10== 0){
		cout<<"Rank "<<m_parameters->getRank()<<" JoinerTaskCreator processWorkerResult ["<<m_completedJobs<<"/"<<m_paths->size()*2<<"]"<<endl;

		if(m_parameters->showMemoryUsage()){
			showMemoryUsage(m_parameters->getRank());
			showDate();
		}
	}

	JoinerWorker*worker2=(JoinerWorker*)worker;
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
void JoinerTaskCreator::destroyWorker(Worker*worker){
	delete worker;
}

void JoinerTaskCreator::registerPlugin(ComputeCore*core){
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"JoinerTaskCreator");
	core->setPluginDescription(plugin,"Similar paths are joined");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_SLAVE_MODE_FINISH_FUSIONS=core->allocateSlaveModeHandle(plugin);
	core->setSlaveModeObjectHandler(plugin,RAY_SLAVE_MODE_FINISH_FUSIONS, __GetAdapter(JoinerTaskCreator,RAY_SLAVE_MODE_FINISH_FUSIONS));
	core->setSlaveModeSymbol(plugin,RAY_SLAVE_MODE_FINISH_FUSIONS,"RAY_SLAVE_MODE_FINISH_FUSIONS");

	m_previouslyDone=false;

	// TODO this is buggy
	m_fastRun=false;
}

void JoinerTaskCreator::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_FINISH_FUSIONS=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_FINISH_FUSIONS");
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");

	RAY_MPI_TAG_ASK_VERTEX_PATH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATH");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE");
	RAY_MPI_TAG_GET_PATH_LENGTH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_LENGTH");
	RAY_MPI_TAG_GET_PATH_VERTEX=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_PATH_VERTEX");
	RAY_MPI_TAG_FINISH_FUSIONS_FINISHED=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_FINISH_FUSIONS_FINISHED");

	__BindPlugin(JoinerTaskCreator);
}
