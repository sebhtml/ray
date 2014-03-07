/*
    Copyright 2014 Maxime Déraspe
    Copyright 2014 Université Laval

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

// TODO: validate that the kmer length is the same for this file
// and the provided -k argument

#include "GenomeAssemblyReader.h"
#include "CoalescenceManager.h"

#include "SequenceKmerReader.h"

#include <code/Mock/constants.h>
#include <code/Mock/common_functions.h>

#include <code/KmerAcademyBuilder/Kmer.h>
#include <code/VerticesExtractor/Vertex.h>


#include <iostream>
#include <sstream>
using namespace std;

#include <string.h>

// TODO: need to know the kmer size

GenomeAssemblyReader::GenomeAssemblyReader() {

}

GenomeAssemblyReader::~GenomeAssemblyReader() {

}

void GenomeAssemblyReader::receive(Message & message) {

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
		readKmer();
	}
}

void GenomeAssemblyReader::startParty(Message & message) {

	char * buffer = (char*) message.getBufferBytes();

	memcpy(&m_aggregator, buffer, sizeof(int));

	m_kmerReader.openFile(m_fileName, m_kmerSize);
	m_loaded = 0;

	printName();
	cout <<"opens file " << m_fileName << endl;

	m_parent = message.getSourceActor();

	int source = message.getSourceActor();
	Message response;
	response.setTag(START_PARTY_OK);

	send(source, response);

	readKmer();
}

// DONE 2013-10-16: add a BufferedLineReader class in RayPlatform
// and use it here.
void GenomeAssemblyReader::readKmer() {
// void GenomeAssemblyReader::readLine() {

	string sequence;
	CoverageDepth coverage = 1;
	string badParent = "";
	string badChild = "";

	// ofstream outFile;
	// outFile.open("kmers-created.txt", ios::app);

	if(m_kmerReader.hasAnotherKmer()){

	m_kmerReader.fetchNextKmer(sequence);

	manageCommunicationForNewKmer(sequence, coverage, badParent , badChild);
#if 0
	cout << "DEBUG: Sending a Kmer to storekeeper" << endl;
#endif
	}else{

		Message finishedMessage;
		finishedMessage.setTag(DONE);

		send(m_parent, finishedMessage);

		die();
	}


}



void GenomeAssemblyReader::manageCommunicationForNewKmer(string & sequence, CoverageDepth & coverage, string & parents, string & children){


	// if this is the first one, send the k-mer length too
	if(m_loaded == 0) {

	Message aMessage;
	aMessage.setTag(CoalescenceManager::SET_KMER_LENGTH);

	int length = sequence.length();
	aMessage.setBuffer(&length);
	aMessage.setNumberOfBytes(sizeof(length));

	send(m_aggregator, aMessage);
	}

	Kmer kmer;
	kmer.loadFromTextRepresentation(sequence.c_str());

#if 0
	cout << "DEBUG: "  << sequence.c_str() << endl;
#endif
	Vertex vertex;
	vertex.setKey(kmer);
	vertex.setCoverageValue(coverage);

	// add parents
	for(int i = 0 ; i < (int)parents.length() ; ++i) {

	string parent = sequence;
	for(int j = 0 ; j < (int) parent.length()-1 ; ++j) {
		parent[j + 1] = parent[j];
	}
	parent[0] = parents[i];

	Kmer parentKmer;
	parentKmer.loadFromTextRepresentation(parent.c_str());

	vertex.addIngoingEdge(&kmer, &parentKmer, sequence.length());
	}

	// add children
	for(int i = 0 ; i < (int)children.length() ; ++i) {

	string child = sequence;
	for(int j = 0 ; j < (int) child.length()-1 ; ++j) {
		child[j] = child[j + 1];
	}
	child[child.length() - 1] = children[i];

	Kmer childKmer;
	childKmer.loadFromTextRepresentation(child.c_str());

	vertex.addOutgoingEdge(&kmer, &childKmer, sequence.length());
	}

	char messageBuffer[100];
	int position = 0;

	position += vertex.dump(messageBuffer + position);
	memcpy(messageBuffer + position, &m_sample, sizeof(m_sample));

	position += sizeof(m_sample);

// maybe: accumulate many objects before flushing it.
// we can go up to MAXIMUM_MESSAGE_SIZE_IN_BYTES bytes.

	/*
	  printName();
	  cout << " got data line " << buffer;
	  cout << " sending PAYLOAD to " << m_aggregator << endl;
	*/
	Message message;
	message.setTag(CoalescenceManager::PAYLOAD);
	message.setBuffer(messageBuffer);
	message.setNumberOfBytes(position);

#if 0
	printName();
	cout << "DEBUG sending PAYLOAD to " << m_aggregator;
	cout << " with " << position << " bytes ";
	vertex.print(sequence.length(), false);
	cout << endl;
#endif

	int period = 1000000;
	if(m_loaded % period == 0) {
	printName();
	cout << " loaded " << m_loaded << " sequences" << endl;

	}
	m_loaded ++;
	send(m_aggregator, message);
}


void GenomeAssemblyReader::setFileName(string & fileName, int sample) {

	m_sample = sample;

	m_fileName = fileName;

#if 0
	printName();
	cout << " DEBUG setFileName " << m_fileName << endl;
#endif

}


void GenomeAssemblyReader::setKmerSize(int kmerSize) {
	m_kmerSize = kmerSize;
}
