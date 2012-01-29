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

#include <plugins/RegisteredPlugin.h>
#include <core/slave_modes.h>
#include <core/master_modes.h>
#include <communication/mpi_tags.h>

#include <iostream>
using namespace std;

RegisteredPlugin::RegisteredPlugin(){
	m_name="No name available";
	m_description="No description available";
	m_authors="No authors available";
	m_license="No license available";
}

void RegisteredPlugin::setPluginName(const char*name){
	m_name=name;
}

void RegisteredPlugin::addAllocatedSlaveMode(SlaveMode slaveMode){
	m_allocatedSlaveModes.insert(slaveMode);
}

void RegisteredPlugin::addAllocatedMasterMode(MasterMode masterMode){
	m_allocatedMasterModes.insert(masterMode);
}

void RegisteredPlugin::addAllocatedMessageTag(MessageTag messageTag){
	m_allocatedMessageTags.insert(messageTag);
}

void RegisteredPlugin::addRegisteredSlaveModeHandler(SlaveMode slaveMode){
	m_registeredSlaveModeHandlers.insert(slaveMode);
}

void RegisteredPlugin::addRegisteredMasterModeHandler(MasterMode masterMode){
	m_registeredMasterModeHandlers.insert(masterMode);
}

void RegisteredPlugin::addRegisteredMessageTagHandler(MessageTag messageTag){
	m_registeredMessageTagHandlers.insert(messageTag);
}

void RegisteredPlugin::addRegisteredSlaveModeSymbol(SlaveMode slaveMode){
	m_registeredSlaveModeSymbols.insert(slaveMode);
}

void RegisteredPlugin::addRegisteredMasterModeSymbol(MasterMode masterMode){
	m_registeredMasterModeSymbols.insert(masterMode);
}

void RegisteredPlugin::addRegisteredMessageTagSymbol(MessageTag messageTag){
	m_registeredMessageTagSymbols.insert(messageTag);
}

bool RegisteredPlugin::hasSlaveMode(SlaveMode mode){
	return m_allocatedSlaveModes.count(mode)>0;
}

bool RegisteredPlugin::hasMasterMode(MasterMode mode){
	return m_allocatedMasterModes.count(mode)>0;
}

bool RegisteredPlugin::hasMessageTag(MessageTag tag){
	return m_allocatedMessageTags.count(tag)>0;
}

string RegisteredPlugin::getPluginName(){
	return m_name;
}

void RegisteredPlugin::print(ostream*stream){

	(*stream)<<"Name: "<<getPluginName()<<endl;
	(*stream)<<"Description: "<<getPluginDescription()<<endl;
	(*stream)<<"Authors: "<<getPluginAuthors()<<endl;
	(*stream)<<"License: "<<getPluginLicense()<<endl;
	(*stream)<<endl;
	
	(*stream)<<"Allocated handles"<<endl;
	(*stream)<<endl;

	(*stream)<<"-> Allocated MasterMode handles: "<<m_allocatedMasterModes.size()<<endl;
	(*stream)<<"   -> with a registered symbol: "<<m_registeredMasterModeSymbols.size()<<endl;
	(*stream)<<"   -> with a registered MasterModeHandler: "<<m_registeredMasterModeHandlers.size()<<endl;
	(*stream)<<"   -> with a registered MasterMode-to-MessageTag switch: "<<m_registeredMasterModeToMessageTagSwitches.size()<<endl;
	(*stream)<<"   -> with a registered next MasterMode: "<<m_registeredMasterModeNextMasterModes.size()<<endl;
	(*stream)<<"   -> registered as first MasterMode: "<<m_registeredFirstMasterModes.size()<<endl;
	(*stream)<<endl;

	(*stream)<<"-> Allocated SlaveMode handles: "<<m_allocatedSlaveModes.size()<<endl;
	(*stream)<<"   -> with a registered symbol: "<<m_registeredSlaveModeSymbols.size()<<endl;
	(*stream)<<"   -> with a registered SlaveModeHandler: "<<m_registeredSlaveModeHandlers.size()<<endl;
	(*stream)<<endl;

	(*stream)<<"-> Allocated MessageTag handles: "<<m_allocatedMessageTags.size()<<endl;
	(*stream)<<"   -> with a registered symbol: "<<m_registeredMessageTagSymbols.size()<<endl;
	(*stream)<<"   -> with a registered MessageTagHandler: "<<m_registeredMessageTagHandlers.size()<<endl;
	(*stream)<<"   -> with a registered MessageTag-to-SlaveMode switch: "<<m_registeredMessageTagToSlaveModeSwitches.size()<<endl;
	(*stream)<<"   -> with a registered reply MessageTag: "<<m_registeredMessageTagReplyTags.size()<<endl;
	(*stream)<<"   -> with a registered size: "<<m_registeredMessageTagSizes.size()<<endl;
	(*stream)<<endl;

	(*stream)<<"Resolved symbols"<<endl;

	(*stream)<<endl;
	(*stream)<<"-> Resolved MasterMode symbols: "<<m_resolvedMasterModes.size()<<endl;
	(*stream)<<"-> Resolved SlaveMode symbols: "<<m_resolvedSlaveModes.size()<<endl;
	(*stream)<<"-> Resolved MessageTag symbols: "<<m_resolvedMessageTags.size()<<endl;

	(*stream)<<endl;
}

void RegisteredPlugin::addResolvedMessageTag(MessageTag handle){
	m_resolvedMessageTags.insert(handle);
}

void RegisteredPlugin::addResolvedSlaveMode(SlaveMode handle){
	m_resolvedSlaveModes.insert(handle);
}

void RegisteredPlugin::addResolvedMasterMode(MasterMode handle){
	m_resolvedMasterModes.insert(handle);
}

void RegisteredPlugin::setPluginDescription(const char*a){
	m_description=a;
}

void RegisteredPlugin::setPluginAuthors(const char*a){
	m_authors=a;
}

void RegisteredPlugin::setPluginLicense(const char*a){
	m_license=a;
}

string RegisteredPlugin::getPluginDescription(){
	return m_description;
}

string RegisteredPlugin::getPluginLicense(){
	return m_license;
}

string RegisteredPlugin::getPluginAuthors(){
	return m_authors;
}

void RegisteredPlugin::addRegisteredMasterModeToMessageTagSwitch(MasterMode mode){
	m_registeredMasterModeToMessageTagSwitches.insert(mode);
}

void RegisteredPlugin::addRegisteredMessageTagToSlaveModeSwitch(SlaveMode mode){
	m_registeredMessageTagToSlaveModeSwitches.insert(mode);
}

void RegisteredPlugin::addRegisteredMessageTagReplyMessageTag(MessageTag tag){
	m_registeredMessageTagReplyMessageTags.insert(tag);
}

void RegisteredPlugin::addRegisteredMessageTagSize(MessageTag tag){
	m_registeredMessageTagSizes.insert(tag);
}

void RegisteredPlugin::addRegisteredMasterModeNextMasterMode(MasterMode mode){
	m_registeredMasterModeNextMasterModes.insert(mode);
}

void RegisteredPlugin::addRegisteredFirstMasterMode(MasterMode mode){
	m_registeredFirstMasterModes.insert(mode);
}
