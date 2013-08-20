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

#include <RayPlatform/cryptography/crypto.h>

#include <sstream>
#include <algorithm>
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
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER);
__CreateMessageTagAdapter(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION);


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

	//cout << "[DEBUG] call_RAY_SLAVE_MODE_PUSH_SEED_LENGTHS preparing payload." << endl;

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

	char * buffer = (char *) message->getBuffer();

	GraphSearchResult entry;
	entry.load(buffer);

	m_mergingTechnology.getResults().push_back(entry);

	m_core->getSwitchMan()->sendEmptyMessage(m_outbox, m_rank, message->getSource(), RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY);
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_GATHER_PROXIMITY_ENTRY_REPLY(Message * message) {
	m_messageWasReceived = true;
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER(Message * message) {

	initializeMergingProcess();

	m_synced ++;

	//cout << "[DEBUG] recv RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER m_synced " << m_synced << endl;

	if(m_synced == m_core->getSize()) {
		//cout << "[DEBUG] arbiter will advise" << endl;

		m_mustAdviseRanks = true;
		m_rankToAdvise = 0;
	}
}

void SpuriousSeedAnnihilator::initializeMergingProcess() {
	if(!m_initializedProcessing) {

		//cout << "[DEBUG] initialize RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS" << endl;

		m_entryIndex = 0;

		int value = 0;
		MODE_SPREAD_DATA = value++;
		MODE_CHECK_RESULTS = value++;
		MODE_STOP_THIS_SITUATION = value++;
		MODE_GATHER_COVERAGE_VALUES = value++;
		MODE_SHARE_WITH_LINKED_ACTORS = value ++;
		MODE_WAIT_FOR_ARBITER = value++;
		MODE_EVALUATE_GOSSIPS = value++;
		MODE_REBUILD_SEED_ASSETS = value++;
		MODE_SHARE_PUSH_DATA_IN_KEY_VALUE_STORE = value++;
		MODE_GENERATE_NEW_SEEDS = value++;
		MODE_CLEAN_KEY_VALUE_STORE = value++;

		m_mode = MODE_SPREAD_DATA;

		m_toDistribute = m_mergingTechnology.getResults().size();

		m_messageWasSent = false;

		m_initializedProcessing = true;
		m_synced = 0;
		m_mustAdviseRanks = false;

		// we can not return here because we could lose a message
		//return;
	}
}

void SpuriousSeedAnnihilator::spreadAcquiredData() {

	if(m_entryIndex < (int) m_toDistribute) {

		if(!m_messageWasSent) {

			//cout << "[DEBUG] MODE_SPREAD_DATA send " << m_entryIndex << endl;

			char *messageBuffer=(char *)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);
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

			//cout << "[DEBUG] MODE_SPREAD_DATA destination " << destination << endl;

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

			//cout << "[DEBUG] MODE_SPREAD_DATA receive " << m_entryIndex << endl;

			m_entryIndex ++;

			m_messageWasSent = false;
		}
	} else {

		sendMessageToArbiter();
		m_nextMode = MODE_CHECK_RESULTS;
	}

}

void SpuriousSeedAnnihilator::sendMessageToArbiter() {

#ifdef DEBUG_SEED_MERGING
	cout << "[DEBUG] Rank " << m_core->getRank() << " sendMessageToArbiter// " << endl;
#endif

	Rank arbiter = getArbiter();
	this->m_core->getSwitchMan()->sendEmptyMessage(m_outbox, m_rank, arbiter, RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER);

	//cout << "[DEBUG] saying hello to arbiter " << arbiter << " with RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER" << endl;

	m_mode = MODE_WAIT_FOR_ARBITER;
}

void SpuriousSeedAnnihilator::checkResults() {
	//cout << "[DEBUG] MODE_CHECK_RESULTS m_toDistribute " << m_toDistribute << " now -> " << m_mergingTechnology.getResults().size() << endl;
	//cout << "[DEBUG] MODE_CHECK_RESULTS scanning for duplicates" << endl;

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
				//cout << "[DEBUG] MODE_CHECK_RESULTS got a symmetric relation between " << i->first;
				//cout << " and " << j->first << endl;

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

	//cout << "[DEBUG] MODE_CHECK_RESULTS gossiping begins..." << endl;

	m_mode = MODE_SHARE_WITH_LINKED_ACTORS;


}

