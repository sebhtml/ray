
#include "CoalescenceManager.h"

#include <iostream>
using namespace std;

CoalescenceManager::CoalescenceManager() {

}

CoalescenceManager::~CoalescenceManager() {

}

void CoalescenceManager::receive(Message & message) {

	int tag = message.getTag();

	/*
	printName();
	cout << " CoalescenceManager DEBUG receive message !";
	cout << endl;
*/

	if(tag == PAYLOAD) {

		receivePayload(message);

	} else if(tag == DIE) {

		die();
	}
}

void CoalescenceManager::receivePayload(Message & message) {

	int source = message.getSourceActor();

	Message response;
	response.setTag(PAYLOAD_RESPONSE);

	send(source, response);
}
