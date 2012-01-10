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
#include <core/slave_modes.h>
#include <core/master_modes.h>
using namespace std;

/**
 * the switchman controls the workflow on all ranks
 * he is the one who decides when something must be done.
 * \author Sébastien Boisvert
 * \date 2012-01-02
 */
class SwitchMan{

	RaySlaveMode m_slaveMode;
	RayMasterMode m_masterMode;

	/** number of cores */
	int m_size;
	int m_counter;

/** a list of switches describing the order of the master modes */
	map<RayMasterMode,RayMasterMode> m_switches;

/** a table to convert a tag to a slave mode */
	map<Tag,RaySlaveMode> m_tagToSlaveModeTable;

	map<RayMasterMode,Tag> m_masterModeToTagTable;

/** run some assertions */
	void runAssertions();

public:
	void constructor(int numberOfRanks);
	void reset();
	bool allRanksAreReady();

	RayMasterMode getNextMasterMode(RayMasterMode currentMode);

	void addNextMasterMode(RayMasterMode a,RayMasterMode b);

	void addSlaveSwitch(Tag tag,RaySlaveMode slaveMode);

	void addMasterSwitch(RayMasterMode masterMode,Tag tag);

	/** remotely open a slave mode */
	void openSlaveMode(Tag tag,StaticVector*outbox,Rank source,Rank destination);

	/** open a slave mode locally */
	void openSlaveModeLocally(Tag tag,Rank rank);

	/** close a slave mode remotely */
	void closeSlaveMode(Rank source);

	/** locally close a slave mode */
	void closeSlaveModeLocally(StaticVector*outbox,Rank source);

	/** begin a master mode */
	// TODO: add an option SERIAL or PARALLEL */
	void openMasterMode(StaticVector*outbox,Rank source);

	/** close a master mode */
	void closeMasterMode();

	/** send an empty message */
	void sendEmptyMessage(StaticVector*outbox,Rank source,Rank destination,Tag tag);
	void sendMessage(uint64_t*buffer,int count,StaticVector*outbox,Rank source,Rank destination,Tag tag);

	RaySlaveMode getSlaveMode();
	int*getSlaveModePointer();
	void setSlaveMode(RaySlaveMode mode);

	RayMasterMode getMasterMode();
	int*getMasterModePointer();
	void setMasterMode(RayMasterMode mode);

	void sendToAll(StaticVector*outbox,Rank source,Tag tag);
	void sendMessageToAll(uint64_t*buffer,int count,StaticVector*outbox,Rank source,Tag tag);
};

#endif