void SpuriousSeedAnnihilator::shareWithLinkedActors() {

	// find a information that was not shared with a given actor
	// this is a good-enough implementation
	// this is like a gossip algorithm, but here the pairs are not randomly
	// selected, they are obtained from the relationships computed
	// in the de Bruijn graph.

	/*
	m_mode = MODE_STOP_THIS_SITUATION; // remove this
	return; // remove this
	*/

	if(m_inbox->hasMessage(RAY_MESSAGE_TAG_SEED_GOSSIP_REPLY)) {

		m_activeQueries --;

#ifdef CONFIG_ASSERT
		assert(m_activeQueries >= 0);
#endif // CONFIG_ASSERT
	}

	/*
	 * Add a messaging regulator here.
	 */
	int maximumActiveQueries =16;

	if(m_core->getSize() / 2 < maximumActiveQueries)
		maximumActiveQueries = m_core->getSize() / 2;

	if(maximumActiveQueries < 2)
		maximumActiveQueries = 2;

	if(m_inbox->hasMessage(RAY_MESSAGE_TAG_SEED_GOSSIP)) {
		Message * message = m_inbox->at(0);

		// send the response now because the return
		// statements below are evil.
		// anyway, RayPlatform will only send the message in
		// its sendMessages() call in its main loop so
		// it does not change anything whatsoever.

		this->m_core->getSwitchMan()->sendEmptyMessage(m_outbox, m_rank, message->getSource(),
			RAY_MESSAGE_TAG_SEED_GOSSIP_REPLY);

		int availableUnits = message->getCount();

		// if the message is empty, we have nothing to do.
		// this is likely because the object was too large
		// to be transported.
		if(availableUnits == 0) {
			return;
		}

		GraphSearchResult gossip;
		int position = 0;
		char * buffer = (char *) message->getBuffer();

		int bytes = gossip.load(buffer + position);
		position += bytes;

		// first check if the gossip is already known.
		// this implementation is good enough, but it could
		// use an index.

		string key = gossip.toString();

		bool found = m_gossipIndex.count(key) > 0;

		if(found) {
			return;
		}

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

		//cout << "[DEBUG] MODE_SHARE_WITH_LINKED_ACTORS Rank rank:" << m_rank << " received gossip gossip:" << key << " from rank rank:" << actor << endl;

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

		if(m_activeQueries > 0)
			return;

		// check delay and latency
		time_t currentTime = time(NULL);

		int minimumWaitTime = 10; // seconds
		int distance = currentTime - m_lastGossipingEventTime;

		if(distance < minimumWaitTime)
			return;

		//cout << "[DEBUG] MODE_SHARE_WITH_LINKED_ACTORS Rank " << m_rank << " gossips have spreaded." << endl;

		m_gossipStatus.clear();
		m_linkedActorsForGossip.clear();

		// synchronize indexes in m_indexesToShareWithArbiter
		//m_mode = MODE_EVALUATE_GOSSIPS;
		m_mode = MODE_SHARE_PUSH_DATA_IN_KEY_VALUE_STORE;

		return;
	}

	// at this point, we have messages of type
	// RAY_MESSAGE_TAG_SEED_GOSSIP to send.

	// \\ messaging regulator
	if(m_activeQueries >= maximumActiveQueries)
		return;

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

			char *messageBuffer = (char *)m_outboxAllocator->allocate(MAXIMUM_MESSAGE_SIZE_IN_BYTES);

#ifdef ASSERT_CONFIG
			assert( messageBuffer != NULL );
#endif /* ASSERT_CONFIG */

			int position = 0;
			GraphSearchResult & gossip =  /*&*/ m_gossips[gossipIndex];

			int requiredBytes = gossip.getRequiredNumberOfBytes();

			int availableBytes = MAXIMUM_MESSAGE_SIZE_IN_BYTES - position;

			if(availableBytes < requiredBytes) {
				cout << "Error, object is too large to be transported." << endl;
			} else {
				position += gossip.dump(messageBuffer + position);
			}

			int units = position / sizeof(MessageUnit);

			if(position % sizeof(MessageUnit))
				units ++;

			string key = gossip.toString();

			Message aMessage((MessageUnit*)messageBuffer, units, actor,
				RAY_MESSAGE_TAG_SEED_GOSSIP, m_rank);
			m_outbox->push_back(&aMessage);

			m_activeQueries ++;

			m_gossipStatus[gossipIndex].insert(actor);

			//cout << "[DEBUG] MODE_SHARE_WITH_LINKED_ACTORS Rank rank:" << m_rank << " sent gossip gossip:" << key << " from rank rank:" << actor << endl;

			return;
		}
	}

	// at this point, we tried every gossip and they are all synchronized.

	m_hasNewGossips = false;


}

