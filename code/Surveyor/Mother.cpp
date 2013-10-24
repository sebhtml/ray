/*
    Copyright 2013 Sébastien Boisvert
    Copyright 2013 Université Laval
    Copyright 2013 Centre Hospitalier Universitaire de Québec

    This file is part of Ray Surveyor.

    Ray Surveyor is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    Ray Surveyor is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ray Surveyor.  If not, see <http://www.gnu.org/licenses/>.
*/


// Written by <<sebhtml>>
// 2013-10-10

#include "Mother.h"
#include "StoreKeeper.h"
#include "GenomeGraphReader.h"

#include <RayPlatform/cryptography/crypto.h>

#include <iostream>
using namespace std;

#define PLAN_RANK_ACTORS_PER_RANK 1
#define PLAN_MOTHER_ACTORS_PER_RANK 1
#define PLAN_GENOME_GRAPH_READER_ACTORS_PER_RANK 999999



Mother::Mother() {

	m_finishedMothers = 0;
	//cout << "DEBUG Mother constructor" << endl;
}

Mother::~Mother() {

}

void Mother::receive(Message & message) {

	int tag = message.getTag();

	if(tag == Actor::BOOT) {

		boot(message);
	} else if (tag == Mother::HELLO) {
		hello(message);

	} else if(tag == GenomeGraphReader::START_PARTY_OK) {

		// spawn the next reader now !

		/*
		printName();
		cout << "DEBUG spawnReader because START_PARTY_OK" << endl;
*/
		spawnReader();

	} else if(tag == GenomeGraphReader::DONE) {

		m_aliveReaders--;

		//cout << "DEBUG received GenomeGraphReader::DONE remaining " << m_aliveReaders << endl;

		if(m_aliveReaders == 0) {

			notifyController();
		}

	} else if(tag == SHUTDOWN) {

		Message response;
		response.setTag(SHUTDOWN_OK);
		send(message.getSourceActor(), response);

		stop();
		
	} else if(tag == FINISH_JOB) {

		m_finishedMothers++;

		if(m_finishedMothers == getSize()) {

			// all readers have finished,
			// now tell mother to flush aggregators

			sendToFirstMother(SHUTDOWN, SHUTDOWN_OK);
		}

	} else if(tag == m_responseTag) {

		// every mother was informed.
		if(m_motherToKill < getSize())
			return;

		sendMessageWithReply(m_motherToKill, m_forwardTag);
		m_motherToKill--;
	}
}

void Mother::sendToFirstMother(int forwardTag, int responseTag) {

	m_forwardTag = forwardTag;
	m_responseTag = responseTag;

	m_motherToKill = 2 * getSize() - 1;

	sendMessageWithReply(m_motherToKill, forwardTag);
	m_motherToKill--;
}

void Mother::sendMessageWithReply(int & actor, int tag) {
/*
	printName();
	cout << "kills Mother " << actor << endl;
	*/

	Message message;
	message.setTag(tag);
	send(actor, message);
}

void Mother::notifyController() {
	Message message2;
	message2.setTag(FINISH_JOB);

	// first Mother
	int controller = getSize();

	printName();
	cout << "Mother notifies controller " << controller << endl;
	send(controller, message2);

}

void Mother::stop() {

	Message kill;
	kill.setTag(CoalescenceManager::DIE);

	send(m_coalescenceManager, kill);

	send(m_storeKeepers[0], kill);

	m_storeKeepers.clear();
	m_coalescenceManager = -1;

	die();

}

void Mother::hello(Message & message) {
	/*
	printName();
	cout << "received HELLO from ";
	cout << message.getSourceActor();
	cout << " bytes: " << message.getNumberOfBytes();
	cout << " content: " << *((int*) message.getBufferBytes());
	*/

	//char * buffer = (char*) message.getBufferBytes();
	/*
	uint32_t checksum = computeCyclicRedundancyCode32((uint8_t*) message.getBufferBytes(),
			message.getNumberOfBytes());
			*/
	//cout << "DEBUG CRC32= " << checksum << endl;

	//cout << endl;
}

