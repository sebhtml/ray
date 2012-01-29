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

#ifndef _SwitchMan_H
#define _SwitchMan_H

#include <core/master_modes.h>
#include <structures/StaticVector.h>
#include <communication/Message.h>
#include <core/slave_modes.h>
#include <core/master_modes.h>
#include <plugins/CorePlugin.h>
#include <handlers/MessageTagHandler.h>
#include <scheduling/SwitchMan_adapters.h>

#include <map>
#include <vector>
using namespace std;

class ComputeCore;

/**
 * the switchman controls the workflow on all ranks
 * he is the one who decides when something must be done.
 * \author Sébastien Boisvert
 * \date 2012-01-02
 */
class SwitchMan: public CorePlugin {

	Adapter_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL m_adapter_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL ;

	MessageTag RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON;
	MessageTag RAY_MPI_TAG_GOOD_JOB_SEE_YOU_SOON_REPLY;
	MessageTag RAY_MPI_TAG_DUMMY;
	MessageTag RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL;

	MasterMode RAY_MASTER_MODE_DO_NOTHING;
	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;
	SlaveMode RAY_SLAVE_MODE_DIE;

/** the current slave mode of the rank */
	SlaveMode m_slaveMode;

/** the current master mode of the rank */
	MasterMode m_masterMode;
	
	MasterMode m_lastMasterMode;

/** the rank **/
	Rank m_rank;

	/** number of cores */
	int m_size;

/** a counter to check progression */
	int m_counter;

/** a list of switches describing the order of the master modes */
	map<MasterMode,MasterMode> m_switches;

/** a table to convert a tag to a slave mode */
	map<MessageTag,SlaveMode> m_tagToSlaveModeTable;

/** a table containing mapping from master modes to MPI tags */
	map<MasterMode,MessageTag> m_masterModeToTagTable;

/** run some assertions */
	void runAssertions();

public:

/** the switchman is constructed with a number of cores */
	void constructor(int rank,int numberOfRanks);

/** reset */
	void reset();

/** returns true if all ranks finished their current slave mode */
	bool allRanksAreReady();

/** according to master_mode_order.txt, returns
 * the next master mode to set */
	MasterMode getNextMasterMode(MasterMode currentMode);

/**
 * add a master mode associated to another */
	void addNextMasterMode(MasterMode a,MasterMode b);

/** add a slave switch */
	void addSlaveSwitch(MessageTag tag,SlaveMode slaveMode);

/** add a master switch */
	void addMasterSwitch(MasterMode masterMode,MessageTag tag);

	/** remotely open a slave mode */
	void openSlaveMode(MessageTag tag,StaticVector*outbox,Rank source,Rank destination);

	/** open a slave mode locally */
	void openSlaveModeLocally(MessageTag tag,Rank rank);

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
	void sendEmptyMessage(StaticVector*outbox,Rank source,Rank destination,MessageTag tag);

/** sends a message with a full list of parameters */
	void sendMessage(uint64_t*buffer,int count,StaticVector*outbox,Rank source,Rank destination,MessageTag tag);

/** get the current slave mode */
	SlaveMode getSlaveMode();

/** get the current slave mode pointer */
	int*getSlaveModePointer();

/** change the slave mode manually */
	void setSlaveMode(SlaveMode mode);

/** get the master mode */
	MasterMode getMasterMode();

/** get the master mode pointer */
	int*getMasterModePointer();

/** changes the master mode manually */
	void setMasterMode(MasterMode mode);

/** send a message to all MPI ranks */
	void sendToAll(StaticVector*outbox,Rank source,MessageTag tag);

/** send a message to all MPI ranks, possibly with data */
	void sendMessageToAll(uint64_t*buffer,int count,StaticVector*outbox,Rank source,MessageTag tag);

	void addMasterMode(MasterMode masterMode);

	vector<MasterMode>*getMasterModeOrder();

	Rank getRank();

	int getSize();

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

	void addDummy(ComputeCore*core);

	void call_RAY_MPI_TAG_SWITCHMAN_COMPLETION_SIGNAL(Message*message);
};

#endif