void SpuriousSeedAnnihilator::call_RAY_SLAVE_MODE_PROCESS_MERGING_ASSETS() {

	initializeMergingProcess();

	if(m_mustAdviseRanks) {
		if(m_rankToAdvise < m_core->getSize()) {

			this->m_core->getSwitchMan()->sendEmptyMessage(m_outbox, m_rank, m_rankToAdvise, RAY_MESSAGE_TAG_ARBITER_SIGNAL);
			m_rankToAdvise ++;
		} else {
			m_synced = 0;
			m_mustAdviseRanks = false;

			//cout << "[DEBUG] everybody received the arbiter advise" << endl;
		}
	}

	if (m_mode == MODE_SPREAD_DATA) {

		spreadAcquiredData();

	} else if(m_mode == MODE_WAIT_FOR_ARBITER) {

		if(m_inbox->hasMessage(RAY_MESSAGE_TAG_ARBITER_SIGNAL)) {

			//cout << "[DEBUG] arbiter advises to continue" << endl;

			m_mode = m_nextMode;
		}

	} else if(m_mode == MODE_CHECK_RESULTS) {

		checkResults();

	} else if(m_mode == MODE_SHARE_WITH_LINKED_ACTORS) {

		shareWithLinkedActors();

	} else if(m_mode == MODE_SHARE_PUSH_DATA_IN_KEY_VALUE_STORE) {

		pushDataInKeyValueStore();

	} else if(m_mode == MODE_EVALUATE_GOSSIPS) {

		//cout << "DEBUG Calling evaluateGossips" << endl;
		evaluateGossips();

	} else if(m_mode == MODE_REBUILD_SEED_ASSETS) {

		rebuildSeedAssets();

	} else if(m_mode == MODE_GENERATE_NEW_SEEDS) {

		generateNewSeeds();

	} else if(m_mode == MODE_CLEAN_KEY_VALUE_STORE) {

		cleanKeyValueStore();

	} else if(m_mode == MODE_GATHER_COVERAGE_VALUES) {

		gatherCoverageValues();

	} else if(m_mode == MODE_STOP_THIS_SITUATION) {

		m_core->closeSlaveModeLocally();
	}

}

void SpuriousSeedAnnihilator::getSeedKey(PathHandle & handle, string & keyObject) {

	ostringstream key;

	// something like
	//
	// /seeds/1000012

	key << "/seeds/" << handle;

	keyObject = key.str();

}

void SpuriousSeedAnnihilator::pushDataInKeyValueStore() {

	for(int i = 0 ; i  < (int)m_seeds->size() ; ++i) {

		GraphPath & seed = m_seeds->at(i);

		PathHandle handle = getPathUniqueId(m_core->getRank(), i);

		string keyObject;
		getSeedKey(handle, keyObject);

		// obviously content is just a test...

		int bytes = seed.getRequiredNumberOfBytes();
		char * content  = m_core->getKeyValueStore().allocateMemory(bytes);
		seed.dump(content);

		if(!(m_core->getKeyValueStore().insertLocalKey(keyObject, content, bytes))) {

			// failed to insert key
			// we should catch the error...
			//
			// For example, it will return false if the key is already there.
		}
	}

	sendMessageToArbiter();
	m_nextMode = MODE_EVALUATE_GOSSIPS;

#ifdef DEBUG_SEED_MERGING
	cout << "[DEBUG] wait before evaluating gossips..." << endl;
#endif
}

