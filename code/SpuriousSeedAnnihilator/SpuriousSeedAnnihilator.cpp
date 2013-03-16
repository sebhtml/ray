/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
 *  Copyright (C) 2013 Sébastien Boisvert
 *
 *  http://DeNovoAssembler.SourceForge.Net/
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You have received a copy of the GNU General Public License
 *  along with this program (gpl-3.0.txt).
 *  see <http://www.gnu.org/licenses/>
 */

#include "SpuriousSeedAnnihilator.h"

__CreatePlugin(SpuriousSeedAnnihilator);

__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);

__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);

__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);

SpuriousSeedAnnihilator::SpuriousSeedAnnihilator(){

}

/*
 * Methods to implement for the TaskCreator interface.
 *
 * The TaskCreator stack is used in the handler
 * RAY_SLAVE_MODE_FILTER_SEEDS.
 */

/** initialize the whole thing */
void SpuriousSeedAnnihilator::initializeMethod(){

}

/** finalize the whole thing */
void SpuriousSeedAnnihilator::finalizeMethod(){

}

/** has an unassigned task left to compute */
bool SpuriousSeedAnnihilator::hasUnassignedTask(){
	return false;
}

/** assign the next task to a worker and return this worker */
Worker*SpuriousSeedAnnihilator::assignNextTask(){
	return NULL;
}

/** get the result of a worker */
void SpuriousSeedAnnihilator::processWorkerResult(Worker*worker){

}

/** destroy a worker */
void SpuriousSeedAnnihilator::destroyWorker(Worker*worker){

}

/*
 * handlers.
 */
void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_REGISTER_SEEDS(){

	if(!m_distributionIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_distributionIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		m_core->getSwitchMan()->closeMasterMode();
	}
}

void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_FILTER_SEEDS(){

	if(!m_filteringIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_filteringIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){
		m_core->getSwitchMan()->closeMasterMode();
	}
}

void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_CLEAN_SEEDS(){

	if(!m_cleaningIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_cleaningIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){
		m_core->getSwitchMan()->closeMasterMode();
	}
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_REGISTER_SEEDS(){

	if(m_inbox->hasMessage(RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY))
		m_activeQueries--;

	#ifdef ASSERT
	assert(m_activeQueries>=0);
	#endif

	if(m_activeQueries > 0)
		return;

	#ifdef ASSERT
	assert(m_activeQueries==0);
	#endif

	if(m_seedIndex < (int)m_seeds->size()){

		if(m_seedPosition < (*m_seeds)[m_seedIndex].size()){

			if(m_seedIndex==0 && m_seedPosition == 0)
				cout<<"Rank "<<m_core->getRank()<<" has "<<m_seeds->size()<<" seeds to register."<<endl;

			Kmer vertex;
			(*m_seeds)[m_seedIndex].at(m_seedPosition,&vertex);

			Rank destination=m_parameters->vertexRank(&vertex);

			MessageTag tag=RAY_MESSAGE_TAG_PUSH_SEEDS;
			int elements = m_virtualCommunicator->getElementsPerQuery(tag);

			for(int i=0;i<vertex.getNumberOfU64();i++){
				m_buffers.addAt(destination,vertex.getU64(i));
			}

			PathHandle identifier = getPathUniqueId(m_rank, m_seedIndex);
			m_buffers.addAt(destination, identifier);
			m_buffers.addAt(destination, m_seedPosition);

			if(m_buffers.flush(destination, elements, tag, m_outboxAllocator, m_outbox, m_rank,false)){
				m_activeQueries++;
			}

			m_seedPosition++;
		}else{

			m_seedIndex++;
		}
	}else{
		m_seedIndex=0;
		m_seedPosition=0;

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
	}
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_FILTER_SEEDS(){

	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_CLEAN_SEEDS(){

	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
}

/**
 * The 3 methods below are not used, but they were added to test this
 * new RayPlatform API call:
 *
 * __ConfigureMessageTagHandler
 *
 */
void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_REGISTER_SEEDS(Message*message){
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_FILTER_SEEDS(Message*message){
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_CLEAN_SEEDS(Message*message){
}

/*
 * Methods to implement for the CorePlugin interface.
 */
void SpuriousSeedAnnihilator::registerPlugin(ComputeCore*core){

	m_core=core;
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"SpuriousSeedAnnihilator");
	core->setPluginDescription(plugin,"Pre-processing of seeds.");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);

	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);

	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);

	__BindPlugin(SpuriousSeedAnnihilator);

	m_outboxAllocator = m_core->getOutboxAllocator();
	m_outbox = m_core->getOutbox() ;
	m_rank = m_core->getRank() ;
	m_inbox = m_core->getInbox();

	m_size = m_core->getSize();

	m_distributionIsStarted=false;
	m_filteringIsStarted=false;
	m_cleaningIsStarted=false;

	m_seedIndex=0;
	m_seedPosition=0;

	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_REGISTER_SEEDS, RAY_MASTER_MODE_FILTER_SEEDS);
	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_FILTER_SEEDS, RAY_MASTER_MODE_CLEAN_SEEDS);

	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_REGISTER_SEEDS, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_REGISTER_SEEDS, RAY_SLAVE_MODE_REGISTER_SEEDS);

	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_FILTER_SEEDS, RAY_MESSAGE_TAG_FILTER_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_FILTER_SEEDS, RAY_SLAVE_MODE_FILTER_SEEDS);

	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_CLEAN_SEEDS, RAY_MESSAGE_TAG_CLEAN_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_CLEAN_SEEDS, RAY_SLAVE_MODE_CLEAN_SEEDS);

	m_activeQueries=0;

	m_virtualCommunicator = m_core->getVirtualCommunicator();
}

void SpuriousSeedAnnihilator::resolveSymbols(ComputeCore*core){

	// before arriving here, there is this edition to be performed elsewhere in the code
	// replace RAY_MASTER_MODE_TRIGGER_DETECTION by RAY_MASTER_MODE_CLEAN_SEEDS

	RAY_MASTER_MODE_TRIGGER_DETECTION=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_DETECTION");
	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_CLEAN_SEEDS, RAY_MASTER_MODE_TRIGGER_DETECTION);

	m_seeds=(vector<GraphPath>*) m_core->getObjectFromSymbol(m_plugin, "/RayAssembler/ObjectStore/Seeds.ray");
	m_parameters=(Parameters*)m_core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");

	RAY_MESSAGE_TAG_PUSH_SEEDS = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MESSAGE_TAG_PUSH_SEEDS");
	RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY");

	int elements = m_virtualCommunicator->getElementsPerQuery(RAY_MESSAGE_TAG_PUSH_SEEDS);

	m_buffers.constructor(m_size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),
		"/memory/SpuriousSeedAnnihilator",m_parameters->showMemoryAllocations(),elements);
}
