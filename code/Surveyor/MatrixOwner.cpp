/*
    Copyright 2013 Sébastien Boisvert, Maxime Déraspe
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

#include "MatrixOwner.h"
#include "CoalescenceManager.h" // for DIE

#include <RayPlatform/core/OperatingSystem.h>

#include <fstream>
#include <string>
#include <sstream>
using namespace std;

#include <math.h>

MatrixOwner::MatrixOwner() {

	m_completedStoreActors = 0;

	m_receivedPayloads = 0;

}

MatrixOwner::~MatrixOwner() {

}


void MatrixOwner::receive(Message & message) {

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

	} else if(tag == PUSH_PAYLOAD) {

		SampleIdentifier sample1 = -1;
		SampleIdentifier sample2 = -1;
		LargeCount count = 0;

		int offset = 0;

		memcpy(&sample1, buffer + offset, sizeof(sample1));
		offset += sizeof(sample1);
		memcpy(&sample2, buffer + offset, sizeof(sample2));
		offset += sizeof(sample2);
		memcpy(&count, buffer + offset, sizeof(count));
		offset += sizeof(count);

#ifdef CONFIG_ASSERT
		assert(sample1 >= 0);
		assert(sample2 >= 0);
		assert(count >= 0);
#endif

		m_receivedPayloads ++;

		m_localGramMatrix[sample1][sample2] += count;

		Message response;
		response.setTag(PUSH_PAYLOAD_OK);
		send(source, response);
        } else if(tag == PUSH_PAYLOAD_END) {

		m_completedStoreActors++;

		if(m_completedStoreActors == getSize()) {

			printName();
			cout << "MatrixOwner received " << m_receivedPayloads << " payloads" << endl;

			// create directory for Surveyor
			ostringstream matrixFile;
			matrixFile << m_parameters->getPrefix() << "/Surveyor/";
			string surveyorDirectory = matrixFile.str();

			// avoid a race condition
			// we don't know which actor will win the race of life.
			if(!fileExists(surveyorDirectory.c_str())) {
				createDirectory(surveyorDirectory.c_str());
			}

			// create SimilarityMatrix
			matrixFile << "SimilarityMatrix.tsv";

			string similarityMatrix = matrixFile.str();
			ofstream similarityFile;
			similarityFile.open(similarityMatrix.c_str());
			printLocalGramMatrix(similarityFile, m_localGramMatrix);
			similarityFile.close();

			printName();
			cout << "MatrixOwner printed the Similarity Matrix: ";
			cout << similarityMatrix << endl;

			// create DistanceMatrix

			computeDistanceMatrix();

			ostringstream matrixFileForDistances;
			matrixFileForDistances << m_parameters->getPrefix() << "/Surveyor/";
			matrixFileForDistances << "DistanceMatrix.tsv";


			string distanceMatrix = matrixFileForDistances.str();
			ofstream distanceFile;
			distanceFile.open(distanceMatrix.c_str());
			printLocalGramMatrix(distanceFile, m_kernelDistanceMatrix);
			distanceFile.close();

			printName();
			cout << "MatrixOwner printed the Distance Matrix: ";
			cout << distanceMatrix << endl;


			// tell Mother that the matrix is ready now.
                        Message coolMessage;
                        coolMessage.setTag(GRAM_MATRIX_IS_READY);
                        send(m_mother, coolMessage);


			// clear matrices

			m_localGramMatrix.clear();
			m_kernelDistanceMatrix.clear();
		}
	}
}


// TODO: save time by only computing the lower triangle.
void MatrixOwner::computeDistanceMatrix() {

	for(map<SampleIdentifier, map<SampleIdentifier, LargeCount> >::iterator row = m_localGramMatrix.begin();
			row != m_localGramMatrix.end(); ++row) {

		SampleIdentifier sample1 = row->first;

		for(map<SampleIdentifier, LargeCount>::iterator cell = row->second.begin();
				cell != row->second.end(); ++cell) {

			SampleIdentifier sample2 = cell->first;

			// d(x, x') = sqrt( k(x,x) + k(x', x') - 2 k (x, x'))
			LargeCount distance = 0;
			distance += m_localGramMatrix[sample1][sample1];
			distance += m_localGramMatrix[sample2][sample2];
			distance -= 2 * m_localGramMatrix[sample1][sample2];

			distance = (LargeCount) sqrt((double)distance);

			m_kernelDistanceMatrix[sample1][sample2] = distance;
			m_kernelDistanceMatrix[sample2][sample1] = distance;

		}

	}
}

void MatrixOwner::printLocalGramMatrix(ostream & stream, map<SampleIdentifier, map<SampleIdentifier, LargeCount> > & matrix) {

	int numberOfSamples = m_sampleNames->size();

	for(int i = 0 ; i < numberOfSamples ; ++i) {

		string & sampleName1 = m_sampleNames->at(i);

		stream << "	" << sampleName1;
	}

	stream << endl;


	for(int i = 0 ; i < numberOfSamples ; ++i) {

		string & sampleName1 = m_sampleNames->at(i);

		stream << sampleName1;

		for(int j = 0 ; j < numberOfSamples ; ++j) {

			//string & sampleName2 = m_sampleNames->at(j);

			LargeCount hits = 0;

			if(matrix.count(i) > 0 && matrix[i].count(j) > 0) {

				hits = matrix[i][j];
			}

			stream << "	" << hits;
		}

		stream << endl;
	}
}

/**
 * Write it in RaySurveyorResults/SurveyorMatrix.tsv
 * Also write a distance matrix too !
 */
void MatrixOwner::printLocalGramMatrixWithHash(ostream & stream, map<SampleIdentifier, map<SampleIdentifier, LargeCount> > & matrix) {

	/*
	printName();
	cout << "Local Gram Matrix: " << endl;
	cout << endl;
	*/

	for(map<SampleIdentifier, map<SampleIdentifier, LargeCount> >::iterator column = matrix.begin();
			column != matrix.end(); ++column) {

		SampleIdentifier sample = column->first;

		stream << "	" << m_sampleNames->at(sample);
	}

	stream << endl;

	for(map<SampleIdentifier, map<SampleIdentifier, LargeCount> >::iterator row = matrix.begin();
			row != matrix.end(); ++row) {

		SampleIdentifier sample1 = row->first;

		stream << m_sampleNames->at(sample1);

		for(map<SampleIdentifier, LargeCount>::iterator cell = row->second.begin();
				cell != row->second.end(); ++cell) {

			//SampleIdentifier sample2 = cell->first;

			LargeCount hits = cell->second;

			stream << "	" << hits;
		}

		stream << endl;
	}
}

