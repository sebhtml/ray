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


#ifndef GenomeGraphReaderHeader
#define GenomeGraphReaderHeader

#include <RayPlatform/actors/Actor.h>
#include <RayPlatform/files/FileReader.h>


#include <string>
#include <fstream>
using namespace std;

#define I_LIKE_FAST_IO

class GenomeGraphReader: public Actor {

	int m_loaded;

#ifdef I_LIKE_FAST_IO

	// fast IO using a wrapper.
	FileReader m_reader;
#else

	ifstream m_reader;
#endif

	string m_fileName;

	int m_aggregator;
	int m_parent;

	void startParty(Message & message);

public:

	enum {
		START_PARTY = 10300,
		START_PARTY_OK,
		DONE
	};


	GenomeGraphReader();
	~GenomeGraphReader();
	void receive(Message & message);
	void readLine();
	void setFileName(string & fileName);
};

#endif
