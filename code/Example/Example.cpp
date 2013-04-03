/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2013 Charles Joly Beauparlant

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

    This is derived from RayPlatform-example.

*/

#include "Example.h"

__CreatePlugin(Example);

__CreateMasterModeAdapter(Example,RAY_MASTER_MODE_STEP_A);
__CreateMasterModeAdapter(Example,RAY_MASTER_MODE_STEP_B);
__CreateMasterModeAdapter(Example,RAY_MASTER_MODE_STEP_C);

__CreateSlaveModeAdapter(Example,RAY_SLAVE_MODE_STEP_A);
__CreateSlaveModeAdapter(Example,RAY_SLAVE_MODE_STEP_B);
__CreateSlaveModeAdapter(Example,RAY_SLAVE_MODE_STEP_C);

__CreateMessageTagAdapter(Example,RAY_MPI_TAG_STOP_AND_DIE);
__CreateMessageTagAdapter(Example,RAY_MPI_TAG_TIME_BOMB);

Example::Example(){
	m_doneA=false;
	m_doneB=false;
	m_doneC=false;
}

void Example::call_RAY_MASTER_MODE_STEP_A(){
	if(m_doneA==false){
		m_doneA=true;
		cout<<"Rank "<<m_core->getRank()<<" call_RAY_MASTER_MODE_STEP_A"<<endl;

		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());
	}else if(m_core->getSwitchMan()->allRanksAreReady()){
		m_core->getSwitchMan()->closeMasterMode();
	}
}

void Example::call_RAY_MASTER_MODE_STEP_B(){
	if(m_doneB==false){
		m_doneB=true;
		cout<<"Rank "<<MASTER_RANK<<" call_RAY_MASTER_MODE_STEP_B"<<endl;
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());
	}else if(m_core->getSwitchMan()->allRanksAreReady()){
		m_core->getSwitchMan()->closeMasterMode();
	}
}

void Example::call_RAY_MASTER_MODE_STEP_C(){
	if(m_doneC==false){
		m_doneC=true;
		cout<<"Rank "<<MASTER_RANK<<" call_RAY_MASTER_MODE_STEP_C"<<endl;
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());
	}else if(m_core->getSwitchMan()->allRanksAreReady()){
		m_core->getSwitchMan()->closeMasterMode();
	}
}

void Example::call_RAY_SLAVE_MODE_STEP_A(){
	cout<<"I am "<<m_core->getRank()<<" doing call_RAY_SLAVE_MODE_STEP_A"<<endl;

	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
}

void Example::call_RAY_SLAVE_MODE_STEP_B(){
	cout<<"I am "<<m_core->getRank()<<" doing call_RAY_SLAVE_MODE_STEP_B"<<endl;

	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
}

void Example::call_RAY_SLAVE_MODE_STEP_C(){

	cout<<"I am "<<m_core->getRank()<<" doing call_RAY_SLAVE_MODE_STEP_C, now I die"<<endl;
	cout<<"This is over "<<endl;

	if(m_core->getRank()==0){
		uint64_t*buffer=(uint64_t*)m_core->getOutboxAllocator()->allocate(1*sizeof(uint64_t));
		buffer[0]=100;

		// compute the next destination
		Rank destination=m_core->getRank()+1;
		if(destination==m_core->getSize())
			destination=0;

		Message aMessage(buffer,1,destination,RAY_MPI_TAG_TIME_BOMB,m_core->getRank());

		// send the bomb to another rank
		m_core->getOutbox()->push_back(&aMessage);
	}

	// do nothing now
	m_core->getSwitchMan()->setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
}

void Example::call_RAY_MPI_TAG_TIME_BOMB(Message*message){
	uint64_t*buffer=message->getBuffer();

	if(buffer[0]==0){
		cout<<"The bomb exploded on rank "<<m_core->getRank()<<" !"<<endl;

		// kill everyone

		m_core->sendEmptyMessageToAll(RAY_MPI_TAG_STOP_AND_DIE);
	}else{
		uint64_t*bufferOut=(uint64_t*)m_core->getOutboxAllocator()->allocate(1*sizeof(uint64_t));
		bufferOut[0]=buffer[0]-1;

		cout<<"Remaining time before the explosion is "<<bufferOut[0]<<" according to rank "<<m_core->getRank()<<endl;

		// compute the next destination
		Rank destination=m_core->getRank()+1;
		if(destination==m_core->getSize())
			destination=0;

		Message aMessage(bufferOut,1,destination,RAY_MPI_TAG_TIME_BOMB,m_core->getRank());

		// send the bomb to another rank
		m_core->getOutbox()->push_back(&aMessage);
	}
}

void Example::call_RAY_MPI_TAG_STOP_AND_DIE(Message*message){

	cout<<"rank "<<m_core->getRank()<<" received message RAY_MPI_TAG_STOP_AND_DIE, this kills the batman"<<endl;

	m_core->stop();
}

