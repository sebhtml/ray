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


#ifndef GenomeAssemblyReaderHeader
#define GenomeAssemblyReaderHeader

#include <RayPlatform/actors/Actor.h>
/* #include <RayPlatform/files/FileReader.h> */

#include "GenomeAssemblyReader.h"
#include "CoalescenceManager.h"

#include <code/Mock/constants.h>
#include <code/Mock/common_functions.h>

#include <code/KmerAcademyBuilder/Kmer.h>
#include <code/VerticesExtractor/Vertex.h>

#include <code/Surveyor/SequenceKmerReader.h>


#include <string>
#include <fstream>
using namespace std;

#define I_LIKE_FAST_IO

class GenomeAssemblyReader: public Actor {

private:

	bool m_bad;

	int m_sample;
	int m_loaded;

	string m_fileName;

	int m_aggregator;
	int m_parent;

	void startParty(Message & message);

	void manageCommunicationForNewKmer(string & sequence, CoverageDepth & coverage, string & parent, string & child);
	void killActor();


	int m_kmerSize;
	SequenceKmerReader m_kmerReader;

	void readKmer();

public:

	enum {
		// Using the same tag as GenomeGraphReader
		// because we can mix an assembly reader with a graph reader
		FIRST_TAG = 10200,
		START_PARTY,
		START_PARTY_OK,
		DONE,
		LAST_TAG
	};


	GenomeAssemblyReader();
	~GenomeAssemblyReader();
	void receive(Message & message);
	void setFileName(string & fileName, int sample);
	void setKmerSize(int kmerSize);

};

#endif
