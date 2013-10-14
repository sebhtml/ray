// Written by <<sebhtml>>
// 2013-10-10

#include "Mother.h"
#include "StoreKeeper.h"
#include "GenomeGraphReader.h"

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
	}

}

void Mother::hello(Message & message) {
	printName();
	cout << "received HELLO from ";
	cout << message.getSourceActor();
	cout << endl;
}

void Mother::boot(Message & message) {

	printName();
	cout << "Mother is booting and says hello" << endl;

	Message message2;
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
	}
}

void Mother::startSurveyor() {

	bool isRoot = (getName() % getSize()) == 0;

	cout << "DEBUG startSurveyor isRoot" << isRoot << endl;

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
		cout << "DEBUG samples= " << m_sampleNames.size() << endl;
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
	cout << "DEBUG readers to spawn: " << m_filesToSpawn.size() << endl;

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

		dummyMessage.setTag(GenomeGraphReader::START_PARTY);
		send(destination, dummyMessage);
	}
}

void Mother::setParameters(Parameters * parameters) {
	m_parameters = parameters;
}
