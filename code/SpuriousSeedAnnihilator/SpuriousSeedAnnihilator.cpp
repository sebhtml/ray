/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
 *  Copyright (C) 2013 Sébastien Boisvert
 *
 *  http://DeNovoAssembler.SourceForge.Net/
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You have received a copy of the GNU General Public License
 *  along with this program (gpl-3.0.txt).
 *  see <http://www.gnu.org/licenses/>
 */

#include "SpuriousSeedAnnihilator.h"

#include <code/plugin_VerticesExtractor/GridTableIterator.h>

#include <sstream>
using namespace std;

__CreatePlugin(SpuriousSeedAnnihilator);

__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);

__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);

__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS);

SpuriousSeedAnnihilator::SpuriousSeedAnnihilator(){
}

/**
 * Even if checkpoints exist, we don't skip this code path.
 */
void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_PUSH_SEED_LENGTHS(){
	if(!m_initialized){

		vector<GraphPath> newSeeds;

		for(int i=0;i<(int)(*m_seeds).size();i++){

			if((*m_seeds)[i].isDeleted())
				continue;

			newSeeds.push_back((*m_seeds)[i]);
		}

		(*m_seeds) = newSeeds;

		for(int i=0;i<(int)(*m_seeds).size();i++){

			int length=getNumberOfNucleotides((*m_seeds)[i].size(),
				m_parameters->getWordSize());
			m_slaveSeedLengths[length]++;
		}
		m_iterator=m_slaveSeedLengths.begin();
		m_initialized=true;
		m_communicatorWasTriggered=false;


		m_virtualCommunicator->resetCounters();
	}

	if(m_inbox->size()==1&&(*m_inbox)[0]->getTag()==RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY)
		m_communicatorWasTriggered=false;

	if(m_communicatorWasTriggered)
		return;

	if(m_iterator==m_slaveSeedLengths.end()){

		writeCheckpointForSeeds();

		writeSingleSeedFile();

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());

		return;
	}

	MessageUnit*messageBuffer=(MessageUnit*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	int maximumPairs=MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit)/2;

	int i=0;

	while(i<maximumPairs && m_iterator!=m_slaveSeedLengths.end()){
		int length=m_iterator->first;
		int count=m_iterator->second;
		messageBuffer[2*i]=length;
		messageBuffer[2*i+1]=count;
		i++;
		m_iterator++;
	}

	Message aMessage(messageBuffer,2*i,MASTER_RANK,
		RAY_MESSAGE_TAG_SEND_SEED_LENGTHS, m_rank);
	m_outbox->push_back(&aMessage);
}

/*
 * handlers.
 */
void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_REGISTER_SEEDS(){

	if(!m_distributionIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_distributionIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		m_core->getSwitchMan()->closeMasterMode();
	}
}

void SpuriousSeedAnnihilator::writeSingleSeedFile(){

	/** write seeds for debugging purposes */
	if(m_parameters->hasOption("-write-seeds")){
		ostringstream fileName;
		fileName<<m_parameters->getPrefix()<<"Rank"<<m_parameters->getRank()<<".RaySeeds.fasta";
		ofstream f(fileName.str().c_str());

		for(int i=0;i<(int)(*m_seeds).size();i++){
			PathHandle id=getPathUniqueId(m_parameters->getRank(),i);
			f<<">RaySeed-"<<id<<endl;

			f<<addLineBreaks(convertToString(&((*m_seeds)[i]),
				m_parameters->getWordSize(),m_parameters->getColorSpaceMode()),
				m_parameters->getColumns());
		}
		f.close();
	}
}

void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_PUSH_SEED_LENGTHS(){

	if(!m_gatheringHasStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_gatheringHasStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		writeSeedStatistics();

		m_core->getSwitchMan()->closeMasterMode();
	}
}

void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_FILTER_SEEDS(){

	if(!m_filteringIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_filteringIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){
		m_core->getSwitchMan()->closeMasterMode();
	}
}

