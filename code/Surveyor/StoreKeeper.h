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

#include <RayPlatform/actors/Actor.h>

/**
 * Provides genomic storage.
 *
 * \author Sébastien Boisvert
 */
class StoreKeeper: public Actor {
private:

	int m_kmerLength;
	bool m_colorSpaceMode;

	uint64_t m_receivedObjects;

	void pushSampleVertex(Message & message);
	void printStatus();

public:

	StoreKeeper();
	~StoreKeeper();

	void receive(Message & message);

	enum {
		PUSH_SAMPLE_VERTEX = 10400,
		PUSH_SAMPLE_VERTEX_OK
	};
};

#endif
