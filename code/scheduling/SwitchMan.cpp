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
	cout<<"Closing remotely slave mode on rank "<<source<<endl;
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


RayMasterMode SwitchMan::getNextMasterMode(RayMasterMode currentMode){
	#ifdef ASSERT
	if(m_switches.count(currentMode)==0){
		cout<<"CurrentMode= "<<MASTER_MODES[currentMode]<<endl;
	}

	assert(m_switches.count(currentMode)>0);
	#endif

	return m_switches[currentMode];
}

void SwitchMan::addNextMasterMode(RayMasterMode a,RayMasterMode b){
	#ifdef ASSERT
	assert(m_switches.count(a)==0);
	#endif

	m_switches[a]=b;
}

void SwitchMan::openSlaveMode(Tag tag,StaticVector*outbox,Rank source,Rank destination){
	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Opening remotely slave mode on rank "<<destination<<endl;
	#endif

	#ifdef ASSERT
	assert(source == MASTER_RANK);
	#endif

	Message aMessage(NULL,0,destination,tag,source);
	outbox->push_back(aMessage);
}

/** send a signal to the switchman */
void SwitchMan::closeSlaveModeLocally(StaticVector*outbox,Rank source){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Closing locally slave mode on rank "<<source<<endl;
	#endif

	sendEmptyMessage(outbox,source,MASTER_RANK,RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL);

	setSlaveMode(RAY_SLAVE_MODE_DO_NOTHING);
}

void SwitchMan::openMasterMode(StaticVector*outbox,Rank source){

	#ifdef ASSERT
	assert(m_masterModeToTagTable.count(m_masterMode)>0);
	#endif

	Tag tag=m_masterModeToTagTable[m_masterMode];

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Opening master mode on rank "<<source<<endl;
	cout<<"tag= "<<MESSAGES[tag]<<" source= "<<source<<" Outbox= "<<outbox<<endl;
	#endif

	#ifdef ASSERT
	assert(source==MASTER_RANK);
	assert(outbox!=NULL);
	#endif


	for(Rank i=0;i<m_size;i++){
		openSlaveMode(tag,outbox,source,i);
	}

	reset();
}

void SwitchMan::closeMasterMode(){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Closing master mode on rank "<<MASTER_RANK<<endl;
	#endif

	RayMasterMode currentMasterMode=getMasterMode();
	RayMasterMode nextMode=getNextMasterMode(currentMasterMode);

	setMasterMode(nextMode);
}

void SwitchMan::sendEmptyMessage(StaticVector*outbox,Rank source,Rank destination,Tag tag){
	Message aMessage(NULL,0,destination,tag,source);
	outbox->push_back(aMessage);
}

void SwitchMan::sendToAll(StaticVector*outbox,Rank source,Tag tag){
	for(int i=0;i<m_size;i++){
		Message aMessage(NULL,0,i,tag,source);
		outbox->push_back(aMessage);
	}
}

void SwitchMan::openSlaveModeLocally(Tag tag,Rank rank){
	if(m_tagToSlaveModeTable.count(tag)==0)
		return;

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Opening locally slave mode on rank "<<rank<<endl;
	#endif

	RaySlaveMode desiredSlaveMode=m_tagToSlaveModeTable[tag];

	setSlaveMode(desiredSlaveMode);
}

void SwitchMan::addSlaveSwitch(Tag tag,RaySlaveMode slaveMode){
	#ifdef ASSERT
	assert(m_tagToSlaveModeTable.count(tag)==0);
	#endif

	m_tagToSlaveModeTable[tag]=slaveMode;
}

RaySlaveMode SwitchMan::getSlaveMode(){
	return m_slaveMode;
}

int*SwitchMan::getSlaveModePointer(){
	void*pointer=&m_slaveMode;
	return (int*)pointer;
}

void SwitchMan::setSlaveMode(RaySlaveMode mode){
	m_slaveMode=mode;
}

RayMasterMode SwitchMan::getMasterMode(){
	return m_masterMode;
}

void SwitchMan::setMasterMode(RayMasterMode mode){
	m_masterMode=mode;
}

int*SwitchMan::getMasterModePointer(){
	void*pointer= &m_masterMode;
	return(int*)pointer;
}

void SwitchMan::addMasterSwitch(RayMasterMode masterMode,Tag tag){
	#ifdef ASSERT
	assert(m_masterModeToTagTable.count(masterMode)==0);
	#endif

	m_masterModeToTagTable[masterMode]=tag;
}
