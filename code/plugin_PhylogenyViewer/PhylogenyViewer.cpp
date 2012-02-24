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
#include <plugin_VerticesExtractor/GridTableIterator.h>
#include <plugin_PhylogenyViewer/GenomeToTaxonLoader.h>

//#define DEBUG_PHYLOGENY

void PhylogenyViewer::call_RAY_MASTER_MODE_PHYLOGENY_MAIN(){
	if(!m_started){

		#ifdef DEBUG_PHYLOGENY
		cout<<"Opening call_RAY_MASTER_MODE_PHYLOGENY_MAIN"<<endl;
		#endif

		m_switchMan->openMasterMode(m_outbox,m_rank);

		m_started=true;

		m_ranksThatLoadedTaxons=0;

		m_mustSync=false;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_LOADED_TAXONS)){
		
		m_ranksThatLoadedTaxons++;

		#ifdef ASSERT
		assert(m_rank==0);
		#endif

		#ifdef DEBUG_PHYLOGENY
		cout<<"[phylogeny] m_ranksThatLoadedTaxons -> "<<m_ranksThatLoadedTaxons<<endl;
		#endif

		if(m_ranksThatLoadedTaxons==m_size){

			m_mustSync=true;
			m_ranksThatLoadedTaxons=-1;

			copyTaxonsFromSecondaryTable();

			m_taxonIterator=m_taxonsForPhylogeny.begin();

			m_responses=0;
			m_messageSent=false;

			cout<<"Rank "<<m_rank<<" is starting taxon syncing across the compute tribe."<<endl;

			m_timePrinter->printElapsedTime("Loading taxons");
		}

	}else if(m_mustSync){

		sendTaxonsFromMaster();


	}else if(m_switchMan->allRanksAreReady()){

		m_timePrinter->printElapsedTime("Loading tree");

		m_switchMan->closeMasterMode();
	}
}

void PhylogenyViewer::copyTaxonsFromSecondaryTable(){

	for(set<uint64_t>::iterator i=m_taxonsForPhylogenyMaster.begin();
		i!=m_taxonsForPhylogenyMaster.end();i++){

		m_taxonsForPhylogeny.insert(*i);
	}

	#ifdef ASSERT
	assert(m_taxonsForPhylogeny.size() == m_taxonsForPhylogenyMaster.size());
	#endif

	m_taxonsForPhylogenyMaster.clear();
}

void PhylogenyViewer::call_RAY_SLAVE_MODE_PHYLOGENY_MAIN(){
	if(!m_extractedColorsForPhylogeny){

		extractColorsForPhylogeny();

	}else if(!m_loadedTaxonsForPhylogeny){
	
		loadTaxons();

	}else if(!m_sentTaxonsToMaster){

		sendTaxonsToMaster();

	}else if(!m_sentTaxonControlMessage){
		
		#ifdef DEBUG_PHYLOGENY
		cout<<"Sending control message"<<endl;
		#endif

		m_switchMan->sendEmptyMessage(m_outbox,m_rank,
			MASTER_RANK,RAY_MPI_TAG_LOADED_TAXONS);

		m_sentTaxonControlMessage=true;

		m_synced=false;
	
	}else if(m_inbox->hasMessage(RAY_MPI_TAG_SYNCED_TAXONS)){
	
		copyTaxonsFromSecondaryTable();

		cout<<"Rank "<<m_rank<<" has "<<m_taxonsForPhylogeny.size()<<" taxons after syncing with master"<<endl;

		m_synced=true;

	}else if(!m_synced){
	
	}else if(m_outbox->size()==0){

		#ifdef DEBUG_PHYLOGENY
		cout<<"Rank "<<m_rank<<" is closing call_RAY_SLAVE_MODE_PHYLOGENY_MAIN"<<endl;
		#endif

		m_switchMan->closeSlaveModeLocally(m_outbox,m_rank);
	}
}

void PhylogenyViewer::call_RAY_MPI_TAG_TOUCH_TAXON(Message*message){
	uint64_t*buffer=message->getBuffer();

	int count=message->getCount();

	for(int i=0;i<count;i++){
		uint64_t taxon=buffer[i];

		m_taxonsForPhylogenyMaster.insert(taxon);
	}

	m_switchMan->sendEmptyMessage(m_outbox,m_parameters->getRank(),
		message->getSource(),RAY_MPI_TAG_TOUCH_TAXON_REPLY);
}

