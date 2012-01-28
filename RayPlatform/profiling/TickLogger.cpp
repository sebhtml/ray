/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

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

#include <profiling/TickLogger.h>
#include <assert.h>
#include <iostream>
using namespace std;

//#define CONFIG_DISPLAY_TICKS
#define CONFIG_TICK_STEPPING 1000000

void TickLogger::logSlaveTick(SlaveMode i){
	#ifdef ASSERT
	assert(i!=INVALID_HANDLE);
	#endif

	// this is the same as the last one
	if(i==m_lastSlaveMode){
		m_slaveCount++;

	// this case occurs once during the first call
	}else if(m_lastSlaveMode==INVALID_HANDLE){
		// start the new entry
		m_lastSlaveMode=i;
		m_slaveCount=1;

	// this is a new slave mode
	}else{
		// add the last entry
		m_slaveModes.push_back(m_lastSlaveMode);
		m_slaveCounts.push_back(m_slaveCount);

		// start the new entry
		m_lastSlaveMode=i;
		m_slaveCount=1;
	}

	#ifdef CONFIG_DISPLAY_TICKS
	if(m_slaveCount% CONFIG_TICK_STEPPING == 0)
		cout<<"[TickLogger::addSlaveTick] "<<SLAVE_MODES[i]<<" "<<m_slaveCount<<endl;
	#endif
}

void TickLogger::logMasterTick(MasterMode i){
	#ifdef ASSERT
	assert(i!=INVALID_HANDLE);
	#endif

	// this is the same as the last one
	if(i==m_lastMasterMode){
		m_masterCount++;

	// this case occurs once during the first call
	}else if(m_lastMasterMode==INVALID_HANDLE){
		// start the new entry
		m_lastMasterMode=i;
		m_masterCount=1;

	// this is a new slave mode
	}else{

		// add the last entry
		m_masterModes.push_back(m_lastMasterMode);
		m_masterCounts.push_back(m_masterCount);

		// start the new entry
		m_lastMasterMode=i;
		m_masterCount=1;
	}

	#ifdef CONFIG_DISPLAY_TICKS
	if(m_masterCount% CONFIG_TICK_STEPPING == 0)
		cout<<"[TickLogger::addMasterTick] "<<MASTER_MODES[i]<<" "<<m_masterCount<<endl;
	#endif

}

void TickLogger::printSlaveTicks(ofstream*file){
	// add the last entry
	m_slaveModes.push_back(m_lastSlaveMode);
	m_slaveCounts.push_back(m_slaveCount);
	
	// reset the entry
	m_lastSlaveMode=INVALID_HANDLE;
	m_slaveCount=0;

	// print stuff
	
	#ifdef ASSERT
	assert(m_slaveModes.size()==m_slaveCounts.size());
	#endif

	uint64_t total=0;

	for(int i=0;i<(int)m_slaveModes.size();i++){
		total+=m_slaveCounts[i];
	}

	(*file)<<"Index	Slave mode	Number of ticks	Ratio"<<endl;
	for(int i=0;i<(int)m_slaveModes.size();i++){
		double ratio=m_slaveCounts[i];
		if(total!=0)
			ratio/=total;

		(*file)<<i<<"	"<<SLAVE_MODES[m_slaveModes[i]]<<"	"<<m_slaveCounts[i];
		//<<"	"<<ratio<<endl;
		(*file)<<endl;
	}

	m_slaveModes.clear();
	m_slaveCounts.clear();
}

void TickLogger::printMasterTicks(ofstream*file){

	// add the last entry
	m_masterModes.push_back(m_lastMasterMode);
	m_masterCounts.push_back(m_masterCount);

	// reset the entry
	m_lastMasterMode=INVALID_HANDLE;
	m_masterCount=0;

	// print stuff
	
	#ifdef ASSERT
	assert(m_masterModes.size()==m_masterCounts.size());
	#endif
	
	(*file)<<"Index	Master mode	Number of ticks"<<endl;

	uint64_t total=0;

	for(int i=0;i<(int)m_masterModes.size();i++){
		total+=m_masterCounts[i];
	}

	for(int i=0;i<(int)m_masterModes.size();i++){
		double ratio=m_masterCounts[i];
		if(total!=0)
			ratio/=total;

		(*file)<<i<<"	"<<MASTER_MODES[m_masterModes[i]]<<"	"<<m_masterCounts[i]<<endl;
	}

	m_masterModes.clear();
	m_masterCounts.clear();
}

TickLogger::TickLogger(){
	m_lastSlaveMode=INVALID_HANDLE;
	m_lastMasterMode=INVALID_HANDLE;
	m_slaveCount=0;
	m_masterCount=0;
}
