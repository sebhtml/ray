/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _RegisteredPlugin_h
#define _RegisteredPlugin_h

#include <core/types.h>
#include <set>
#include <string>
using namespace std;

/**
 * a registered plugin
 */
class RegisteredPlugin{

	string m_name;

	set<SlaveMode> m_allocatedSlaveModes;
	set<MasterMode> m_allocatedMasterModes;
	set<MessageTag> m_allocatedMessageTags;

	set<SlaveMode> m_registeredSlaveModeHandlers;
	set<MasterMode> m_registeredMasterModeHandlers;
	set<MessageTag> m_registeredMessageTagHandlers;

	set<SlaveMode> m_registeredSlaveModeSymbols;
	set<MasterMode> m_registeredMasterModeSymbols;
	set<MessageTag> m_registeredMessageTagSymbols;

public:

	RegisteredPlugin();

	void setName(string name);
	string getName();

	void addAllocatedSlaveMode(SlaveMode slaveMode);
	void addAllocatedMasterMode(MasterMode masterMode);
	void addAllocatedMessageTag(MessageTag messageTag);

	void addRegisteredSlaveModeHandler(SlaveMode slaveMode);
	void addRegisteredMasterModeHandler(MasterMode masterMode);
	void addRegisteredMessageTagHandler(MessageTag messageTag);

	void addRegisteredSlaveModeSymbol(SlaveMode slaveMode);
	void addRegisteredMasterModeSymbol(MasterMode masterMode);
	void addRegisteredMessageTagSymbol(MessageTag messageTag);

	bool hasSlaveMode(SlaveMode mode);
	bool hasMasterMode(MasterMode mode);
	bool hasMessageTag(MessageTag tag);

	void print();
};

#endif
