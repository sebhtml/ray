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
*/

#include <plugin_PhylogenyViewer/PhylogenyViewer.h>


void PhylogenyViewer::call_RAY_MASTER_MODE_PHYLOGENY_MAIN(){
	if(!m_started){

		#ifdef DEBUG_PHYLOGENY
		cout<<"Opening call_RAY_MASTER_MODE_PHYLOGENY_MAIN"<<endl;
		#endif

		m_switchMan->openMasterMode(m_outbox,m_rank);

		m_started=true;

	}else if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();
	}
}

void PhylogenyViewer::call_RAY_SLAVE_MODE_PHYLOGENY_MAIN(){

	if(m_outbox->size()==0){
		#ifdef DEBUG_PHYLOGENY
		cout<<"Rank "<<m_rank<<" is closing call_RAY_SLAVE_MODE_PHYLOGENY_MAIN"<<endl;
		#endif

		m_switchMan->closeSlaveModeLocally(m_outbox,m_rank);
	}
}


void PhylogenyViewer::registerPlugin(ComputeCore*core){

	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"PhylogenyViewer");
	core->setPluginDescription(m_plugin,"Get a phylogeny view on the sample.");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_PHYLOGENY_MAIN=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,"RAY_MASTER_MODE_PHYLOGENY_MAIN");
	m_adapter_RAY_MASTER_MODE_PHYLOGENY_MAIN.setObject(this);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,&m_adapter_RAY_MASTER_MODE_PHYLOGENY_MAIN);

	RAY_SLAVE_MODE_PHYLOGENY_MAIN=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_PHYLOGENY_MAIN,"RAY_SLAVE_MODE_PHYLOGENY_MAIN");
	m_adapter_RAY_SLAVE_MODE_PHYLOGENY_MAIN.setObject(this);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_PHYLOGENY_MAIN,&m_adapter_RAY_SLAVE_MODE_PHYLOGENY_MAIN);

	RAY_MPI_TAG_PHYLOGENY_MAIN=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_PHYLOGENY_MAIN,"RAY_MPI_TAG_PHYLOGENY_MAIN");
}

void PhylogenyViewer::resolveSymbols(ComputeCore*core){


	RAY_MASTER_MODE_PHYLOGENY_MAIN=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PHYLOGENY_MAIN");
	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");

	RAY_MPI_TAG_PHYLOGENY_MAIN=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PHYLOGENY_MAIN");

	RAY_SLAVE_MODE_PHYLOGENY_MAIN=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_PHYLOGENY_MAIN");

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,RAY_MPI_TAG_PHYLOGENY_MAIN);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_PHYLOGENY_MAIN,RAY_SLAVE_MODE_PHYLOGENY_MAIN);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,RAY_MASTER_MODE_KILL_RANKS);

	m_switchMan=core->getSwitchMan();
	m_outbox=core->getOutbox();
	m_inbox=core->getInbox();
	m_outboxAllocator=core->getOutboxAllocator();
	m_inboxAllocator=core->getInboxAllocator();

	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");

	m_rank=core->getMessagesHandler()->getRank();
	m_size=core->getMessagesHandler()->getSize();

	m_core=core;

}

