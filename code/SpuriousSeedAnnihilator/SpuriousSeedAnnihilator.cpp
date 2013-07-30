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

#include <code/VerticesExtractor/GridTableIterator.h>

#include <sstream>
using namespace std;

__CreatePlugin(SpuriousSeedAnnihilator);

__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PROCESS_MERGING_ASSETS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);
__CreateMasterModeAdapter(SpuriousSeedAnnihilator, RAY_MASTER_MODE_MERGE_SEEDS);

__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);
__CreateSlaveModeAdapter(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_MERGE_SEEDS);

__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_MERGE_SEEDS);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY);

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

		m_core->closeSlaveModeLocally();

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

ComputeCore * SpuriousSeedAnnihilator::getCore() {
	return m_core;
}

SpuriousSeedAnnihilator * SpuriousSeedAnnihilator::getThis() {
	return this;
}

SpuriousSeedAnnihilator * SpuriousSeedAnnihilator::getThat() {

	SpuriousSeedAnnihilator * that = this;

	return that;
}

/*
 * handlers.
 */
void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_REGISTER_SEEDS(){

	if(!m_distributionIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_distributionIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		//m_core->getSwitchMan()->closeMasterMode();

		if(!m_filteredSeeds) {

			this->m_core->getSwitchMan()->setMasterMode(RAY_MASTER_MODE_FILTER_SEEDS);

			m_filteredSeeds = true;
		} else if(!m_mergedSeeds) {

			this->m_core->getSwitchMan()->setMasterMode(RAY_MASTER_MODE_MERGE_SEEDS);

			m_mergedSeeds = true;
		}

		m_distributionIsStarted = false;
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

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS(Message * message) {
}

void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_PROCESS_MERGING_ASSETS() {

	if(!m_processingIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(), m_core->getRank());

		m_processingIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		m_core->getSwitchMan()->closeMasterMode();
	}
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY(Message * message) {

	//cout << "[DEBUG] received RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY" << endl;

	uint8_t * buffer = (uint8_t*) message->getBuffer();

	GraphSearchResult entry;
	entry.load(buffer);

	m_mergingTechnology.getResults().push_back(entry);

	m_core->getSwitchMan()->sendEmptyMessage(m_outbox, m_rank, message->getSource(), RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY);
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY(Message * message) {
	m_messageWasReceived = true;
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS() {

	if(!m_initializedProcessing) {

		//cout << "[DEBUG] initialize RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS" << endl;

		m_entryIndex = 0;

		int value = 0;
		MODE_SPREAD_DATA = value++;
		MODE_CHECK_RESULTS = value++;
		MODE_STOP_THIS_SITUATION = value++;
		MODE_SHARE_WITH_LINKED_ACTORS = value ++;
		MODE_WAIT_FOR_ARBITER = value++;
		MODE_EVALUATE_GOSSIPS = value++;

		m_mode = MODE_SPREAD_DATA;
		m_toDistribute = m_mergingTechnology.getResults().size();

		m_messageWasSent = false;

		m_initializedProcessing = true;
		m_synced = 0;
		m_mustAdviseRanks = false;

		return;
	}

	if(m_inbox->hasMessage(RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER)) {
		m_synced ++;

		if(m_synced == m_core->getSize()) {
			cout << "[DEBUG] arbiter will advise" << endl;
			m_mustAdviseRanks = true;
			m_rankToAdvise = 0;
		}
	}

	if(m_mustAdviseRanks) {
		if(m_rankToAdvise < m_core->getSize()) {

			this->m_core->getSwitchMan()->sendEmptyMessage(m_outbox, m_rank, m_rankToAdvise, RAY_MESSAGE_TAG_ARBITER_SIGNAL);
			m_rankToAdvise ++;
		} else {
			m_synced = 0;
			m_mustAdviseRanks = false;

			cout << "[DEBUG] everybody received the arbiter advise" << endl;
		}
	}

	if (m_mode == MODE_SPREAD_DATA) {

		if(m_entryIndex < (int) m_toDistribute) {

			if(!m_messageWasSent) {

				//cout << "[DEBUG] send " << m_entryIndex << endl;

				uint8_t*messageBuffer=(uint8_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
				GraphSearchResult & entry = m_mergingTechnology.getResults()[m_entryIndex];
				int bytes = entry.dump(messageBuffer);

				int elements = bytes / sizeof(MessageUnit);

				if(bytes % sizeof(MessageUnit))
					elements ++;

				// at least one rank is the current rank
				Rank rank1 = getRankFromPathUniqueId(entry.getPathHandles()[0]);
				Rank rank2 = getRankFromPathUniqueId(entry.getPathHandles()[1]);

				Rank destination = rank1;

				if(destination == m_rank)
					destination = rank2;

				//cout << "[DEBUG] destination " << destination << endl;

				if(destination != m_rank) {
					Message aMessage((MessageUnit*)messageBuffer, elements , destination, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY, m_rank);
					m_outbox->push_back(&aMessage);

					m_messageWasReceived = false;

				} else {
					// nothing to send,
					// the local rank has both

					m_messageWasReceived = true;
				}

				m_messageWasSent = true;

			} else if(m_messageWasReceived) {

				//cout << "[DEBUG] receive " << m_entryIndex << endl;

				m_entryIndex ++;

				m_messageWasSent = false;
			}
		} else {
			Rank arbiter = getArbiter();

			this->m_core->getSwitchMan()->sendEmptyMessage(m_outbox, m_rank, arbiter, RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER);
			m_mode = MODE_WAIT_FOR_ARBITER;
		}
	} else if(m_mode == MODE_WAIT_FOR_ARBITER) {

		if(m_inbox->hasMessage(RAY_MESSAGE_TAG_ARBITER_SIGNAL))
			m_mode = MODE_CHECK_RESULTS;

	} else if(m_mode == MODE_CHECK_RESULTS) {

		cout << "[DEBUG] m_toDistribute " << m_toDistribute << " now -> " << m_mergingTechnology.getResults().size() << endl;
		cout << "[DEBUG] scanning for duplicates" << endl;

		map<PathHandle, map<PathHandle, vector<int> > > counts;

		for(int i = 0 ; i < (int)m_mergingTechnology.getResults().size() ; i++) {

			// there were no local hits...
			// so let it fail
			if(m_toDistribute == 0)
				break;

			GraphSearchResult & result = m_mergingTechnology.getResults()[i];

			PathHandle handle1 = result.getPathHandles()[0];
			PathHandle handle2 = result.getPathHandles()[1];

			if(handle2 < handle1) {
				PathHandle store = handle1;
				handle1 = handle2;
				handle2 = store;
			}

			counts[handle1][handle2].push_back(i);
		}

		for(map<PathHandle, map<PathHandle, vector<int> > >::iterator i = counts.begin();
				i != counts.end(); ++i) {

			for(map<PathHandle, vector<int> >::iterator j = i->second.begin();
					j != i->second.end() ; j++) {

				if(j->second.size() == 2) {
					cout << "[DEBUG] MODE_CHECK_RESULTS got a symmetric relation between " << i->first;
					cout << " and " << j->first << endl;

					//m_indexesToShareWithArbiter.push_back(j->second[0]);
					//m_indexesToShareWithArbiter.push_back(j->second[1]);

					int index = j->second[0];
					GraphSearchResult & gossip = m_mergingTechnology.getResults()[index];
					m_gossips.push_back(gossip);
					m_gossipIndex.insert(gossip.toString());

					// at least one rank is the current rank
					Rank rank1 = getRankFromPathUniqueId(gossip.getPathHandles()[0]);
					Rank rank2 = getRankFromPathUniqueId(gossip.getPathHandles()[1]);

					if(rank1 != m_rank)
						m_linkedActorsForGossip.insert(rank1);
					if(rank2 != m_rank)
						m_linkedActorsForGossip.insert(rank2);
				}
			}

		}

		m_mergingTechnology.getResults().clear();

		m_hasNewGossips = true;
		m_lastGossipingEventTime = time(NULL);

		cout << "[DEBUG] gossiping begins..." << endl;

		m_mode = MODE_SHARE_WITH_LINKED_ACTORS;

	} else if(m_mode == MODE_SHARE_WITH_LINKED_ACTORS) {

		// find a information that was not shared with a given actor
		// this is a good-enough implementation
		// this is like a gossip algorithm, but here the pairs are not randomly
		// selected, they are obtained from the relationships computed
		// in the de Bruijn graph.

#if 0
		m_mode = MODE_STOP_THIS_SITUATION; // remove this
		return; // remove this
#endif

		if(m_inbox->hasMessage(RAY_MESSAGE_TAG_SEED_GOSSIP)) {
			Message * message = m_inbox->at(0);

			GraphSearchResult gossip;
			int position = 0;
			uint8_t * buffer = (uint8_t*) message->getBuffer();

			int bytes = gossip.load(buffer + position);
			position += bytes;

			// first check if the gossip is already known.
			// this implementation is good enough, but it could
			// use an index.

			string key = gossip.toString();

			bool found = m_gossipIndex.count(key) > 0;

			if(found)
				return;

			// yay we got new gossip to share !!!
			m_gossips.push_back(gossip);
			m_gossipIndex.insert(key);
			m_lastGossipingEventTime = time(NULL);

			// we could also update the m_gossipStatus
			// because we know that message->getSource() has this gossip
			// already.
			// But anyway, it does not change much to the algorithm...
			// update: source has gossip already.
			//
			// also important, gossipIndex values are local and not global to
			// all ranks
			//

			Rank actor = message->getSource();
			int gossipIndex = m_gossips.size() - 1;

#ifdef CONFIG_ASSERT
			assert(m_linkedActorsForGossip.count(actor) > 0);
			assert(gossipIndex < (int)m_gossips.size());
			assert(actor >= 0);
			assert(actor < m_core->getSize());
#endif

			m_gossipStatus[gossipIndex].insert(actor);

			// we have new gossip, so this is important to store.
			m_hasNewGossips = true;

			cout << "[DEBUG] Rank rank:" << m_rank << " received gossip gossip:" << key << " from rank rank:" << actor << endl;

			return;
		}

		/**
		 *
		 * We wait a specific amount of time before stopping the
		 * gossip process.
		 *
		 * This code will fail if the process receives a SIGSTOP
		 *
		 * TODO  or if the process is not scheduled during too many seconds.
		 */

		if(!m_hasNewGossips) {

			// check delay and latency
			time_t currentTime = time(NULL);

			int minimumWaitTime = 10; // seconds
			int distance = currentTime - m_lastGossipingEventTime;

			if(distance < minimumWaitTime)
				return;

			cout << "[DEBUG] Rank " << m_rank << " gossips have spreaded." << endl;

			m_gossipStatus.clear();
			m_linkedActorsForGossip.clear();

			// synchronize indexes in m_indexesToShareWithArbiter
			m_mode = MODE_EVALUATE_GOSSIPS;
			return;
		}

		for(int gossipIndex = 0 ; gossipIndex < (int)m_gossips.size() ; ++gossipIndex) {

			if(m_gossipStatus[gossipIndex].size() == m_linkedActorsForGossip.size())
				continue;

			// attempt to share gossip with linked actors
			// find an actor that does not have the gossip already
			for(set<Rank>::iterator i = m_linkedActorsForGossip.begin() ; i != m_linkedActorsForGossip.end() ; ++i) {
				Rank actor = *i;

				bool gossipStatus = m_gossipStatus[gossipIndex].count(actor) > 0;

				if(gossipStatus)
					continue;

				// we found a gossip that needs to be shared with actor <actor>

				m_lastGossipingEventTime = time(NULL);

				uint8_t*messageBuffer = (uint8_t*)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

#ifdef ASSERT_CONFIG
				assert( messageBuffer != NULL );
#endif /* ASSERT_CONFIG */

				int position = 0;
				GraphSearchResult & gossip =  /*&*/ m_gossips[gossipIndex];
				position += gossip.dump(messageBuffer + position);

				int units = position / sizeof(MessageUnit);

				if(position % sizeof(MessageUnit))
					units ++;

				string key = gossip.toString();

				Message aMessage((MessageUnit*)messageBuffer, units, actor,
					RAY_MESSAGE_TAG_SEED_GOSSIP, m_rank);
				m_outbox->push_back(&aMessage);

				m_gossipStatus[gossipIndex].insert(actor);
				cout << "[DEBUG] Rank rank:" << m_rank << " sent gossip gossip:" << key << " from rank rank:" << actor << endl;

				return;
			}
		}

		// at this point, we tried every gossip and they are all synchronized.

		m_hasNewGossips = false;

	} else if(m_mode == MODE_EVALUATE_GOSSIPS) {

		/**
		 * Here, we do that in batch.
		 */

		m_seedGossipSolver.setInput(&m_gossips);
		m_seedGossipSolver.compute();

		vector<GraphSearchResult> & solution = m_seedGossipSolver.getSolution();

		cout << "[DEBUG] Rank " << m_rank << " gossip count: " << m_gossips.size() << endl;
		cout << "[DEBUG] solution has " << solution.size() << " entries !" << endl;

		/**
		 * Now, the actor needs to check how many of its seeds are in the solution.
		 * The actor effectively loses ownership for any of these seeds in the solution.
		 *
		 * The actor can keep the seeds not in the solution.
		 *
		 * After that, the seeds in the solution need to be assigned ownership.
		 * For the first implementation, the ownership will be based on actors associated
		 * with the seeds in the solution.
		 *
		 * For instance, for the result A-B-C where A is owned by 0, B by 1 and C by 2. Then this
		 * A-B-C solution will be owned by 0, 1 or 2.
		 */

		m_mode = MODE_STOP_THIS_SITUATION;

	} else if(m_mode == MODE_STOP_THIS_SITUATION) {

		m_core->closeSlaveModeLocally();
	}

}

void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_CLEAN_SEEDS(){

	if(!m_cleaningIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_cleaningIsStarted=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		if(!this->m_mergedSeeds){

			this->m_core->getSwitchMan()->setMasterMode(RAY_MASTER_MODE_REGISTER_SEEDS);
			this->m_core->getSwitchMan()->reset();

		} else {

			// the registered handle with the API is
			// RAY_MASTER_MODE_PUSH_SEED_LENGTHS
			m_core->getSwitchMan()->closeMasterMode();
		}

		// anyway, the code in this score changed the fate of future cycles.
		m_cleaningIsStarted = false;
	}
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_REGISTER_SEEDS(){

	if(!m_initializedSeedRegistration) {

		cout << "[DEBUG] call_RAY_SLAVE_MODE_REGISTER_SEEDS: initializing..." << endl;
		m_seedIndex=0;
		m_seedPosition=0;
		m_initializedSeedRegistration = true;
		m_registrationIterations++;

		if(!m_debugCode && m_parameters->hasCheckpoint("Seeds")){
			m_hasCheckpointFilesForSeeds = true;
		}

		return;
	}

	// iteration 1: for the seed elimination (skipped for small k)
	// iteration 2: for the merging (never skipped)
	if((!m_debugCode && m_hasCheckpointFilesForSeeds) || (m_skip && m_registrationIterations == 1)){

		m_core->closeSlaveModeLocally();

#ifdef DEBUG_SEED_REGISTRATION
		cout << "[DEBUG] skipping call_RAY_SLAVE_MODE_REGISTER_SEEDS" << endl;
#endif

		m_initializedSeedRegistration = false;

		return;
	}

	if(m_inbox->hasMessage(RAY_MESSAGE_TAG_PUSH_SEEDS_REPLY)){
		m_activeQueries--;
		return;
	}

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

			m_buffers.addAt(destination, identifier.getValue());
			m_buffers.addAt(destination, m_seedPosition);

			if(m_buffers.flush(destination, elements, tag, m_outboxAllocator, m_outbox, m_rank,false)){
				m_activeQueries++;
			}

#if 0
			if(m_seedPosition % 1000 == 0) {
				cout << "Rank "<<m_rank << " registered " << m_seedIndex << "/" <<m_seeds->size();
				cout<< " "<< m_seedPosition << "/" << (*m_seeds)[m_seedIndex].size() << endl;
			}
#endif

			m_seedPosition++;
		}else{

			if(m_seedIndex % 1000 == 0)
				cout << "Rank "<<m_rank << " registered " << m_seedIndex << "/" <<m_seeds->size() << endl;

			m_seedIndex++;

			m_seedPosition = 0;
		}

	}else if(!m_buffers.isEmpty()){

/**
 * Flush additional queries
 */
		MessageTag tag=RAY_MESSAGE_TAG_PUSH_SEEDS;

		m_activeQueries += m_buffers.flushAll(tag, m_outboxAllocator, m_outbox, m_rank);

	}else{
		m_initializedSeedRegistration = false;

		cout << "Rank "<<m_rank << " registered " << m_seedIndex - 1 << "/" <<m_seeds->size() << endl;
		cout<<"Rank "<<m_rank << " registered its seeds" << endl;

		m_core->closeSlaveModeLocally();
	}
}

/**
 *
 * \see https://github.com/sebhtml/ray/issues/188
 * \author Sébastien Boisvert
 */
void SpuriousSeedAnnihilator::call_RAY_MASTER_MODE_MERGE_SEEDS() {
	// merge the seeds using arbitration

	if(!m_mergingIsStarted){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getRank());

		m_mergingIsStarted = true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		//this->getThis()->getThat()->getCore()->getSwitchMan()->setMasterMode(RAY_MASTER_MODE_PUSH_SEED_LENGTHS);
		this->getThis()->getThat()->getCore()->getSwitchMan()->setMasterMode(RAY_MASTER_MODE_CLEAN_SEEDS);
		getCore()->getSwitchMan()->reset();
	}

	/*
	 * parallel design:
	 *
	 * while true
	 * 	1. register seeds
	 * 	2. merge seeds using arbitration
	 * 	3. clean seeds
	 *
	 * then, exit via the backdoor called "RAY_MASTER_MODE_PUSH_SEED_LENGTHS"
	 *
	 * checkpoints are saved top storage devices after this current step.
	 */
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_MERGE_SEEDS() {
	// merge the seeds using arbitration

	if(this->m_debug) {
		cout << m_core->getRank() << " merging seeds now." << endl;
	}

	m_mergingTechnology.mainLoop();

	// It is the class SeedMergingWorkflow (interface is TaskCreator) that
	// will actually close the slave mode.
	//m_core->closeSlaveModeLocally();
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_MERGE_SEEDS(Message*message) {
	// merge the seeds using arbitration
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_FILTER_SEEDS(){

	if((!m_debugCode && m_hasCheckpointFilesForSeeds) || m_skip){

		m_core->closeSlaveModeLocally();
		return;
	}

	m_workflow.mainLoop();
}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_CLEAN_SEEDS(){

	m_cleaningIterations ++;

	// this must be skipped for the first iteration if m_skip is true
	if((!m_debugCode && m_hasCheckpointFilesForSeeds) || (m_skip && m_cleaningIterations == 1)){

		m_core->closeSlaveModeLocally();
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

	cout<<"Rank "<<m_rank<<" freed "<< bytes <<" bytes from the path memory pool ";
	cout << "(chunks: " << m_directionsAllocator->getNumberOfChunks() << ")" << endl;

	m_directionsAllocator->clear();

// Trace was here -> <s>FAIL</s>  PASS

/*
 * Tell another rank that we are done with this.
 */
	m_core->closeSlaveModeLocally();
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
		ostringstream buffer;

		cout<<"Rank "<<m_parameters->getRank()<<" is writing checkpoint Seeds"<<endl;
		int count=(*m_seeds).size();

		buffer.write((char*)&count, sizeof(int));

		for(int i=0;i<(int)(*m_seeds).size();i++){
			int length=(*m_seeds)[i].size();
			buffer.write((char*)&length, sizeof(int));

			for(int j=0;j<(int)(*m_seeds)[i].size();j++){
				Kmer theKmer;
				(*m_seeds)[i].at(j,&theKmer);
				theKmer.write(&buffer);

				CoverageDepth coverageValue=0;
				coverageValue=(*m_seeds)[i].getCoverageAt(j);
				buffer.write((char*)&coverageValue, sizeof(CoverageDepth));
				flushFileOperationBuffer(false, &buffer, &f, CONFIG_FILE_IO_BUFFER_SIZE);
			}
		}
                flushFileOperationBuffer(true, &buffer, &f, CONFIG_FILE_IO_BUFFER_SIZE);
		f.close();
	}
}

bool SpuriousSeedAnnihilator::isPrimeNumber(int number) {

	for(int i = 2 ; i < number ; ++i) {

		if(number % i == 0)
			return false;
	}

	return true;
}

Rank SpuriousSeedAnnihilator::getArbiter() {
	int rank = m_parameters->getSize();

	while(rank > 0 && !isPrimeNumber(rank))
		rank--;

	return rank;
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
 *
 * The entry point is RAY_MASTER_MODE_REGISTER_SEEDS.
 *
 * The exit point is RAY_MASTER_MODE_TRIGGER_DETECTION (not here).
 */
void SpuriousSeedAnnihilator::registerPlugin(ComputeCore*core){

	m_core=core;
	PluginHandle plugin=core->allocatePluginHandle();
	m_plugin=plugin;

	core->setPluginName(plugin,"SpuriousSeedAnnihilator");
	core->setPluginDescription(plugin,"Pre-processing of seeds.");
	core->setPluginAuthors(plugin,"Sébastien Boisvert");
	core->setPluginLicense(plugin,"GNU General Public License version 3");

	RAY_MESSAGE_TAG_ARBITER_SIGNAL = m_core->allocateMessageTagHandle(m_plugin);
	RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER = m_core->allocateMessageTagHandle(m_plugin);

	RAY_MESSAGE_TAG_SEED_GOSSIP = m_core->allocateMessageTagHandle(m_plugin);

	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_REGISTER_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_FILTER_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_CLEAN_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_MERGE_SEEDS);
	__ConfigureMasterModeHandler(SpuriousSeedAnnihilator, RAY_MASTER_MODE_PROCESS_MERGING_ASSETS);

	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_REGISTER_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_FILTER_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_CLEAN_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_MERGE_SEEDS);
	__ConfigureSlaveModeHandler(SpuriousSeedAnnihilator, RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS);

	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_FILTER_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_CLEAN_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_MERGE_SEEDS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY);

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

	m_initializedSeedRegistration = false;

	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_REGISTER_SEEDS, RAY_MASTER_MODE_FILTER_SEEDS);
	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_FILTER_SEEDS, RAY_MASTER_MODE_CLEAN_SEEDS);

	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_CLEAN_SEEDS, RAY_MASTER_MODE_PROCESS_MERGING_ASSETS);
	core->setMasterModeNextMasterMode(m_plugin, RAY_MASTER_MODE_PROCESS_MERGING_ASSETS, RAY_MASTER_MODE_PUSH_SEED_LENGTHS);

	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_PROCESS_MERGING_ASSETS, RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS);
	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_REGISTER_SEEDS, RAY_MESSAGE_TAG_REGISTER_SEEDS);
	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_FILTER_SEEDS, RAY_MESSAGE_TAG_FILTER_SEEDS);
	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_CLEAN_SEEDS, RAY_MESSAGE_TAG_CLEAN_SEEDS);
	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_PUSH_SEED_LENGTHS, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS);
	core->setMasterModeToMessageTagSwitch(m_plugin, RAY_MASTER_MODE_MERGE_SEEDS, RAY_MESSAGE_TAG_MERGE_SEEDS);

	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_REGISTER_SEEDS, RAY_SLAVE_MODE_REGISTER_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_PROCESS_MERGING_ASSETS, RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_FILTER_SEEDS, RAY_SLAVE_MODE_FILTER_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_CLEAN_SEEDS, RAY_SLAVE_MODE_CLEAN_SEEDS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_PUSH_SEED_LENGTHS, RAY_SLAVE_MODE_PUSH_SEED_LENGTHS);
	core->setMessageTagToSlaveModeSwitch(m_plugin, RAY_MESSAGE_TAG_MERGE_SEEDS, RAY_SLAVE_MODE_MERGE_SEEDS);

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
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE");
	RAY_MPI_TAG_ASK_VERTEX_PATH = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MPI_TAG_ASK_VERTEX_PATH");

	m_workflow.initialize(m_seeds, m_virtualCommunicator, m_virtualProcessor, m_core, m_parameters,
		RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT, RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
		RAY_MPI_TAG_ASK_VERTEX_PATH
	);

	m_mergingTechnology.initialize(m_seeds, m_virtualCommunicator, m_virtualProcessor, m_core, m_parameters,
		RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT, RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
		RAY_MPI_TAG_ASK_VERTEX_PATH
	);

/**
 * Turn this on to run this code even when checkpoints exist.
 * This should be set to false in production.
 */
	m_debugCode = m_parameters->hasOption("-debug-seed-filter");

	m_skip = 2 * m_parameters->getWordSize() < m_parameters->getMinimumContigLength();

	m_cleaningIterations = 0;
	m_registrationIterations = 0;

	m_debug = false;
	m_mergingIsStarted = false;

	m_filteredSeeds = false;
	m_mergedSeeds = false;

	m_initializedProcessing = false;

}
