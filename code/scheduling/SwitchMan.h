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

#ifndef _SwitchMan_H
#define _SwitchMan_H

#include <map>
#include <core/master_modes.h>
#include <structures/StaticVector.h>
#include <communication/Message.h>
using namespace std;

/**
 * the switchman controls the workflow on all ranks
 * he is the one who decides when something must be done.
 * \author Sébastien Boisvert
 * \date 2012-01-02
 */
class SwitchMan{
	/** number of cores */
	int m_size;
	int m_counter;

/** a list of switches describing the order of the master modes */
	map<int,int> m_switches;

/** a table to convert a tag to a slave mode */
	map<int,int> m_tagToSlaveModeTable;

/** run some assertions */
	void runAssertions();

public:
	void constructor(int numberOfRanks);
	void reset();
	bool allRanksAreReady();

	int getNextMasterMode(int currentMode);

	void addNextMasterMode(int a,int b);

	void addSlaveSwitch(int tag,int slaveMode);

	/** remotely open a slave mode */
	void openSlaveMode(RayMPITag tag,StaticVector*outbox,Rank source,Rank destination);

	/** open a slave mode locally */
	void openSlaveModeLocally(int tag,int*slaveMode,int rank);

	/** close a slave mode remotely */
	void closeSlaveMode(int source);

	/** locally close a slave mode */
	void closeSlaveModeLocally(StaticVector*outbox,int*slaveMode,Rank source);

	/** begin a master mode */
	void openMasterMode(RayMPITag tag,StaticVector*outbox,Rank source);

	/** close a master mode */
	void closeMasterMode(int*masterMode);

	/** send an empty message */
	void sendEmptyMessage(StaticVector*outbox,Rank source,Rank destination,RayMPITag tag);
};

#endif
