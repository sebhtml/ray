/*
    Copyright 2014 Maxime Déraspe
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

#include "KmerMatrixOwner.h"
#include "CoalescenceManager.h" // for DIE

#include <RayPlatform/core/OperatingSystem.h>

#include <fstream>
#include <string>
#include <sstream>
using namespace std;

#include <math.h>

KmerMatrixOwner::KmerMatrixOwner() {

	m_completedStoreActors = 0;

}

KmerMatrixOwner::~KmerMatrixOwner() {

}


void KmerMatrixOwner::receive(Message & message) {

	int tag = message.getTag();
	char * buffer = message.getBufferBytes();
	int source = message.getSourceActor();

	if( tag == CoalescenceManager::DIE) {

		die();

	} else if(tag == GREETINGS) {

		int offset = 0;
		memcpy(&m_parameters, buffer + offset, sizeof(m_parameters));
		offset += sizeof(m_parameters);
		memcpy(&m_sampleNames, buffer + offset, sizeof(m_sampleNames));
		offset += sizeof(m_sampleNames);

#ifdef CONFIG_ASSERT
		assert(m_parameters != NULL);
		assert(m_sampleNames != NULL);
#endif
		m_mother = source;

		//open the buffer of the file
		createKmersMatrixOutputFile();

	} else if(tag == PUSH_KMER_SAMPLES) {

		vector<bool> samplesWithKmer;

		int offset = 0;

		Kmer kmer;
		offset += kmer.load(buffer);
		int numberOfSamples = m_sampleNames->size();
		char * bufferForSamples = buffer + offset;

		for(int i=0; i<numberOfSamples; ++i){
			bool state = bufferForSamples[i];
			samplesWithKmer.push_back(state);
		}

		dumpKmerMatrixBuffer(kmer, samplesWithKmer, false);

		Message response;
		response.setTag(PUSH_KMER_SAMPLES_OK);
		send(source, response);

	} else if(tag == PUSH_KMER_SAMPLES_END) {

		vector<bool> samplesWithKmer;

		int offset = 0;

		Kmer kmer;
		offset += kmer.load(buffer);
		int numberOfSamples = m_sampleNames->size();
		char * bufferForSamples = buffer + offset;

		for(int i=0; i<numberOfSamples; ++i){
			bool state = bufferForSamples[i];
			samplesWithKmer.push_back(state);
		}

		dumpKmerMatrixBuffer(kmer, samplesWithKmer, true);

		Message response;
		response.setTag(PUSH_KMER_SAMPLES_END);
		send(source, response);

		m_completedStoreActors += 1;

		if(m_completedStoreActors >= getSize()){
			Message coolMessage;
			coolMessage.setTag(KMER_MATRIX_IS_READY);
			send(m_mother, coolMessage);
			m_kmerMatrixFile.close();
		}

	}
}


void KmerMatrixOwner::dumpKmerMatrixBuffer(Kmer & kmer, vector<bool> & samplesWithKmer, bool force) {
	m_kmerMatrix << kmer.idToWord(m_parameters->getWordSize(),0);
	for(int i =0; i < (signed) samplesWithKmer.size(); ++i){
		m_kmerMatrix << "\t" << samplesWithKmer[i];
	}
	m_kmerMatrix << endl;

	flushFileOperationBuffer(force, &m_kmerMatrix, &m_kmerMatrixFile, CONFIG_FILE_IO_BUFFER_SIZE);
}


void KmerMatrixOwner::createKmersMatrixOutputFile() {

	string directory = m_parameters->getPrefix() + "Surveyor";

	// This directory is created elsewhere.
	// Just in case it does not exist, we create it.
	if(!fileExists(directory.c_str())) {
		createDirectory(directory.c_str());
	}

	string matrixFileString = m_parameters->getPrefix() + "Surveyor/KmerMatrix.tsv";
	m_kmerMatrixFile.open(matrixFileString.c_str());
	printMatrixHeader();
}


void KmerMatrixOwner::printMatrixHeader() {
	ostringstream header;
	header << "kmers";
	for(vector<string>::iterator sample = m_sampleNames->begin();
	    sample != m_sampleNames->end(); ++sample) {
		header << "\t" << *sample;
	}
	header << endl;
	flushFileOperationBuffer(true, &header, &m_kmerMatrixFile, CONFIG_FILE_IO_BUFFER_SIZE);
}
