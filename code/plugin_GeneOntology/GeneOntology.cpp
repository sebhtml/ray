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

//#define DEBUG_RECURSION

#include <plugin_GeneOntology/GeneOntology.h>
#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <core/OperatingSystem.h>

//#define DEBUG_PHYLOGENY

void GeneOntology::call_RAY_MASTER_MODE_ONTOLOGY_MAIN(){
	if(!m_started){

		m_switchMan->openMasterMode(m_outbox,m_rank);

		m_started=true;


	}else if(m_switchMan->allRanksAreReady()){

		m_timePrinter->printElapsedTime("Processing gene ontologies");

		m_switchMan->closeMasterMode();
	}
}

void GeneOntology::fetchRelevantColors(){

	GridTableIterator iterator;
	iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	//* only fetch half of the iterated things because we just need one k-mer
	// for any pair of reverse-complement k-mers 
	
	int parity=0;

	while(iterator.hasNext()){

		Vertex*node=iterator.next();
		Kmer key=*(iterator.getKey());

		#ifdef ASSERT
		assert(parity==0 || parity==1);
		#endif

		if(parity==0){
			parity=1;
		}else if(parity==1){
			parity=0;

			continue; // we only need data with parity=0
		}

		VirtualKmerColorHandle color=node->getVirtualColor();
		set<PhysicalKmerColor>*physicalColors=m_colorSet->getPhysicalColors(color);

		for(set<PhysicalKmerColor>::iterator j=physicalColors->begin();
			j!=physicalColors->end();j++){

			PhysicalKmerColor physicalColor=*j;
	
			uint64_t nameSpace=physicalColor/COLOR_NAMESPACE_MULTIPLIER;
		
			if(nameSpace==COLOR_NAMESPACE_EMBL_CDS){
				PhysicalKmerColor colorForPhylogeny=physicalColor % COLOR_NAMESPACE_MULTIPLIER;

				m_colorsForOntology.insert(colorForPhylogeny);

			}
		}
	}
		
	cout<<"Rank "<<m_rank<<" has exactly "<<m_colorsForOntology.size()<<" k-mer physical colors related to EMBL CDS objects."<<endl;
	cout<<endl;


	m_listedRelevantColors=true;

}

void GeneOntology::call_RAY_SLAVE_MODE_ONTOLOGY_MAIN(){

	if(!m_listedRelevantColors){

		fetchRelevantColors();

	}else{
		m_switchMan->closeSlaveModeLocally(m_outbox,m_rank);
	}
}


void GeneOntology::registerPlugin(ComputeCore*core){

	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"GeneOntology");
	core->setPluginDescription(m_plugin,"Get a ontology view on the sample.");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3");

	RAY_MASTER_MODE_ONTOLOGY_MAIN=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,"RAY_MASTER_MODE_ONTOLOGY_MAIN");
	m_adapter_RAY_MASTER_MODE_ONTOLOGY_MAIN.setObject(this);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,&m_adapter_RAY_MASTER_MODE_ONTOLOGY_MAIN);

	RAY_SLAVE_MODE_ONTOLOGY_MAIN=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_ONTOLOGY_MAIN,"RAY_SLAVE_MODE_ONTOLOGY_MAIN");
	m_adapter_RAY_SLAVE_MODE_ONTOLOGY_MAIN.setObject(this);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_ONTOLOGY_MAIN,&m_adapter_RAY_SLAVE_MODE_ONTOLOGY_MAIN);

	RAY_MPI_TAG_ONTOLOGY_MAIN=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_ONTOLOGY_MAIN,"RAY_MPI_TAG_ONTOLOGY_MAIN");

	m_switchMan=core->getSwitchMan();
	m_outbox=core->getOutbox();
	m_inbox=core->getInbox();
	m_outboxAllocator=core->getOutboxAllocator();
	m_inboxAllocator=core->getInboxAllocator();

	m_rank=core->getMessagesHandler()->getRank();
	m_size=core->getMessagesHandler()->getSize();

	m_core=core;
}

void GeneOntology::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_ONTOLOGY_MAIN=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_ONTOLOGY_MAIN");
	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");
	RAY_MASTER_MODE_NEIGHBOURHOOD=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_NEIGHBOURHOOD");

	RAY_MPI_TAG_ONTOLOGY_MAIN=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ONTOLOGY_MAIN");
	
	RAY_SLAVE_MODE_ONTOLOGY_MAIN=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_ONTOLOGY_MAIN");

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,RAY_MPI_TAG_ONTOLOGY_MAIN);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_ONTOLOGY_MAIN,RAY_SLAVE_MODE_ONTOLOGY_MAIN);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,RAY_MASTER_MODE_NEIGHBOURHOOD);

	//core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_ONTOLOGY_MAIN,RAY_MASTER_MODE_KILL_RANKS);

	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");
	m_subgraph=(GridTable*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/deBruijnGraph_part.ray");
	m_colorSet=(ColorSet*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/VirtualColorManagementUnit.ray");
	m_timePrinter=(TimePrinter*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Timer.ray");

	m_searcher=(Searcher*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/plugin_Searcher.ray");

	m_listedRelevantColors=false;

	m_started=false;
}