void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_CLEAN_SEEDS(){

	if(!m_cleaningIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_cleaningIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){
		m_core->getSwitchMan()->closeMasterMode();
	}
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_REGISTER_SEEDS(){

	if(!m_debugCode && m_parameters->hasCheckpoint("Seeds")){
		m_hasCheckpointFilesForSeeds = true;
	}

	if(m_hasCheckpointFilesForSeeds){

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
		return;
	}

	if(m_inbox->hasMessage(RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY))
		m_activeQueries--;

	#ifdef ASSERT
	assert(m_activeQueries>=0);
	#endif

	if(m_activeQueries > 0)
		return;

	#ifdef ASSERT
	assert(m_activeQueries==0);
	#endif

	if(m_seedIndex < (int)m_seeds->size()){

		if(m_seedPosition < (*m_seeds)[m_seedIndex].size()){

			if(m_seedIndex==0 && m_seedPosition == 0)
				cout<<"Rank "<<m_core->getRank()<<" has "<<m_seeds->size()<<" seeds to register."<<endl;

			Kmer vertex;
			(*m_seeds)[m_seedIndex].at(m_seedPosition,&vertex);

			Rank destination=m_parameters->vertexRank(&vertex);

			MessageTag tag=RAY_MESSAGE_TAG_PUSH_SEEDS;
			int elements = m_virtualCommunicator->getElementsPerQuery(tag);

			for(int i=0;i<vertex.getNumberOfU64();i++){
				m_buffers.addAt(destination,vertex.getU64(i));
			}

			PathHandle identifier = getPathUniqueId(m_rank, m_seedIndex);
			m_buffers.addAt(destination, identifier);
			m_buffers.addAt(destination, m_seedPosition);

			if(m_buffers.flush(destination, elements, tag, m_outboxAllocator, m_outbox, m_rank,false)){
				m_activeQueries++;
			}

			m_seedPosition++;
		}else{

			m_seedIndex++;
		}

	}else if(!m_buffers.isEmpty()){

/**
 * Flush additional queries
 */
		MessageTag tag=RAY_MESSAGE_TAG_PUSH_SEEDS;

		m_activeQueries += m_buffers.flushAll(tag, m_outboxAllocator, m_outbox, m_rank);

	}else{
		m_seedIndex=0;
		m_seedPosition=0;

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
	}
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_FILTER_SEEDS(){

	if(!m_debugCode && m_hasCheckpointFilesForSeeds){

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
		return;
	}

	m_workflow.mainLoop();
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_CLEAN_SEEDS(){

	if(!m_debugCode && m_hasCheckpointFilesForSeeds){

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
		return;
	}

// Trace was here -> PASS

	#ifdef ASSERT
	assert(m_parameters != NULL);
	assert(m_subgraph != NULL);
	#endif

	// clear graph
	GridTableIterator iterator;
	iterator.constructor(m_subgraph, m_parameters->getWordSize(), m_parameters);

	#ifdef ASSERT
	LargeCount cleared=0;
	#endif

// Trace was here -> PASS

	while(iterator.hasNext()){
		iterator.next();
		Kmer key=*(iterator.getKey());
		m_subgraph->clearDirections(&key);

		#ifdef ASSERT
		cleared++;

		Vertex*node=m_subgraph->find(&key);
		assert(node->getFirstDirection() == NULL);

		#endif
	}

// Trace was here -> PASS

	#ifdef ASSERT
	assert(cleared == m_subgraph->size());
	#endif

// Trace was here -> PASS

	int bytes=m_directionsAllocator->getChunkSize() * m_directionsAllocator->getNumberOfChunks();

	m_directionsAllocator->clear();

	cout<<"Rank "<<m_rank<<" freed "<<bytes/1024<<" KiB from the path memory pool"<<endl;

// Trace was here -> <s>FAIL</s>  PASS

/*
 * Tell another rank that we are done with this.
 */
	m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getRank());
}

/**
 * The 3 methods below are not used, but they were added to test this
 * new RayPlatform API call:
 *
 * __ConfigureMessageTagHandler
 *
 */
void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_REGISTER_SEEDS(Message*message){
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS(Message*message){
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_FILTER_SEEDS(Message*message){
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_CLEAN_SEEDS(Message*message){
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_SEND_SEED_LENGTHS(Message*message){
	void*buffer=message->getBuffer();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();
	for(int i=0;i<count;i+=2){
		int seedLength=incoming[i];
		int number=incoming[i+1];
		m_masterSeedLengths[seedLength]+=number;
	}

	Message aMessage(NULL,0,message->getSource(),RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY,m_rank);
	m_outbox->push_back(&aMessage);
}

void SpuriousSeedAnnihilator::writeCheckpointForSeeds(){

	/* write the Seeds checkpoint */
	if(m_parameters->writeCheckpoints() && !m_parameters->hasCheckpoint("Seeds")){

		ofstream f(m_parameters->getCheckpointFile("Seeds").c_str());
		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint Seeds"<<endl;
		int count=(*m_seeds).size();

		f.write((char*)&count,sizeof(int));

		for(int i=0;i<(int)(*m_seeds).size();i++){
			int length=(*m_seeds)[i].size();
			f.write((char*)&length,sizeof(int));

			for(int j=0;j<(int)(*m_seeds)[i].size();j++){
				Kmer theKmer;
				(*m_seeds)[i].at(j,&theKmer);
				theKmer.write(&f);

				CoverageDepth coverageValue=0;
				coverageValue=(*m_seeds)[i].getCoverageAt(j);
				f.write((char*)&coverageValue,sizeof(CoverageDepth));
			}
		}
		f.close();
	}
}

void SpuriousSeedAnnihilator::writeSeedStatistics(){
	ostringstream file;
	file<<m_parameters->getPrefix();
	file<<"SeedLengthDistribution.txt";
	ofstream f(file.str().c_str());

	f<<"# SeedLengthInNucleotides	Frequency"<<endl;

	for(map<int,int>::iterator i=m_masterSeedLengths.begin();i!=m_masterSeedLengths.end();i++){
		int length=i->first;
		int count=i->second;
		f<<length<<"\t"<<count<<endl;
	}
	f.close();
}

/*
 * Methods to implement for the CorePlugin interface.
 */
void SpuriousSeedAnnihilator::registerPlugin(ComputeCore*core){

	m_core=core;
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"SpuriousSeedAnnihilator");
	core->setPluginDescription(plugin,"Pre-processing of seeds.");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);

	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);

	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS);

	RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY = m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY, "RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY");

	m_outboxAllocator = m_core->getOutboxAllocator();
	m_outbox = m_core->getOutbox() ;
	m_rank = m_core->getRank() ;
	m_inbox = m_core->getInbox();

	m_size = m_core->getSize();

	m_distributionIsStarted=false;
	m_filteringIsStarted=false;
	m_cleaningIsStarted=false;

	m_seedIndex=0;
	m_seedPosition=0;

	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_REGISTER_SEEDS, RAY_MASTER_MODE_FILTER_SEEDS);
	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_FILTER_SEEDS, RAY_MASTER_MODE_CLEAN_SEEDS);
	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_CLEAN_SEEDS, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);

	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_REGISTER_SEEDS, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_FILTER_SEEDS, RAY_MESSAGE_TAG_FILTER_SEEDS);
	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_CLEAN_SEEDS, RAY_MESSAGE_TAG_CLEAN_SEEDS);
	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_PUSH_SEED_LENGTHS, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);

	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_REGISTER_SEEDS, RAY_SLAVE_MODE_REGISTER_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_FILTER_SEEDS, RAY_SLAVE_MODE_FILTER_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_CLEAN_SEEDS, RAY_SLAVE_MODE_CLEAN_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);

	m_activeQueries=0;

	m_virtualCommunicator = m_core->getVirtualCommunicator();
	m_virtualProcessor = m_core->getVirtualProcessor();

	m_initialized=false;

	__BindPlugin(SpuriousSeedAnnihilator);

	m_hasCheckpointFilesForSeeds = false;

}

