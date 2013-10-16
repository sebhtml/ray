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

#ifndef MotherHeader
#define MotherHeader

#include "CoalescenceManager.h"

#include <code/Mock/Parameters.h>
#include <RayPlatform/actors/Actor.h>

#include <vector>
#include <string>
using namespace std;

/**
 *
 * Map (assuming N ranks)
 *
 * The following map is not used anymore because it is stupid.
 * ----------------------------------------------------------
 * Actors			Quantity	Role
 * Start	End
 * ----------------------------------------------------------
 * 0		N - 1		N		ComputeCore
 * N		2N - 1		N		Mother
 * 2N		102N - 1	100N		StoreKeeper
 * 102N		104N - 1	2N		GenomeGraphReader
 * ----------------------------------------------------------
 *
 * It would be nice to have a number of tokens / second that can be exchanged and also
 * the point-to-point latency for this actor implementation.
 *
 * \author Sébastien Boisvert
 */
class Mother: public Actor {

	Parameters * m_parameters;

	int m_coalescenceManager;
	int m_fileIterator;
	vector<int> m_filesToSpawn;

	vector<int> m_storeKeepers;
	vector<int> m_readers;

	vector<string> m_sampleNames;
	vector<string> m_graphFileNames;

	int m_aliveReaders;

	void spawnReader();
	void startSurveyor();
	void hello(Message & message);
	void boot(Message & message);
	void stop();

public:
	Mother();
	~Mother();
	void receive(Message & message);

	enum {
		HELLO = 10200
	};

	void setParameters(Parameters * parameters);
};

#endif

