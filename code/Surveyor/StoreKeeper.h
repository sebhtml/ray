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

/**
 * Provides genomic storage.
 *
 * \author Sébastien Boisvert
 */
class StoreKeeper: public Actor {

private:

	map<SampleIdentifier, map<SampleIdentifier, LargeCount> > m_localGramMatrix;

	map<SampleIdentifier, map<SampleIdentifier, LargeCount> >::iterator m_iterator1;
	map<SampleIdentifier, LargeCount>::iterator m_iterator2;

	ColorSet m_colorSet;

	int m_mother;
	int m_matrixOwner;

	bool m_configured;

	/**
	 * MyHashTable is a space-efficient data structure provided
	 * by RayPlatform.
	 */
	MyHashTable<Kmer,ExperimentVertex> m_hashTable;

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

	void sendMatrixCell();

public:

	StoreKeeper();
	~StoreKeeper();

	void receive(Message & message);

	enum {
		FIRST_TAG = 10250,
		PUSH_SAMPLE_VERTEX,
		PUSH_SAMPLE_VERTEX_OK,
		MERGE,
		MERGE_OK,
		LAST_TAG
	};
};

#endif