void SpuriousSeedAnnihilator::rebuildSeedAssets() {

	//cout << "DEBUG rebuildSeedAssets" << endl;

	//string testKey = "/seeds/115000022";
	//Rank testRank = 22;

	/*
	string testKey = "/seeds/19";
	Rank testRank = 19;
	*/

	/*
	string testKey = "/seeds/60000001";
	Rank testRank = 1;
	*/

	if(!m_initialized) {

		//cout << "DEBUG rebuildSeedAssets initializing..." << endl;

		m_seedIndex = 0;
		m_pathIndex = 0;
		m_location = 0;

		/*
		 * This was for testing purposes.
		 *
		 * The API of RayPlatform works great.
		 */
		//m_core->getKeyValueStore().pullRemoteKey(testKey, testRank, m_request);

		m_initialized = true;

	/*
	} else if(!m_core->getKeyValueStore().test(m_request)) {
	*/

	} else if(m_seedIndex < (int)m_newSeedBluePrints.size()) {

#if 0
		m_seedIndex ++;
		return;
#endif

		vector<PathHandle> & pathHandles = m_newSeedBluePrints[m_seedIndex].getPathHandles();


		if(m_pathIndex < (int)pathHandles.size()) {

			string key = "";
			PathHandle & handle = pathHandles[m_pathIndex];
			getSeedKey(handle, key);
			Rank rank = getRankFromPathUniqueId(handle);

			if(!m_messageWasSent) {

#if 0
				cout << "DEBUG downloading keys for this blueprint: " << endl;
				m_newSeedBluePrints[m_seedIndex].print();
				cout << endl;

				cout << "[DEBUG] downloading key " << key << " from rank " << rank;
				cout << endl;
#endif

				m_core->getKeyValueStore().pullRemoteKey(key, rank, m_request);

				m_messageWasSent = true;

			} else if(m_core->getKeyValueStore().test(m_request)) {

				/*
				 * The transfer is completed.
				 */


				char * value = NULL;
				int size = 0;
				m_core->getKeyValueStore().getLocalKey(key, value, size);

#if 0
				cout << "[DEBUG] transfer is now completed for key " << key << " ";
				cout << " " << size << " bytes" << endl;
#endif

				m_pathIndex ++;
				m_messageWasSent = false;
			}

		} else {
			m_seedIndex++;

			m_pathIndex = 0;
		}

	} else {

#if 0
		/**
		 *
		 * This code was useful to test the transport of
		 * data.
		 */
		char * value = NULL;
		int valueLength = 0;
		m_core->getKeyValueStore().getLocalKey(testKey, &value, &valueLength);


		cout << "[DEBUG] Rank " << m_core->getRank() << " downloaded key ";
		cout << testKey << " from " << testRank;
		cout << ", value length is " << valueLength << " bytes ";

		cout << " CRC32= 0-3999 .. ";
		cout << computeCyclicRedundancyCode32((uint8_t*) value, (uint32_t)4000);

		cout << " CRC32= 4000- .. ";
		cout << computeCyclicRedundancyCode32((uint8_t*) value + 4000, (uint32_t)valueLength - 4000);

		cout << " CRC32= ";
		cout << computeCyclicRedundancyCode32((uint8_t*) value, (uint32_t)valueLength);

		cout << endl;
#endif

		sendMessageToArbiter();
		m_nextMode = MODE_GENERATE_NEW_SEEDS;

	}
}


