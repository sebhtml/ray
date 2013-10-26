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

#ifndef MatrixOwnerHeader
#define MatrixOwnerHeader

#include <code/Mock/constants.h>
#include <code/Mock/Parameters.h>

#include <RayPlatform/actors/Actor.h>

#include <map>
#include <iostream>
using namespace std;

class MatrixOwner : public Actor {
private:

	Parameters * m_parameters;
	vector<string> * m_sampleNames;

	map<SampleIdentifier, map<SampleIdentifier, LargeCount> > m_localGramMatrix;
	map<SampleIdentifier, map<SampleIdentifier, LargeCount> > m_kernelDistanceMatrix;

	int m_mother;
	int m_completedStoreActors;
	void printLocalGramMatrix(ostream & stream, map<SampleIdentifier, map<SampleIdentifier, LargeCount> > & matrix);

	void computeDistanceMatrix();

public:

	MatrixOwner();
	~MatrixOwner();

	void receive(Message & message);

	enum {
		FIRST_TAG = 10300,
		GREETINGS,
		PUSH_PAYLOAD,
		PUSH_PAYLOAD_OK,
		PUSH_PAYLOAD_END,
		MATRIX_IS_READY,
		LAST_TAG
	};

};

#endif

