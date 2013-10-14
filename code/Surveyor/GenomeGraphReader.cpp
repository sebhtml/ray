
#include "GenomeGraphReader.h"
#include "CoalescenceManager.h"

#include <iostream>
using namespace std;

#include <string.h>

GenomeGraphReader::GenomeGraphReader() {

}

GenomeGraphReader::~GenomeGraphReader() {

}

void GenomeGraphReader::receive(Message & message) {

	int type = message.getTag();

	/*
	printName();
	cout << "received tag " << type << endl;
*/

	if(type == START_PARTY) {
		startParty(message);

	} else if(type == CoalescenceManager::PAYLOAD_RESPONSE) {

		/*
		printName();
		cout << " DEBUG readLine because PAYLOAD_RESPONSE" << endl;
		*/
		// read the next line now !
		readLine();
	}
}

void GenomeGraphReader::startParty(Message & message) {

	char * buffer = (char*) message.getBufferBytes();

	memcpy(&m_aggregator, buffer, sizeof(int));
	//m_aggregator = *(int*)(message.getBufferBytes());

	m_reader.open(m_fileName.c_str());
	m_loaded = 0;

	m_parent = message.getSourceActor();

	/*
	printName();
	cout << "DEBUG startParty" << endl;
	cout << " bytes in message: " << message.getNumberOfBytes();
	cout << " must send messages to aggregator " << m_aggregator;
	cout << endl;
*/

	int source = message.getSourceActor();
	Message response;
	response.setTag(START_PARTY_OK);

	send(source, response);

	readLine();
}

void GenomeGraphReader::readLine() {

	char buffer[1024];
	buffer[0] = '\0';

	while(!m_reader.eof()) {
		m_reader.getline(buffer, 1024);

		// skip comment
		if(strlen(buffer) > 0 && buffer[0] == '#')
			continue;

		break;
	}

	if(m_reader.eof()) {

		printName();
		cout << " finished reading file " << m_fileName;
		cout << " got " << m_loaded << " objects" << endl;

		Message finishedMessage;
		finishedMessage.setTag(DONE);

		send(m_parent, finishedMessage);

		die();
	} else {
		/*
		printName();
		cout << " got data line " << buffer;
		cout << " sending PAYLOAD to " << m_aggregator << endl;
*/
		Message message;
		message.setTag(CoalescenceManager::PAYLOAD);

		if(m_loaded % 1000000 == 0) {
			printName();
			cout << " loaded " << m_loaded << " sequences" << endl;

		}
		m_loaded ++;
		send(m_aggregator, message);
	}
}

void GenomeGraphReader::setFileName(string & fileName) {

	m_fileName = fileName;

	/*
	printName();
	cout << " setFileName " << m_fileName << endl;
	*/
}
