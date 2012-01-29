/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#include <scheduling/SwitchMan.h>
#include <assert.h>
#include <vector>
#include <core/OperatingSystem.h>
#include <iostream>
#include <core/ComputeCore.h>
using namespace std;

//#define CONFIG_SWITCHMAN_VERBOSITY

void SwitchMan::constructor(Rank rank,int numberOfCores){
	m_rank=rank;
	m_size=numberOfCores;
	reset();

	#ifdef ASSERT
	runAssertions();
	#endif

}

/** reset the sole counter */
void SwitchMan::reset(){
	m_counter=0;
	#ifdef ASSERT
	runAssertions();
	#endif
}

bool SwitchMan::allRanksAreReady(){
	#ifdef ASSERT
	runAssertions();
	#endif

	return m_counter==m_size;
}

void SwitchMan::closeSlaveMode(Rank source){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::closeSlaveMode] Closing remotely slave mode on rank "<<source<<endl;
	#endif

	m_counter++;

	#ifdef ASSERT
	runAssertions();
	#endif
}

void SwitchMan::runAssertions(){
	assert(m_counter<=m_size);
	assert(m_counter>=0);
}


MasterMode SwitchMan::getNextMasterMode(MasterMode currentMode){
	#ifdef ASSERT
	if(m_switches.count(currentMode)==0){
		cout<<"CurrentMode= "<<MASTER_MODES[currentMode]<<endl;
	}

	assert(m_switches.count(currentMode)>0);
	#endif

	return m_switches[currentMode];
}

void SwitchMan::addNextMasterMode(MasterMode a,MasterMode b){
	#ifdef ASSERT
	assert(m_switches.count(a)==0);
	#endif

	m_switches[a]=b;
}

void SwitchMan::openSlaveMode(MessageTag tag,StaticVector*outbox,Rank source,Rank destination){
	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::openSlaveMode] Opening remotely slave mode on rank "<<destination<<endl;
	#endif

	#ifdef ASSERT
	assert(source == MASTER_RANK); // only master can do that
	#endif

	sendMessage(NULL,0,outbox,source,destination,tag);
}

/** send a signal to the switchman */
void SwitchMan::closeSlaveModeLocally(StaticVector*outbox,Rank source){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::closeSlaveModeLocally] Closing locally slave mode on rank "<<source<<endl;
	#endif

	sendEmptyMessage(outbox,source,MASTER_RANK,RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL);

	// set the slave mode to do nothing
	// that is not productive...
	setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
}

void SwitchMan::openMasterMode(StaticVector*outbox,Rank source){

	#ifdef ASSERT
	if(m_masterModeToTagTable.count(m_masterMode)==0){
		cout<<"Error, master mode is "<<MASTER_MODES[m_masterMode]<<endl;
	}

	assert(m_masterModeToTagTable.count(m_masterMode)>0);
	#endif

	MessageTag tag=m_masterModeToTagTable[m_masterMode];

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::openMasterMode] Opening master mode on rank "<<source<<endl;
	cout<<"[SwitchMan::openMasterMode] tag= "<<MESSAGE_TAGS[tag]<<" source= "<<source<<" Outbox= "<<outbox<<endl;
	#endif

	#ifdef ASSERT
	assert(source==MASTER_RANK);
	assert(outbox!=NULL);
	#endif

	// open the slave mode on each MPI rank
	for(Rank i=0;i<m_size;i++){
		openSlaveMode(tag,outbox,source,i);
	}

	// reset the counter
	reset();
}

void SwitchMan::closeMasterMode(){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::closeMasterMode] Closing master mode on rank "<<MASTER_RANK<<endl;
	#endif

	// get the next master mode from the table
	MasterMode currentMasterMode=getMasterMode();

	if(currentMasterMode==RAY_MASTER_MODE_DO_NOTHING)
		currentMasterMode=m_lastMasterMode;

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::closeMasterMode] Current master mode -> "<<MASTER_MODES[currentMasterMode]<<endl;
	#endif

	MasterMode nextMode=getNextMasterMode(currentMasterMode);

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::closeMasterMode] Next master mode -> "<<MASTER_MODES[nextMode]<<endl;
	#endif

	// set the new master mode
	setMasterMode(nextMode);
}

void SwitchMan::sendEmptyMessage(StaticVector*outbox,Rank source,Rank destination,MessageTag tag){
	sendMessage(NULL,0,outbox,source,destination,tag);
}

void SwitchMan::sendMessage(uint64_t*buffer,int count,StaticVector*outbox,Rank source,Rank destination,MessageTag tag){
	// send a message
	Message aMessage(buffer,count,destination,tag,source);
	outbox->push_back(aMessage);
}

