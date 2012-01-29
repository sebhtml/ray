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
#include <iostream>
using namespace std;

/**
 * a registered plugin
 */
class RegisteredPlugin{

	string m_name;
	string m_description;
	string m_authors;
	string m_license;

	set<MasterMode> m_resolvedMasterModes;
	set<SlaveMode> m_resolvedSlaveModes;
	set<MessageTag> m_resolvedMessageTags;

	set<MasterMode> m_allocatedMasterModes;
	set<MasterMode> m_registeredMasterModeSymbols;
	set<MasterMode> m_registeredMasterModeHandlers;
	set<MasterMode> m_registeredMasterModeToMessageTagSwitches;
	set<MasterMode> m_registeredMasterModeNextMasterModes;
	set<MasterMode> m_registeredFirstMasterModes;

	set<SlaveMode> m_allocatedSlaveModes;
	set<SlaveMode> m_registeredSlaveModeSymbols;
	set<SlaveMode> m_registeredSlaveModeHandlers;

	set<MessageTag> m_allocatedMessageTags;
	set<MessageTag> m_registeredMessageTagSymbols;
	set<MessageTag> m_registeredMessageTagHandlers;
	set<MessageTag> m_registeredMessageTagReplyTags;
	set<MessageTag> m_registeredMessageTagToSlaveModeSwitches;
	set<MessageTag> m_registeredMessageTagReplyMessageTags;
	set<MessageTag> m_registeredMessageTagSizes;

public:

	RegisteredPlugin();

	void setPluginName(const char*name);
	void setPluginDescription(const char*text);
	void setPluginAuthors(const char*text);
	void setPluginLicense(const char*text);

	string getPluginName();
	string getPluginDescription();
	string getPluginLicense();
	string getPluginAuthors();

	void addRegisteredMasterModeSymbol(MasterMode masterMode);
	void addAllocatedMasterMode(MasterMode masterMode);
	void addRegisteredMasterModeHandler(MasterMode masterMode);
	void addResolvedMasterMode(MasterMode masterMode);
	void addRegisteredMasterModeToMessageTagSwitch(MasterMode mode);
	void addRegisteredMasterModeNextMasterMode(MasterMode mode);
	void addRegisteredFirstMasterMode(MasterMode mode);

	void addAllocatedSlaveMode(SlaveMode slaveMode);
	void addRegisteredSlaveModeHandler(SlaveMode slaveMode);
	void addRegisteredSlaveModeSymbol(SlaveMode slaveMode);
	void addResolvedSlaveMode(SlaveMode handle);

	void addAllocatedMessageTag(MessageTag messageTag);
	void addRegisteredMessageTagHandler(MessageTag messageTag);
	void addRegisteredMessageTagSymbol(MessageTag messageTag);
	void addResolvedMessageTag(MessageTag handle);
	void addRegisteredMessageTagToSlaveModeSwitch(MessageTag handle);
	void addRegisteredMessageTagReplyMessageTag(MessageTag tag);
	void addRegisteredMessageTagSize(MessageTag tag);

	bool hasSlaveMode(SlaveMode mode);
	bool hasMasterMode(MasterMode mode);
	bool hasMessageTag(MessageTag tag);

	void print(ostream*stream);
};

#endif