void SpuriousSeedAnnihilator::generateNewSeeds() {

	m_initialized = false;
	m_seedIndex = 0;
	m_pathIndex = 0;
	m_location = 0;

	// we have everything we need in the key-value store now.

	int oldCount = m_seeds->size();

	m_seeds->clear();

	for(int metaSeedIndex = 0 ; metaSeedIndex < (int) m_newSeedBluePrints.size() ;
			++ metaSeedIndex) {

#if 0
		cout << "[DEBUG] ***" << endl;
		cout << "DEBUG generateNewSeeds metaSeedIndex= " << metaSeedIndex << endl;
		cout << " blueprint ";
		m_newSeedBluePrints[metaSeedIndex].print();
		cout << endl;
#endif

		GraphPath emptyPath;
		emptyPath.setKmerLength(m_parameters->getWordSize());

		vector<PathHandle> & handles = m_newSeedBluePrints[metaSeedIndex].getPathHandles();
		vector<bool> & orientations = m_newSeedBluePrints[metaSeedIndex].getPathOrientations();
		vector<GraphPath> & gaps = m_newSeedBluePrints[metaSeedIndex].getComputedPaths();

		for(int pathHandleIndex = 0 ;
				pathHandleIndex < (int) handles.size(); ++ pathHandleIndex) {

			PathHandle & handle = handles[pathHandleIndex];
			bool orientation = orientations[pathHandleIndex];

			char * value = NULL;
			int valueSize = 0;

			string key = "";
			getSeedKey(handle, key);

			m_core->getKeyValueStore().getLocalKey(key, value, valueSize);

#if 0
			cout << "DEBUG generateNewSeeds getLocalKey on key " << key << " -> " << valueSize << " bytes";
#endif

			GraphPath seedPath;
			seedPath.load(value);

			if(orientation) {
				GraphPath newPath;
				seedPath.reverseContent(newPath);
				seedPath = newPath;
			}

#if 0
			cout << "DEBUG append path " << pathHandleIndex << " " << handle;
			cout << " container " << emptyPath.size() << " object " << seedPath.size() << endl;
#endif


			emptyPath.appendPath(seedPath);

			// if it is not the last one, append the gap
			// too
			if(pathHandleIndex != (int) handles.size() -1) {

				GraphPath & gapPath = gaps[pathHandleIndex];

#if 0
				cout << "DEBUG append gap path ";
				cout << "container " << emptyPath.size() << " object " << gapPath.size() << endl;
#endif
				emptyPath.appendPath(gapPath);
			}
		}

		// add the new longer seed...

		emptyPath.reserveSpaceForCoverage();

		m_seeds->push_back(emptyPath);
	}

	// rebuild seeds here for real

	int newCount = m_seeds->size();

	// sort the seeds by length
	vector<GraphPath> & seeds = (*m_seeds);

	std::sort(seeds.begin(),
		seeds.end(), comparePaths);


	cout << "Rank " << m_core->getRank() << " merged its seeds: " << oldCount << " seeds -> ";
	cout << newCount << " seeds" << endl;

	sendMessageToArbiter();
	m_nextMode = MODE_CLEAN_KEY_VALUE_STORE;
}

void SpuriousSeedAnnihilator::cleanKeyValueStore() {

	m_core->getKeyValueStore().clear();

	m_seedIndex = 0;
	m_seedPosition = 0;
	m_mode = MODE_GATHER_COVERAGE_VALUES;
}

void SpuriousSeedAnnihilator::gatherCoverageValues() {

	/*
	 * We need to gather coverage values here.
	 *
	 * The message tag is RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE.
	 *
	 * For each kmer we put in the message, we will receive a coverage value.
	 *
	 * For this, we will use a workflow.
	 */

	if(m_inbox->hasMessage(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY)) {

		Message * message = m_inbox->at(0);

		Rank source = message->getSource();

		MessageUnit * buffer = message->getBuffer();

		int count = message->getCount();

		for(int i = 0 ; i < count ; ++i) {

			int seedIndex = m_buffersForPaths->getAt(source, i);
			int positionIndex = m_buffersForPositions->getAt(source, i);

#ifdef CONFIG_ASSERT
			assert(seedIndex < (int) m_seeds->size());
			assert(positionIndex < (int) m_seeds->at(seedIndex).size());
#endif

			CoverageDepth coverage = buffer[i];

#ifdef CONFIG_ASSERT
			assert(coverage > 0);
#endif

			GraphPath & seed = m_seeds->at(seedIndex);

			seed.setCoverageValueAt(positionIndex, coverage);

#if 0
			cout << "[DEBUG] gatherCoverageValues operation: SetCoverage(path: " << seedIndex;
			cout << " position: " << positionIndex << " value: " << coverage << endl;
#endif
		}

		// we don't need to clear the buffers for m_buffersForMessages because
		// the flush (or flushAll) call did that for us
		m_buffersForPaths->reset(source);
		m_buffersForPositions->reset(source);
		m_pendingMessages--;
	}

	if(m_pendingMessages)
		return;

	if(m_seedIndex < (int)m_seeds->size()) {

		GraphPath & seed = m_seeds->at(m_seedIndex);

		if(m_seedPosition < seed.size()) {

			if(m_seedIndex == 0 && m_seedPosition == 0) {

				m_buffersForMessages = new BufferedData();
				m_buffersForPaths = new BufferedData();
				m_buffersForPositions = new BufferedData();

				m_buffersForMessages->constructor(m_core->getSize(),
						MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),
						"/dev/memory-for-messages", m_parameters->showMemoryAllocations(), KMER_U64_ARRAY_SIZE);

				m_buffersForPaths->constructor(m_core->getSize(),
						MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),
						"/dev/memory-for-paths", m_parameters->showMemoryAllocations(), KMER_U64_ARRAY_SIZE);

				m_buffersForPositions->constructor(m_core->getSize(),
						MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(MessageUnit),
						"/dev/memory-for-positions", m_parameters->showMemoryAllocations(), KMER_U64_ARRAY_SIZE);

				m_pendingMessages = 0;
			}

			Kmer kmer;
			seed.at(m_seedPosition, &kmer);

			Rank rankToFlush = kmer.vertexRank(m_parameters->getSize(),m_parameters->getWordSize(),
				m_parameters->getColorSpaceMode());

			for(int i=0;i<KMER_U64_ARRAY_SIZE;i++){
				m_buffersForMessages->addAt(rankToFlush, kmer.getU64(i));
				m_buffersForPaths->addAt(rankToFlush, m_seedIndex);
				m_buffersForPositions->addAt(rankToFlush, m_seedPosition);
			}

			if(m_buffersForMessages->flush(rankToFlush, KMER_U64_ARRAY_SIZE, RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,
				m_outboxAllocator, m_outbox,
				m_rank, false)){

				m_pendingMessages++;
			}

			// do something with this kmer now...
			// possibly use BufferedData

			m_seedPosition ++;
		} else {
			m_seedIndex ++;
			m_seedPosition = 0;
		}
	} else if (!m_buffersForMessages->isEmpty()) {

		// flush the remaining bits
		m_pendingMessages += m_buffersForMessages->flushAll(RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,
			m_outboxAllocator, m_outbox, m_rank);

	} else {

		m_buffersForMessages->clear();
		m_buffersForPaths->clear();
		m_buffersForPositions->clear();

		m_buffersForMessages = NULL;
		m_buffersForPaths = NULL;
		m_buffersForPositions = NULL;

		for(int i = 0 ; i < (int)m_seeds->size() ; ++i) {
			m_seeds->at(i).computePeakCoverage();
		}
		m_mode = MODE_STOP_THIS_SITUATION;
	}
}

