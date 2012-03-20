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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

\file GenomeNeighbourhood.h
\author Sébastien Boisvert
*/

#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood.h>

void GenomeNeighbourhood::call_RAY_MASTER_MODE_NEIGHBOURHOOD(){

	if(!m_started){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getMessagesHandler()->getRank());

		m_started=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		m_timePrinter->printElapsedTime("Computing neighbourhoods");

		m_core->getSwitchMan()->closeMasterMode();
	}
}

void GenomeNeighbourhood::call_RAY_SLAVE_MODE_NEIGHBOURHOOD(){

	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getMessagesHandler()->getRank());
}

/**
 * register the plugin
 * */
void GenomeNeighbourhood::registerPlugin(ComputeCore*core){

	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"GenomeNeighbourhood");
	core->setPluginDescription(m_plugin,"Get a sophisticated bird's-eye view of a sample's DNA");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3 (GPLv3)");

	// register handles
	
	RAY_MASTER_MODE_NEIGHBOURHOOD=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,"RAY_MASTER_MODE_NEIGHBOURHOOD");
	m_adapter_RAY_MASTER_MODE_NEIGHBOURHOOD.setObject(this);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,&m_adapter_RAY_MASTER_MODE_NEIGHBOURHOOD);

	RAY_SLAVE_MODE_NEIGHBOURHOOD=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_NEIGHBOURHOOD,"RAY_SLAVE_MODE_NEIGHBOURHOOD");
	m_adapter_RAY_SLAVE_MODE_NEIGHBOURHOOD.setObject(this);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_NEIGHBOURHOOD,&m_adapter_RAY_SLAVE_MODE_NEIGHBOURHOOD);

	RAY_MPI_TAG_NEIGHBOURHOOD=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD,"RAY_MPI_TAG_NEIGHBOURHOOD");
}

/**
 * resolve symbols
 */
void GenomeNeighbourhood::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");
	RAY_MASTER_MODE_NEIGHBOURHOOD=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_NEIGHBOURHOOD");
	RAY_SLAVE_MODE_NEIGHBOURHOOD=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_NEIGHBOURHOOD");

	RAY_MPI_TAG_NEIGHBOURHOOD=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_NEIGHBOURHOOD");

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,RAY_MPI_TAG_NEIGHBOURHOOD);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD,RAY_SLAVE_MODE_NEIGHBOURHOOD);

	/* this is done here because we need symbols */

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,RAY_MASTER_MODE_KILL_RANKS);

	m_started=false;
	m_timePrinter=(TimePrinter*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Timer.ray");

	m_core=core;
}

