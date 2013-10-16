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


#ifndef CoalescenceManagerHeader
#define CoalescenceManagerHeader

#include <code/KmerAcademyBuilder/Kmer.h>

#include <RayPlatform/actors/Actor.h>

class CoalescenceManager : public Actor {

private:

	int m_localStore;

	int m_kmerLength;
	bool m_colorSpaceMode;

	int m_storeFirstActor;
	int m_storeLastActor;

	int getVertexDestination(Kmer & kmer);
public:

	CoalescenceManager();
	~CoalescenceManager();

	void receive(Message & message);
	void receivePayload(Message & message);

	enum {
		PAYLOAD = 10100,
		PAYLOAD_RESPONSE,
		SET_KMER_LENGTH,
		SET_KMER_LENGTH_OK,
		INTRODUCE_STORE,
		DIE
	};
};

#endif