void SpuriousSeedAnnihilator::call_RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION(Message*message) {

	void*buffer=message->getBuffer();
	Rank source=message->getSource();
	MessageUnit*incoming=(MessageUnit*)buffer;
	int count=message->getCount();
	MessageUnit*message2=(MessageUnit*)m_outboxAllocator->allocate(count*sizeof(MessageUnit));

	for(int i=0;i<count;i+= (KMER_U64_ARRAY_SIZE + 1)){

		int position = incoming[i];
		Kmer vertex;
		int bufferPosition=i;
		vertex.unpack(incoming + 1, &bufferPosition);

		Vertex*node=m_subgraph->find(&vertex);

		// if it is not there, then it has a coverage of 0
		CoverageDepth coverage=0;

		if(node!=NULL){
			coverage=node->getCoverage(&vertex);

			#ifdef CONFIG_ASSERT
			assert(coverage!=0);
			#endif
		}

		message2[i] = position;
		message2[i + 1] = coverage;
	}

	Message aMessage(message2, count, source,
		RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION_REPLY, m_rank);

	m_outbox->push_back(&aMessage);
}

void SpuriousSeedAnnihilator::evaluateGossips() {

#if 0
	cout << "[DEBUG] evaluateGossips" << endl;
#endif

	/**
	 * Here, we do that in batch.
	 */

	m_seedGossipSolver.setInput(&m_gossips);
	m_seedGossipSolver.compute();

	vector<GraphSearchResult> & solution = m_seedGossipSolver.getSolution();

	//cout << "[DEBUG] MODE_EVALUATE_GOSSIPS Rank " << m_rank << " gossip count: " << m_gossips.size() << endl;
	//cout << "[DEBUG] MODE_EVALUATE_GOSSIPS solution has " << solution.size() << " entries !" << endl;

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

	// current seeds are stored in m_seeds
	// new seeds are in solution;
	// to make this a generalized process, let's add the seeds that are not in the solution
	// in a new solution

	set<PathHandle> localPathsInSolution;

	for(vector<GraphSearchResult>::iterator i = solution.begin();
			i != solution.end() ; ++i) {

		GraphSearchResult & result = *i;

#if 0
		cout << "DEBUG inspecting solution entry" << endl;
		result.print();
		cout << endl;
#endif

		vector<PathHandle> & handles = result.getPathHandles();

		for(vector<PathHandle>::iterator j = handles.begin() ; j != handles.end() ; ++j) {

			PathHandle & thePath = *j;
			localPathsInSolution.insert(thePath);
		}
	}

	//cout << "[DEBUG] MODE_EVALUATE_GOSSIPS " << solution.size() << " entries need an owner." << endl;

	for(vector<GraphSearchResult>::iterator i = solution.begin() ;
			i!= solution.end() ; ++i) {

		GraphSearchResult & entry = *i;

#if 0
		cout << "DEBUG inspecting again this solution entry" << endl;
		entry.print();
		cout << endl;
#endif

		PathHandle & firstHandle = entry.getPathHandles()[0];
		PathHandle & lastHandle = entry.getPathHandles()[entry.getPathHandles().size()-1];

		// this algorithm is stupid because it does not enforce
		// load balancing.
		//
		// TODO: implement a true load balancing algorithme here...

		PathHandle smallest = firstHandle;
		if(lastHandle < firstHandle)
			smallest = lastHandle;

		Rank owner = getRankFromPathUniqueId(smallest);

		if(owner == m_core->getRank()) {
			m_newSeedBluePrints.push_back(entry);

#if 0
			// The bug is here already for the blueprint
			cout << "DEBUG adding this blueprint:" << endl;
			entry.print();
			cout << endl;
#endif
		}
	}

	//cout << "[DEBUG] MODE_EVALUATE_GOSSIPS Rank " << m_core->getRank() << " claimed ownership for " << m_newSeedBluePrints.size();

#ifdef DEBUG_SEED_MERGING
	cout << " before merging its own assets in the pool." << endl;
#endif

	// this needs to run after trimming those short seeds with dead-ends

	// add local seeds that are not in the local solution
	// a local seeds can't be in a remote solution if it is in
	// the local solution anyway
	for(int i = 0 ; i < (int) m_seeds->size() ; ++i) {

		PathHandle identifier = getPathUniqueId(m_parameters->getRank(), i);

		if(localPathsInSolution.count(identifier) == 0) {

			GraphSearchResult result;
			result.addPathHandle(identifier, false);

			m_newSeedBluePrints.push_back(result);
		}
	}

	//cout << "[DEBUG] MODE_EVALUATE_GOSSIPS Rank " << m_core->getRank() << " will assume ownership for " << m_newSeedBluePrints.size();
	//cout << " objects, had " << m_seeds->size() << " before merging" << endl;

#if 0
	int index = 0;
	for(vector<GraphSearchResult>::iterator i = m_newSeedBluePrints.begin() ;
			i != m_newSeedBluePrints.end() ; ++i) {
		cout << "[DEBUG] OWNED OBJECT @" << index++ << " ";
		(*i).print();
		cout << endl;
	}
#endif

	//cout << "DEBUG next = MODE_REBUILD_SEED_ASSETS" << endl;

	m_mode = MODE_REBUILD_SEED_ASSETS;
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

		//cout << "[DEBUG] call_RAY_SLAVE_MODE_REGISTER_SEEDS: initializing..." << endl;

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

		// TODO: don't store coverage in seed checkpoints...

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
	m_core->setMessageTagSymbol(m_plugin, RAY_MESSAGE_TAG_ARBITER_SIGNAL, "RAY_MESSAGE_TAG_ARBITER_SIGNAL");

	RAY_MESSAGE_TAG_SEED_GOSSIP = m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin, RAY_MESSAGE_TAG_SEED_GOSSIP, "RAY_MESSAGE_TAG_SEED_GOSSIP");

	RAY_MESSAGE_TAG_SEED_GOSSIP_REPLY = m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin, RAY_MESSAGE_TAG_SEED_GOSSIP_REPLY,
			"RAY_MESSAGE_TAG_SEED_GOSSIP_REPLY");

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
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_SAY_HELLO_TO_ARBITER);
	__ConfigureMessageTagHandler(SpuriousSeedAnnihilator, RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION);

	RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY = m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin, RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY, "RAY_MESSAGE_TAG_SEND_SEED_LENGTHS_REPLY");

	RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION_REPLY = m_core->allocateMessageTagHandle(m_plugin);
	m_core->setMessageTagSymbol(m_plugin, RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION_REPLY,
			"RAY_MESSAGE_TAG_REQUEST_VERTEX_COVERAGE_WITH_POSITION_REPLY");


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
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE");
	RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY = m_core->getMessageTagFromSymbol(m_plugin, "RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY");

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