void Example::registerPlugin(ComputeCore*core){

	/** register the m_plugin with the core **/

	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"Example");
        core->setPluginDescription(m_plugin,"Minimal plugin example");
        core->setPluginAuthors(m_plugin,"Charles Joly Beauparlant");
        core->setPluginLicense(m_plugin,"GNU General Public License version 3");

	// for each master mode, we allocate a handle after that, we register a handler for it
	//
	// allocateMasterModeHandle takes 1 arguments
	//  - the plugin handle

	RAY_MASTER_MODE_STEP_A=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_STEP_A,
		__GetAdapter(Example,RAY_MASTER_MODE_STEP_A));

	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_STEP_A,"RAY_MASTER_MODE_STEP_A");

	core->setMasterModePublicAccess(m_plugin, RAY_MASTER_MODE_STEP_A);

	RAY_MASTER_MODE_STEP_B=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_STEP_B,
		__GetAdapter(Example,RAY_MASTER_MODE_STEP_B));

	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_STEP_B,"RAY_MASTER_MODE_STEP_B");

	RAY_MASTER_MODE_STEP_C=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_STEP_C,
		__GetAdapter(Example,RAY_MASTER_MODE_STEP_C));

	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_STEP_C,"RAY_MASTER_MODE_STEP_C");

	// for each slave mode, we allocate a handle after that, we register a handler for it

	RAY_SLAVE_MODE_STEP_A=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_STEP_A,
		__GetAdapter(Example,RAY_SLAVE_MODE_STEP_A));

	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_STEP_A,"RAY_SLAVE_MODE_STEP_A");

	RAY_SLAVE_MODE_STEP_B=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_STEP_B,
		__GetAdapter(Example,RAY_SLAVE_MODE_STEP_B));

	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_STEP_B,"RAY_SLAVE_MODE_STEP_B");

	RAY_SLAVE_MODE_STEP_C=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_STEP_C,
		__GetAdapter(Example,RAY_SLAVE_MODE_STEP_C));

	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_STEP_C,"RAY_SLAVE_MODE_STEP_C");

	RAY_MPI_TAG_START_STEP_A=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin, RAY_MPI_TAG_START_STEP_A, "RAY_MPI_TAG_START_STEP_A");
	RAY_MPI_TAG_START_STEP_B=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin, RAY_MPI_TAG_START_STEP_B, "RAY_MPI_TAG_START_STEP_B");
	RAY_MPI_TAG_START_STEP_C=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin, RAY_MPI_TAG_START_STEP_C, "RAY_MPI_TAG_START_STEP_C");

	// now, we register the order of the master modes
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_STEP_A,RAY_MASTER_MODE_STEP_B);
	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_STEP_B,RAY_MASTER_MODE_STEP_C);

	// configure which control message to send given a master mode
	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_STEP_A,RAY_MPI_TAG_START_STEP_A);
	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_STEP_B,RAY_MPI_TAG_START_STEP_B);
	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_STEP_C,RAY_MPI_TAG_START_STEP_C);

	// configure which slave mode to set given a message tag
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_START_STEP_A,RAY_SLAVE_MODE_STEP_A);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_START_STEP_B,RAY_SLAVE_MODE_STEP_B);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_START_STEP_C,RAY_SLAVE_MODE_STEP_C);

	m_core=core;

	// configure the two message tags
	RAY_MPI_TAG_TIME_BOMB=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_TIME_BOMB,
		__GetAdapter(Example,RAY_MPI_TAG_TIME_BOMB));

	core->setMessageTagSymbol(m_plugin, RAY_MPI_TAG_TIME_BOMB, "RAY_MPI_TAG_TIME_BOMB");

	RAY_MPI_TAG_STOP_AND_DIE=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_STOP_AND_DIE,
		__GetAdapter(Example,RAY_MPI_TAG_STOP_AND_DIE));

	core->setMessageTagSymbol(m_plugin, RAY_MPI_TAG_STOP_AND_DIE, "RAY_MPI_TAG_STOP_AND_DIE");

	/* a plugin can share any of its object with other plugins **/
	/** other plugins have to resolve the symbol. **/
	void*object=&m_doneA;
	core->setObjectSymbol(m_plugin,object,"BooleanState");
}

void Example::resolveSymbols(ComputeCore*core){

	// here, we resolve symbols owned by other m_plugins
	// but needed by the current m_plugin
	// obviously, this is not needed here because
	// we only have one m_plugin

	RAY_SLAVE_MODE_STEP_A=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_STEP_A");

	// this slave mode is registered somewhere in RayPlatform
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");

	bool*example=(bool*) core->getObjectFromSymbol(m_plugin,"BooleanState");

	if(*example)
		*example=false;

	__BindPlugin(Example);

	__BindAdapter(Example,RAY_MASTER_MODE_STEP_A);
	__BindAdapter(Example,RAY_MASTER_MODE_STEP_B);
	__BindAdapter(Example,RAY_MASTER_MODE_STEP_C);

	__BindAdapter(Example,RAY_SLAVE_MODE_STEP_A);
	__BindAdapter(Example,RAY_SLAVE_MODE_STEP_B);
	__BindAdapter(Example,RAY_SLAVE_MODE_STEP_C);

	__BindAdapter(Example,RAY_MPI_TAG_STOP_AND_DIE);
	__BindAdapter(Example,RAY_MPI_TAG_TIME_BOMB);
}
