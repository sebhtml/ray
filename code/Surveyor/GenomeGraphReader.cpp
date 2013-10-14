
#include "GenomeGraphReader.h"

#include <iostream>
using namespace std;

#include <string.h>

GenomeGraphReader::GenomeGraphReader() {

}

GenomeGraphReader::~GenomeGraphReader() {

}

void GenomeGraphReader::receive(Message & message) {

	int type = message.getTag();

	printName();
	cout << "received tag " << type << endl;

	if(type == START_PARTY) {
		startParty(message);
	}
}

void GenomeGraphReader::startParty(Message & message) {

	printName();
	cout << "DEBUG startParty" << endl;
	cout << " bytes in message: " << message.getNumberOfBytes();
	cout << endl;

	m_reader.open(m_fileName.c_str());
	m_loaded = 0;

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
		cout << " finished reading file " << m_fileName << endl;

	} else {
		printName();
		cout << " got data line " << buffer << endl;
	}
}

void GenomeGraphReader::setFileName(string & fileName) {

	m_fileName = fileName;

	printName();
	cout << " setFileName " << m_fileName << endl;
}
