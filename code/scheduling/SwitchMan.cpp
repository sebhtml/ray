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
#include <core/slave_modes.h>
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

void SwitchMan::closeSlaveMode(int source){

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


int SwitchMan::getNextMasterMode(int currentMode){
	#ifdef ASSERT
	assert(m_switches.count(currentMode)>0);
	#endif

	return m_switches[currentMode];
}

void SwitchMan::addNextMasterMode(int a,int b){
	#ifdef ASSERT
	assert(m_switches.count(a)==0);
	#endif

	m_switches[a]=b;
}

void SwitchMan::openSlaveMode(RayMPITag tag,StaticVector*outbox,Rank source,Rank destination){
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
void SwitchMan::closeSlaveModeLocally(StaticVector*outbox,int*slaveMode,int source){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Closing locally slave mode on rank "<<source<<endl;
	#endif

	sendEmptyMessage(outbox,source,MASTER_RANK,RAY_MPI_TAG_SWITCH_MAN_SIGNAL);

	(*slaveMode)=RAY_SLAVE_MODE_DO_NOTHING;
}

void SwitchMan::openMasterMode(RayMPITag tag,StaticVector*outbox,Rank source){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Opening master mode on rank "<<source<<endl;
	cout<<"tag= "<<MESSAGES[tag]<<" source= "<<source<<" Outbox= "<<outbox<<endl;
	#endif

	#ifdef ASSERT
	assert(source==MASTER_RANK);
	assert(outbox!=NULL);
	#endif


	for(int i=0;i<m_size;i++){
		openSlaveMode(tag,outbox,source,i);
	}

	reset();
}

void SwitchMan::closeMasterMode(int*masterMode){

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Closing master mode on rank "<<MASTER_RANK<<endl;
	#endif

	int currentMasterMode=*masterMode;
	int nextMode=getNextMasterMode(currentMasterMode);

	(*masterMode)=nextMode;
}

void SwitchMan::sendEmptyMessage(StaticVector*outbox,Rank source,Rank destination,RayMPITag tag){
	Message aMessage(NULL,0,destination,tag,source);
	outbox->push_back(aMessage);
}

void SwitchMan::openSlaveModeLocally(int tag,int*slaveMode,int rank){
	if(m_tagToSlaveModeTable.count(tag)==0)
		return;

	#ifdef CONFIG_SWITCHMAN_VERBOSITY
	cout<<"Opening locally slave mode on rank "<<rank<<endl;
	#endif

	int desiredSlaveMode=m_tagToSlaveModeTable[tag];

	*slaveMode=desiredSlaveMode;
}

void SwitchMan::addSlaveSwitch(int tag,int slaveMode){
	#ifdef ASSERT
	assert(m_tagToSlaveModeTable.count(tag)==0);
	#endif

	m_tagToSlaveModeTable[tag]=slaveMode;
}
