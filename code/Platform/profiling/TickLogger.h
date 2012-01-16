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

#ifndef _TickLogger_h
#define _TickLogger_h

#include <stdint.h>
#include <vector>
#include <core/master_modes.h>
#include <core/slave_modes.h>
#include <fstream>
using namespace std;

/** this class writes these files:
 *
 * Scheduling/Agenda.txt
 * Scheduling/SlaveSwitches.txt
 * Scheduling/MasterSwitches.txt
 * Scheduling/SlaveModes.txt
 * Scheduling/MasterModes.txt
 *
 * Scheduling/0.MasterTicks.txt
 * Scheduling/0.SlaveTicks.txt
 *
 * ...
 *
 * To do so, it counts the ticks
 */
class TickLogger{
	vector<uint64_t> m_slaveCounts;
	vector<SlaveMode> m_slaveModes;
	SlaveMode m_lastSlaveMode;
	uint64_t m_slaveCount;

	vector<uint64_t> m_masterCounts;
	vector<MasterMode> m_masterModes;
	MasterMode m_lastMasterMode;
	uint64_t m_masterCount;
public:

	TickLogger();

	void logSlaveTick(SlaveMode i);
	void logMasterTick(MasterMode i);

	void printSlaveTicks(ofstream*file);
	void printMasterTicks(ofstream*file);
};

#endif