/*
 * Loads taxons by fetching pairs in a conversion file.
 */
void PhylogenyViewer::loadTaxons(){

	if(!m_parameters->hasOption("-with-phylogeny")){
		m_loadedTaxonsForPhylogeny=true;

		return;
	}

	string genomeToTaxonFile=m_parameters->getGenomeToTaxonFile();

	GenomeToTaxonLoader genomeToTaxonUnit;

	genomeToTaxonUnit.load(genomeToTaxonFile);

	while(genomeToTaxonUnit.hasNext()){
	
		uint64_t genome;
		uint64_t taxon;

		genomeToTaxonUnit.getNext(&genome,&taxon);

		if(m_colorsForPhylogeny.count(genome)>0){
			m_taxonsForPhylogeny.insert(taxon);
			
			m_genomeToTaxon[genome]=taxon;
		}
	}

	cout<<"Rank "<<m_rank<<" loaded "<<m_taxonsForPhylogeny.size()<<" taxons."<<endl;

	m_colorsForPhylogeny.clear();

	m_loadedTaxonsForPhylogeny=true;

	m_sentTaxonsToMaster=false;

	m_taxonIterator=m_taxonsForPhylogeny.begin();

	m_messageSent=false;
	m_messageReceived=true;
}

void PhylogenyViewer::sendTaxonsFromMaster(){

	if(m_responses == m_size && m_taxonIterator==m_taxonsForPhylogeny.end()){

		m_switchMan->sendToAll(m_outbox,m_rank,RAY_MPI_TAG_SYNCED_TAXONS);

		cout<<"Rank "<<m_rank<<" synced taxons across the grid with "<<m_size<<" poor slaves."<<endl;

		m_mustSync=false;

	}else if(!m_messageSent){

		#ifdef ASSERT
		assert(m_taxonIterator!= m_taxonsForPhylogeny.end());
		#endif

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int maximum=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);

		int bufferPosition=0;

		while(bufferPosition<maximum && m_taxonIterator!= m_taxonsForPhylogeny.end()){
			uint64_t taxon=*m_taxonIterator;
			buffer[bufferPosition]=taxon;
			bufferPosition++;
			m_taxonIterator++;
		}
		
		#ifdef ASSERT
		assert(bufferPosition!=0);
		#endif

		m_switchMan->sendMessageToAll(buffer,bufferPosition,m_outbox,m_rank,RAY_MPI_TAG_TOUCH_TAXON);

		m_messageSent=true;
		m_responses=0;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_TOUCH_TAXON_REPLY)){

		m_responses++;

		if(m_responses==m_size){
	
			m_messageSent=false;
		}
	}
}



void PhylogenyViewer::sendTaxonsToMaster(){

	if(m_messageReceived && m_taxonIterator==m_taxonsForPhylogeny.end()){

		m_sentTaxonsToMaster=true;
	
		m_sentTaxonControlMessage=false;

	}else if(!m_messageSent){

		#ifdef ASSERT
		assert(m_taxonIterator!= m_taxonsForPhylogeny.end());
		#endif

		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

		int maximum=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t);

		int bufferPosition=0;

		while(bufferPosition<maximum && m_taxonIterator!= m_taxonsForPhylogeny.end()){
			uint64_t taxon=*m_taxonIterator;
			buffer[bufferPosition]=taxon;
			bufferPosition++;
			m_taxonIterator++;
		}
		
		#ifdef ASSERT
		assert(bufferPosition!=0);
		#endif

		m_switchMan->sendMessage(buffer,bufferPosition,m_outbox,m_rank,MASTER_RANK,RAY_MPI_TAG_TOUCH_TAXON);

		m_messageSent=true;
		m_messageReceived=false;

	}else if(m_inbox->hasMessage(RAY_MPI_TAG_TOUCH_TAXON_REPLY)){

		m_messageReceived=true;
		m_messageSent=false;
	}
}

/**
 * here we extract the phylogeny colors
 */