void SwitchMan::sendToAll(StaticVector*outbox,Rank source,MessageTag tag){
	sendMessageToAll(NULL,0,outbox,source,tag);
}

void SwitchMan::sendMessageToAll(uint64_t*buffer,int count,StaticVector*outbox,Rank source,MessageTag tag){
	for(int i=0;i<m_size;i++){
		sendMessage(buffer,count,outbox,source,i,tag);
	}
}

void SwitchMan::openSlaveModeLocally(MessageTag tag,Rank rank){
	if(m_tagToSlaveModeTable.count(tag)==0)
		return;

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::openSlaveModeLocally] Opening locally slave mode on rank "<<rank<<endl;
	#endif

	// translate the MPI tag to a slave mode 
	SlaveMode desiredSlaveMode=m_tagToSlaveModeTable[tag];

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::openSlaveModeLocally] Slave switch triggered, Tag -> "<<MESSAGE_TAGS[tag]<<endl;
	cout<<"[SwitchMan::openSlaveModeLocally] Slave mode -> "<<SLAVE_MODES[desiredSlaveMode]<<endl;
	#endif

	setSlaveMode(desiredSlaveMode);
}

void SwitchMan::addSlaveSwitch(MessageTag tag,SlaveMode slaveMode){
	#ifdef ASSERT
	assert(m_tagToSlaveModeTable.count(tag)==0);
	#endif

	m_tagToSlaveModeTable[tag]=slaveMode;
}

SlaveMode SwitchMan::getSlaveMode(){
	return m_slaveMode;
}

int*SwitchMan::getSlaveModePointer(){
	void*pointer=&m_slaveMode;
	return (int*)pointer;
}

void SwitchMan::setSlaveMode(SlaveMode mode){
	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"setSlaveMode "<<SLAVE_MODES[mode]<<endl;
	#endif

	m_slaveMode=mode;
}

MasterMode SwitchMan::getMasterMode(){
	return m_masterMode;
}

void SwitchMan::setMasterMode(MasterMode mode){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"setMasterMode "<<MASTER_MODES[mode]<<endl;
	#endif

	m_masterMode=mode;

	if(mode!=RAY_MASTER_MODE_DO_NOTHING)
		m_lastMasterMode=mode;
}

int*SwitchMan::getMasterModePointer(){
	void*pointer= &m_masterMode;
	return(int*)pointer;
}

void SwitchMan::addMasterSwitch(MasterMode masterMode,MessageTag tag){
	#ifdef ASSERT
	assert(m_masterModeToTagTable.count(masterMode)==0);
	#endif

	m_masterModeToTagTable[masterMode]=tag;
}

Rank SwitchMan::getRank(){
	return m_rank;
}

int SwitchMan::getSize(){
	return m_size;
}

/* the switch man do the accounting for ready ranks */
void SwitchMan::call_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL(Message*message){
	closeSlaveMode(message->getSource());
}

void SwitchMan::registerPlugin(ComputeCore*core){
	m_plugin=core->allocatePluginHandle();
	PluginHandle plugin=m_plugin;

	core->setPluginName(m_plugin,"SwitchMan");
	core->setPluginDescription(m_plugin,"Parallel coordinator (bundled with RayPlatform)");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU Lesser General License version 3");

	RAY_SLAVE_MODE_DO_NOTHING=core->allocateSlaveModeHandle(m_plugin,RAY_SLAVE_MODE_DO_NOTHING);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_DO_NOTHING,"RAY_SLAVE_MODE_DO_NOTHING");

	RAY_MASTER_MODE_DO_NOTHING=core->allocateMasterModeHandle(m_plugin,RAY_MASTER_MODE_DO_NOTHING);
	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_DO_NOTHING,"RAY_MASTER_MODE_DO_NOTHING");

	RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL=core->allocateMessageTagHandle(m_plugin,RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL);
	m_adapter_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL.setObject(this);
	core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL, &m_adapter_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL,"RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL");

	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON=core->allocateMessageTagHandle(plugin,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON");

	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY=core->allocateMessageTagHandle(plugin,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY);
	core->setMessageTagSymbol(plugin,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY");

	// set default modes
	// these symbols are resolved already
	
	setMasterMode(RAY_MASTER_MODE_DO_NOTHING); 
	setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);

}

void SwitchMan::resolveSymbols(ComputeCore*core){
	RAY_SLAVE_MODE_DO_NOTHING=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DO_NOTHING");
	RAY_SLAVE_MODE_DIE=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_DIE");

	RAY_MASTER_MODE_DO_NOTHING=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_DO_NOTHING");

	RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL");
	RAY_MPI_TAG_DUMMY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_DUMMY");

	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON");
	RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY");

	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON, RAY_SLAVE_MODE_DIE );

}


