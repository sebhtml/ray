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

	for(int i = 0 ; i < PLAN_STORE_KEEPER_ACTORS_PER_RANK; ++i) {

		StoreKeeper * actor = new StoreKeeper();
		spawn(actor);

		m_storeKeepers.push_back(actor->getName());
	}

	for(int i = 0 ; i < PLAN_GENOME_GRAPH_READER_ACTORS_PER_RANK; ++i) {

		GenomeGraphReader * actor = new GenomeGraphReader();
		spawn(actor);

		m_readers.push_back(actor->getName());
	}
}

void Mother::setParameters(Parameters * parameters) {
	m_parameters = parameters;
}
