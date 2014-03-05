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

#ifndef KmerMatrixOwnerHeader
#define KmerMatrixOwnerHeader

#include <code/Mock/constants.h>
#include <code/Mock/Parameters.h>

#include <RayPlatform/actors/Actor.h>

#include <map>
#include <iostream>
#include <sstream>
using namespace std;

class KmerMatrixOwner : public Actor {
private:

	Parameters * m_parameters;
	vector<string> * m_sampleNames;

	int m_mother;
	int m_completedStoreActors;

        ostringstream m_kmerMatrix;
        ofstream m_kmerMatrixFile;

        /* void printLocalKmersMatrix(string & kmer, string & samples_kmers, bool force); */
        void printLocalKmersMatrix(Kmer & kmer, vector<bool> & samplesWithKmer, bool force);
        void createKmersMatrixOutputFile();

        void printMatrixHeader();

public:

	KmerMatrixOwner();
	~KmerMatrixOwner();

	void receive(Message & message);

	enum {
		FIRST_TAG = 10350,
		GREETINGS,
                PUSH_KMER_SAMPLES,
                PUSH_KMER_SAMPLES_OK,
                PUSH_KMER_SAMPLES_END,
		KMER_MATRIX_IS_READY,
		LAST_TAG
	};

};

#endif

