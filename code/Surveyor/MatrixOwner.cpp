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

#include "MatrixOwner.h"
#include "CoalescenceManager.h" // for DIE

#include <RayPlatform/RayPlatform/core/OperatingSystem.h>

#include <fstream>
#include <string>
#include <sstream>
using namespace std;

MatrixOwner::MatrixOwner() {

	m_completedStoreActors = 0;
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

		memcpy(&m_parameters, buffer, sizeof(m_parameters));

#ifdef CONFIG_ASSERT
		assert(m_parameters != NULL);
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

		/*
		printName();
		cout << "DEBUG add " << sample1 << " " << sample2 << " " << count << endl;
*/

		m_localGramMatrix[sample1][sample2] += count;

		Message response;
		response.setTag(PUSH_PAYLOAD_OK);
		send(source, response);

	} else if(tag == PUSH_PAYLOAD_END) {

		m_completedStoreActors++;

		if(m_completedStoreActors == getSize()) {


			ostringstream matrixFile;
			matrixFile << m_parameters->getPrefix() << "/Surveyor/";
			string surveyorDirectory = matrixFile.str();
			createDirectory(surveyorDirectory.c_str());
			matrixFile << "SimilarityMatrix.tsv";

			string similarityMatrix = matrixFile.str();
			ofstream similarityFile;
			similarityFile.open(similarityMatrix.c_str());
			printLocalGramMatrix(similarityFile);
			similarityFile.close();

			printName();
			cout << "MatrixOwner prints the Similarity Matrix: ";
			cout << similarityMatrix << endl;

			Message coolMessage;
			coolMessage.setTag(MATRIX_IS_READY);
			send(m_mother, coolMessage);
		}
	}
}

/**
 * Write it in RaySurveyorResults/SurveyorMatrix.tsv
 * Also write a distance matrix too !
 */
void MatrixOwner::printLocalGramMatrix(ostream & stream) {

	/*
	printName();
	cout << "Local Gram Matrix: " << endl;
	cout << endl;
	*/

	for(map<SampleIdentifier, map<SampleIdentifier, LargeCount> >::iterator column = m_localGramMatrix.begin();
			column != m_localGramMatrix.end(); ++column) {

		SampleIdentifier sample = column->first;

		stream << "	" << sample;
	}

	stream << endl;

	for(map<SampleIdentifier, map<SampleIdentifier, LargeCount> >::iterator row = m_localGramMatrix.begin();
			row != m_localGramMatrix.end(); ++row) {

		SampleIdentifier sample1 = row->first;

		stream << sample1;
		for(map<SampleIdentifier, LargeCount>::iterator cell = row->second.begin();
				cell != row->second.end(); ++cell) {

			//SampleIdentifier sample2 = cell->first;

			LargeCount hits = cell->second;

			stream << "	" << hits;
		}

		stream << endl;
	}
}