void SpuriousSeedAnnihilator::resolveSymbols(ComputeCore*core){

	// before arriving here, there is this edition to be performed elsewhere in the code
	// replace RAY_MASTER_MODE_TRIGGER_DETECTION by RAY_MASTER_MODE_CLEAN_SEEDS

	RAY_MASTER_MODE_TRIGGER_DETECTION=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_TRIGGER_DETECTION");
	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_PUSH_SEED_LENGTHS, RAY_MASTER_MODE_TRIGGER_DETECTION);

	m_seeds=(vector<GraphPath>*) m_core->getObjectFromSymbol(m_plugin, "/RayAssembler/ObjectStore/Seeds.ray");
	m_parameters=(Parameters*)m_core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");

	RAY_MESSAGE_TAG_PUSH_SEEDS = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MESSAGE_TAG_PUSH_SEEDS");
	RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY");
	RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MPI_TAG_IS_DONE_SENDING_SEED_LENGTHS");

	int elements = m_virtualCommunicator->getElementsPerQuery(RAY_MESSAGE_TAG_PUSH_SEEDS);

	m_buffers.constructor(m_size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),
		"/memory/SpuriousSeedAnnihilator",m_parameters->showMemoryAllocations(),elements);

	m_subgraph=(GridTable*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/deBruijnGraph_part.ray");
	m_directionsAllocator = (MyAllocator*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/directionMemoryPool.ray");

	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT");

	m_workflow.initialize(m_seeds, m_virtualCommunicator, m_virtualProcessor, m_core, m_parameters,
		RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);
/**
 * Turn this on to run this code even when checkpoints exist.
 * This should be set to false in production.
 */
	m_debugCode = m_parameters->hasOption("-debug-seed-filter");

}
