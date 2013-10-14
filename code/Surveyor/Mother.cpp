// Written by <<sebhtml>>
// 2013-10-10

#include "Mother.h"
#include "StoreKeeper.h"
#include "GenomeGraphReader.h"

#include <RayPlatform/cryptography/crypto.h>

#include <iostream>
using namespace std;

Mother::Mother() {

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

			stop();
		}
	}

}

void Mother::stop() {

	Message kill;
	kill.setTag(CoalescenceManager::DIE);

	send(m_coalescenceManager->getName(), kill);

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
	uint32_t checksum = computeCyclicRedundancyCode32((uint8_t*) message.getBufferBytes(),
			message.getNumberOfBytes());
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

/*

	// spawn actors for storing the graph.
	for(int i = 0 ; i < PLAN_STORE_KEEPER_ACTORS_PER_RANK; ++i) {

		StoreKeeper * actor = new StoreKeeper();
		spawn(actor);

		m_storeKeepers.push_back(actor->getName());
	}
	*/

	m_coalescenceManager = new CoalescenceManager();
	spawn(m_coalescenceManager);

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

		string & fileName = m_graphFileNames[m_filesToSpawn[m_fileIterator]];
		m_fileIterator++;

		GenomeGraphReader * actor = new GenomeGraphReader();
		spawn(actor);
		actor->setFileName(fileName);

		int destination = actor->getName();

		Message dummyMessage;

		int coalescenceManagerName = m_coalescenceManager->getName();
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

		stop();
	}
}

void Mother::setParameters(Parameters * parameters) {
	m_parameters = parameters;
}
