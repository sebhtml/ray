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

// TODO -> flush buffers at the end.

#include "CoalescenceManager.h"
#include "StoreKeeper.h"

#include <code/Mock/constants.h>
#include <code/Mock/common_functions.h>
#include <code/KmerAcademyBuilder/Kmer.h>
#include <code/VerticesExtractor/Vertex.h>

#include <iostream>
using namespace std;

CoalescenceManager::CoalescenceManager() {

	m_kmerLength = 0;
	m_colorSpaceMode = false;

	m_buffers = NULL;
	m_bufferSizes = NULL;
}

CoalescenceManager::~CoalescenceManager() {


	free(m_buffers);
	free(m_bufferSizes);

	m_buffers = NULL;
	m_bufferSizes = NULL;
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

	} else if(tag == SET_KMER_LENGTH) {

		int kmerLength = 0;
		char * buffer = (char*)message.getBufferBytes();
		memcpy(&kmerLength, buffer, sizeof(kmerLength));

		if(m_kmerLength == 0)
			m_kmerLength = kmerLength;

		if(m_kmerLength != kmerLength) {

			printName();
			cout << " Error: the k-mer length is not the same in all input files !";
			cout << endl;
		}

		// cout << "DEBUG m_kmerLength = " << m_kmerLength << endl;

		// the color space mode is an artefact.
		m_colorSpaceMode = false;

		Message response;
		response.setTag(SET_KMER_LENGTH_OK);

		int source = message.getSourceActor();

		/*
		printName();
		cout << "DEBUG Sending SET_KMER_LENGTH_OK to " << source << endl;
		*/

		send(source, response);

		/*
		 * It is not the job here to find the kmer length
*/

	} else if(tag == INTRODUCE_STORE) {

		char * buffer = (char*) message.getBufferBytes();

		int localStore = -1;

		memcpy(&localStore, buffer, sizeof(localStore));

#ifdef CONFIG_ASSERT
		assert(localStore >= 0);
#endif
		// the the node on which we are
		int rank = getRank();
		int numberOfRanks = getSize();

		/*
		 * We have N MPI ranks.
		 * The local rank is x.
		 * The local StoreKeeper actor is y.
		 * There are PLAN_STORE_KEEPER_ACTORS_PER_RANK StoreKeeper actors per rank
		 *
		 * So basically, we need to find the actor name on rank 0 (first StoreKeeper actor)
		 * then, we add PLAN_STORE_KEEPER_ACTORS_PER_RANK * getSize to that -1 (the last StoreKeeper
		 * actor)
		 *
		 * This obviously assumes that allocation is regular.
		 * This assumption is correct since everything is spawned by Mother actors (in Surveyor).
		 *
		 * y = x + i * N
		 *
		 * Find i
		 *
		 * y - x = i * N
		 * i = (y - x) / N
		 *
		 * or
		 *
		 */

		int iterator = ( localStore - rank ) / numberOfRanks;

		m_localStore = localStore;

		int first = 0 + iterator * numberOfRanks;
		int last = first + ( numberOfRanks * PLAN_STORE_KEEPER_ACTORS_PER_RANK ) -1;

		m_storeFirstActor = first;
		m_storeLastActor = last;

		printName();
		cout << " is now acquainted with StoreKeeper actors from ";
		cout << m_storeFirstActor << " to " << m_storeLastActor << endl;

		// allocate buffers too

		if(m_buffers == NULL) {

			int storageActors = m_storeLastActor - m_storeFirstActor + 1;
			
			int bytesAvailable = MAXIMUM_MESSAGE_SIZE_IN_BYTES;

			m_bufferTotalSize = bytesAvailable;
			m_storageActors = storageActors;

			m_buffers = (char*) malloc(storageActors * bytesAvailable);
			m_bufferSizes = (int*) malloc(storageActors * sizeof(int));

			for(int i = 0 ; i < storageActors ; ++i) {
				m_bufferSizes[i] = 0;
			}
		}

	} else if(tag == StoreKeeper::PUSH_SAMPLE_VERTEX_OK) {

		int producer = -1;
		char * buffer = (char*) message.getBufferBytes();

		memcpy(&producer, buffer, sizeof(producer));

		int source = producer;

		// respond to the producer now
		Message response;
		response.setTag(PAYLOAD_RESPONSE);
		send(source, response);

		//cout << "Resume reader 2" << endl;
	}
}

