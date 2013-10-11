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

void Mother::receive(Message * message) {
	
	int tag = message->getTag();

	if(tag == Actor::BOOT) {
	
		boot(message);
	}

}

void Mother::boot(Message * message) {

	printName();
	cout << "Mother is booting and says hello" << endl;
}
