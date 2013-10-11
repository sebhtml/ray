// Written by <<sebhtml>>
// 2013-10-10

#include "Mother.h"

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
}
