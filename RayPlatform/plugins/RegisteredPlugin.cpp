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
#include <iostream>
using namespace std;

RegisteredPlugin::RegisteredPlugin(){
	m_name="UnnamedPlugin";
}

void RegisteredPlugin::setName(string name){
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

string RegisteredPlugin::getName(){
	return m_name;
}

void RegisteredPlugin::print(){

	cout<<" Name: "<<getName()<<endl;

	cout<<" Allocated master mode handles: "<<m_allocatedMasterModes.size()<<endl;
	cout<<"   Handles with a registered handler: "<<m_registeredMasterModeHandlers.size()<<endl;
	cout<<"   Handles with a registered symbol: "<<m_registeredMasterModeSymbols.size()<<endl;

	cout<<" Allocated slave mode handles: "<<m_allocatedSlaveModes.size()<<endl;
	cout<<"   Handles with a registered handler: "<<m_registeredSlaveModeHandlers.size()<<endl;
	cout<<"   Handles with a registered symbol: "<<m_registeredSlaveModeSymbols.size()<<endl;

	cout<<" Allocated message tag handles: "<<m_allocatedMessageTags.size()<<endl;
	cout<<"   Handles with a registered handler: "<<m_registeredMessageTagHandlers.size()<<endl;
	cout<<"   Handles with a registered symbol: "<<m_registeredMessageTagSymbols.size()<<endl;

}
