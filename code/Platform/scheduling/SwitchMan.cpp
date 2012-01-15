/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#include <scheduling/SwitchMan.h>
#include <assert.h>
#include <vector>
#include <core/constants.h>
#include <iostream>
#include <iostream>
using namespace std;

//#define CONFIG_SWITCHMAN_VERBOSITY

void SwitchMan::constructor(int numberOfCores){
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

void SwitchMan::openSlaveMode(Tag tag,StaticVector*outbox,Rank source,Rank destination){
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
	assert(m_masterModeToTagTable.count(m_masterMode)>0);
	#endif

	Tag tag=m_masterModeToTagTable[m_masterMode];

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::openMasterMode] Opening master mode on rank "<<source<<endl;
	cout<<"[SwitchMan::openMasterMode] tag= "<<MESSAGES[tag]<<" source= "<<source<<" Outbox= "<<outbox<<endl;
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

void SwitchMan::sendEmptyMessage(StaticVector*outbox,Rank source,Rank destination,Tag tag){
	sendMessage(NULL,0,outbox,source,destination,tag);
}

void SwitchMan::sendMessage(uint64_t*buffer,int count,StaticVector*outbox,Rank source,Rank destination,Tag tag){
	// send a message
	Message aMessage(buffer,count,destination,tag,source);
	outbox->push_back(aMessage);
}

void SwitchMan::sendToAll(StaticVector*outbox,Rank source,Tag tag){
	sendMessageToAll(NULL,0,outbox,source,tag);
}

void SwitchMan::sendMessageToAll(uint64_t*buffer,int count,StaticVector*outbox,Rank source,Tag tag){
	for(int i=0;i<m_size;i++){
		sendMessage(buffer,count,outbox,source,i,tag);
	}
}

void SwitchMan::openSlaveModeLocally(Tag tag,Rank rank){
	if(m_tagToSlaveModeTable.count(tag)==0)
		return;

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::openSlaveModeLocally] Opening locally slave mode on rank "<<rank<<endl;
	#endif

	// translate the MPI tag to a slave mode 
	SlaveMode desiredSlaveMode=m_tagToSlaveModeTable[tag];

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"[SwitchMan::openSlaveModeLocally] Slave switch triggered, Tag -> "<<MESSAGES[tag]<<endl;
	cout<<"[SwitchMan::openSlaveModeLocally] Slave mode -> "<<SLAVE_MODES[desiredSlaveMode]<<endl;
	#endif

	setSlaveMode(desiredSlaveMode);
}

void SwitchMan::addSlaveSwitch(Tag tag,SlaveMode slaveMode){
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
	m_slaveMode=mode;
}

MasterMode SwitchMan::getMasterMode(){
	return m_masterMode;
}

void SwitchMan::setMasterMode(MasterMode mode){
	m_masterMode=mode;

	if(mode!=RAY_MASTER_MODE_DO_NOTHING)
		m_lastMasterMode=mode;
}

int*SwitchMan::getMasterModePointer(){
	void*pointer= &m_masterMode;
	return(int*)pointer;
}

void SwitchMan::addMasterSwitch(MasterMode masterMode,Tag tag){
	#ifdef ASSERT
	assert(m_masterModeToTagTable.count(masterMode)==0);
	#endif

	m_masterModeToTagTable[masterMode]=tag;
}

