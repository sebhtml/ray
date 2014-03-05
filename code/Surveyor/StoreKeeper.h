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



#ifndef StoreKeeperHeader
#define StoreKeeperHeader

#define PLAN_STORE_KEEPER_ACTORS_PER_RANK 1

#include "ExperimentVertex.h"

#include <code/Searcher/ColorSet.h>
#include <code/VerticesExtractor/Vertex.h>
#include <code/KmerAcademyBuilder/Kmer.h>
#include <code/Mock/constants.h>

#include <RayPlatform/actors/Actor.h>
#include <RayPlatform/structures/MyHashTable.h>
#include <RayPlatform/structures/MyHashTableIterator.h>

#include <iostream>
#include <sstream>

/**
 * Provides genomic storage.
 *
 * \author Sébastien Boisvert
 */
class StoreKeeper: public Actor {

private:

	int m_storeDataCalls;
	int m_receivedPushes;
	map<SampleIdentifier, map<SampleIdentifier, LargeCount> > m_localGramMatrix;

	map<SampleIdentifier, map<SampleIdentifier, LargeCount> >::iterator m_iterator1;
	map<SampleIdentifier, LargeCount>::iterator m_iterator2;

	ColorSet m_colorSet;

	int m_mother;
	int m_matrixOwner;
	int m_kmerMatrixOwner;

	bool m_configured;

	/**
	 * MyHashTable is a space-efficient data structure provided
	 * by RayPlatform.
	 */
	MyHashTable<Kmer,ExperimentVertex> m_hashTable;

        MyHashTableIterator<Kmer,ExperimentVertex> m_hashTableIterator;

	int m_kmerLength;
	bool m_colorSpaceMode;

	uint64_t m_receivedObjects;
	uint64_t m_lastSize;

	void pushSampleVertex(Message & message);
	void printStatus();

	void storeData(Vertex & vertex, int & sample);
	void configureHashTable();
	void computeLocalGramMatrix();
	void printLocalGramMatrix();
	void printColorReport();

        int m_sampleSize;
        string m_outputKmersMatrixPath;
        void printLocalKmersMatrix(string & m_kmer, string & m_samplesKmers);
        void sendKmersSamples();

	void sendMatrixCell();

public:

	StoreKeeper();
	~StoreKeeper();

        void setSampleSize(int sampleSize);

	void receive(Message & message);

	enum {
		FIRST_TAG = 10250,
		PUSH_SAMPLE_VERTEX,
		PUSH_SAMPLE_VERTEX_OK,
		MERGE_GRAM_MATRIX,
		MERGE_GRAM_MATRIX_OK,
                MERGE_KMER_MATRIX,
                MERGE_KMER_MATRIX_OK,
		LAST_TAG
	};
};

#endif