void CoalescenceManager::receivePayload(Message & message) {

	int source = message.getSourceActor();

	char * buffer = (char*)message.getBufferBytes();
	//int bytes = message.getNumberOfBytes();

	int position = 0;
	Vertex vertex;
	position += vertex.load(buffer + position);

	int sample = -1;
	memcpy(&sample, buffer + position, sizeof(sample));
	position += sizeof(sample);

	m_producer = source;

	if(!classifyKmerInBuffer(sample, vertex)) {

		Message response;
		response.setTag(PAYLOAD_RESPONSE);
		send(source, response);
		//cout << "Resume reader 1" << endl;
	}
}

bool CoalescenceManager::classifyKmerInBuffer(int & sample, Vertex & vertex) {

	Kmer kmer = vertex.getKey();
	int storageDestination = getVertexDestination(kmer);

#if 0
	printName();
	int source = -999;

	cout << "DEBUG/CoalescenceManager received PAYLOAD from " << source;
	cout << " ";
	vertex.print(m_kmerLength, m_colorSpaceMode);
	cout << endl;


	cout << "Destination -> " << storageDestination << endl;
#endif

	return addKmerInBuffer(storageDestination, sample, vertex);
}

bool CoalescenceManager::addKmerInBuffer(int & actor, int & sample, Vertex & vertex) {

	int actorIndex = actor - m_storeFirstActor;

#ifdef CONFIG_ASSERT
	assert(actorIndex < m_storageActors);
#endif

	char * buffer = m_buffers + actorIndex * m_bufferTotalSize * sizeof(char);
	int offset = m_bufferSizes[actorIndex];

	int requiredBytes = 0;
	requiredBytes += vertex.getRequiredNumberOfBytes();
	requiredBytes += sizeof(sample);

	offset += vertex.dump(buffer + offset);
	memcpy(buffer + offset, &sample, sizeof(sample));
	offset += sizeof(sample);

	m_bufferSizes[actorIndex] = offset;

	// flush the message if no more bytes are available
	int availableBytes = m_bufferTotalSize - offset;
	availableBytes -= sizeof(int);

	if(availableBytes < requiredBytes) {

		// store producer

		memcpy(buffer + offset, &m_producer, sizeof(m_producer));
		offset += sizeof(m_producer);

		int bytes = offset;

		/*
		printName();
		cout << "flushing data, sending stuff to " << actor << endl;
		*/

		Message routedMessage;
		routedMessage.setTag(StoreKeeper::PUSH_SAMPLE_VERTEX);
		routedMessage.setBuffer(buffer);
		routedMessage.setNumberOfBytes(bytes);

		// free the buffer.
		m_bufferSizes[actorIndex] = 0;


		int storageDestination = actor;

		// DONE: do some aggregation or something !
		send(storageDestination, routedMessage);

		return true;
	}

#if 0
	printName();
	cout << "sends bits to StoreKeeper # " << storageDestination;
	cout << endl;
#endif

	return false;
}

int CoalescenceManager::getVertexDestination(Kmer & kmer) {

	uint64_t hash = kmer.getTwinHash1(m_kmerLength, m_colorSpaceMode);

	//cout << "DEBUG getTwinHash1 " << hash << endl;

	int storageActors = m_storeLastActor - m_storeFirstActor + 1;

	int actor = (hash % storageActors) + m_storeFirstActor;

	return actor;
}