void Mother::boot(Message & message) {

	m_aliveReaders = 0;

	/*
	printName();
	cout << "Mother is booting and says hello" << endl;
*/
	Message message2;
	/*
	char joe[4000];

	int i = 4000;
	char value = 0;
	while(i)
		joe[i--]=value++;
*/

	int joe = 9921;

	message2.setBuffer(&joe);
	//message2.setNumberOfBytes(4000);
	message2.setNumberOfBytes( sizeof(int) * 1 );

	//cout << "DEBUG sending " << joe << endl;

	/*
	uint32_t checksum = computeCyclicRedundancyCode32((uint8_t*) message2.getBufferBytes(),
		       message2.getNumberOfBytes()	);
	cout << "DEBUG CRC32= " << checksum << endl;
*/

	message2.setTag(Mother::HELLO);

	int next = getName() + 1;

	next %= (getSize() * 2);

	if(next < getSize())
		next += getSize();

	printName();
	cout << " local Mother is " << getName() << ",";
	cout << " friend is # " << next << endl;

	send(next, message2);

	if(m_parameters->hasOption("-run-surveyor")) {
		startSurveyor();
	} else {
		die();
	}
}

void Mother::startSurveyor() {

	bool isRoot = (getName() % getSize()) == 0;

	//cout << "DEBUG startSurveyor isRoot" << isRoot << endl;

	// get a list of files.

	vector<string> * commands = m_parameters->getCommands();

	for(int i = 0 ; i < (int) commands->size() ; ++i) {

		string & element = commands->at(i);

		if(element == "-read-sample-graph") {

			string sampleName = commands->at(++i);
			string fileName = commands->at(++i);

			m_sampleNames.push_back(sampleName);
			m_graphFileNames.push_back(fileName);
		}
	}

	if(isRoot) {
		printName();
		cout << "samples= " << m_sampleNames.size() << endl;
	}


	CoalescenceManager * coalescenceManager = new CoalescenceManager();
	spawn(coalescenceManager);

	m_coalescenceManager = coalescenceManager->getName();

	// spawn the local store keeper and introduce the CoalescenceManager
	// to the StoreKeeper

	// spawn actors for storing the graph.
	for(int i = 0 ; i < PLAN_STORE_KEEPER_ACTORS_PER_RANK; ++i) {

		StoreKeeper * actor = new StoreKeeper();
		spawn(actor);

		m_storeKeepers.push_back(actor->getName());

		// tell the CoalescenceManager about the local StoreKeeper
		Message dummyMessage;
		int localStore = actor->getName();

		dummyMessage.setBuffer(&localStore);
		dummyMessage.setNumberOfBytes( sizeof(int) );

		dummyMessage.setTag(CoalescenceManager::INTRODUCE_STORE);

		send(m_coalescenceManager, dummyMessage);

		int kmerLength = m_parameters->getWordSize();

		// send the kmer to local store
		Message kmerMessage;
		kmerMessage.setBuffer(&kmerLength);
		kmerMessage.setNumberOfBytes(sizeof(kmerLength));
		kmerMessage.setTag(CoalescenceManager::SET_KMER_LENGTH);
		send(localStore, kmerMessage);

	}


	// spawn an actor for each file that this actor owns

	for(int i = 0; i < (int) m_graphFileNames.size() ; ++i) {

		int mother = getName() % getSize();
		int fileMother = i % getSize();

		if(mother == fileMother) {

			m_filesToSpawn.push_back(i);

		}
	}

	printName();
	cout << " readers to spawn: " << m_filesToSpawn.size() << endl;

	m_fileIterator = 0;
	spawnReader();
}

void Mother::spawnReader() {

	if(m_fileIterator < (int) m_filesToSpawn.size()) {

		int sampleIdentifier = m_filesToSpawn[m_fileIterator];

		string & fileName = m_graphFileNames[sampleIdentifier];
		m_fileIterator++;

		GenomeGraphReader * actor = new GenomeGraphReader();
		spawn(actor);
		actor->setFileName(fileName, sampleIdentifier);

		int destination = actor->getName();
		Message dummyMessage;

		int coalescenceManagerName = m_coalescenceManager;
		//cout << "DEBUG coalescenceManagerName is " << coalescenceManagerName << endl;

		dummyMessage.setBuffer(&coalescenceManagerName);
		dummyMessage.setNumberOfBytes( sizeof(int) );

		dummyMessage.setTag(GenomeGraphReader::START_PARTY);

		/*
		printName();
		cout << " sending START_PARTY " << GenomeGraphReader::START_PARTY;
		cout << " to " << destination << endl;
*/

		m_aliveReaders ++;

		send(destination, dummyMessage);
	}

	if(m_aliveReaders == 0) {

		notifyController();
	}
}

void Mother::setParameters(Parameters * parameters) {
	m_parameters = parameters;
}