void PhylogenyViewer::extractColorsForPhylogeny(){

	GridTableIterator iterator;
	iterator.constructor(m_subgraph,m_parameters->getWordSize(),m_parameters);

	//* only fetch half of the iterated things because we just need one k-mer
	// for any pair of reverse-complement k-mers 
	
	int parity=0;

	while(iterator.hasNext()){

		#ifdef ASSERT
		assert(parity==0 || parity==1);
		#endif

		if(parity==0){
			parity=1;
		}else if(parity==1){
			parity=0;

			continue; // we only need data with parity=0
		}

		Vertex*node=iterator.next();
		Kmer key=*(iterator.getKey());

		VirtualKmerColorHandle color=node->getVirtualColor();
		set<PhysicalKmerColor>*physicalColors=m_colorSet->getPhysicalColors(color);

		for(set<PhysicalKmerColor>::iterator j=physicalColors->begin();
			j!=physicalColors->end();j++){

			PhysicalKmerColor physicalColor=*j;
	
			uint64_t nameSpace=physicalColor/COLOR_NAMESPACE;
		
			if(nameSpace==PHYLOGENY_NAMESPACE){
				PhysicalKmerColor colorForPhylogeny=physicalColor % COLOR_NAMESPACE;

				m_colorsForPhylogeny.insert(colorForPhylogeny);

				#ifdef DEBUG_PHYLOGENY
				cout<<"[phylogeny] colorForPhylogeny= "<<colorForPhylogeny<<endl;
				#endif
			}
		}
	}
		
	cout<<"Rank "<<m_rank<<" has exactly "<<m_colorsForPhylogeny.size()<<" k-mer physical colors for the phylogeny."<<endl;
	cout<<endl;

	m_extractedColorsForPhylogeny=true;

	m_loadedTaxonsForPhylogeny=false;
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

	RAY_MPI_TAG_SYNCED_TAXONS=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_SYNCED_TAXONS,"RAY_MPI_TAG_SYNCED_TAXONS");

	RAY_MPI_TAG_LOADED_TAXONS=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_LOADED_TAXONS,"RAY_MPI_TAG_LOADED_TAXONS");

	RAY_MPI_TAG_TOUCH_TAXON=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_TOUCH_TAXON,"RAY_MPI_TAG_TOUCH_TAXON");
	m_adapter_RAY_MPI_TAG_TOUCH_TAXON.setObject(this);
	core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_TOUCH_TAXON,&m_adapter_RAY_MPI_TAG_TOUCH_TAXON);

	RAY_MPI_TAG_TOUCH_TAXON_REPLY=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_TOUCH_TAXON_REPLY,"RAY_MPI_TAG_TOUCH_TAXON_REPLY");

	m_switchMan=core->getSwitchMan();
	m_outbox=core->getOutbox();
	m_inbox=core->getInbox();
	m_outboxAllocator=core->getOutboxAllocator();
	m_inboxAllocator=core->getInboxAllocator();

	m_rank=core->getMessagesHandler()->getRank();
	m_size=core->getMessagesHandler()->getSize();
	m_extractedColorsForPhylogeny=false;

	m_core=core;


}

void PhylogenyViewer::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_PHYLOGENY_MAIN=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_PHYLOGENY_MAIN");
	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");

	RAY_MPI_TAG_PHYLOGENY_MAIN=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_PHYLOGENY_MAIN");
	RAY_MPI_TAG_TOUCH_TAXON=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TOUCH_TAXON");
	RAY_MPI_TAG_TOUCH_TAXON_REPLY=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_TOUCH_TAXON_REPLY");
	RAY_MPI_TAG_LOADED_TAXONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_LOADED_TAXONS");
	RAY_MPI_TAG_SYNCED_TAXONS=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_SYNCED_TAXONS");

	RAY_SLAVE_MODE_PHYLOGENY_MAIN=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_PHYLOGENY_MAIN");

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,RAY_MPI_TAG_PHYLOGENY_MAIN);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_PHYLOGENY_MAIN,RAY_SLAVE_MODE_PHYLOGENY_MAIN);

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_PHYLOGENY_MAIN,RAY_MASTER_MODE_KILL_RANKS);


	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");
	m_subgraph=(GridTable*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/deBruijnGraph_part.ray");
	m_colorSet=(ColorSet*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/VirtualColorManagementUnit.ray");
	m_timePrinter=(TimePrinter*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Timer.ray");

}

