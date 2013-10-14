

#ifndef GenomeGraphReaderHeader
#define GenomeGraphReaderHeader

#include <RayPlatform/actors/Actor.h>

#include <string>
#include <fstream>
using namespace std;

class GenomeGraphReader: public Actor {

	int m_loaded;
	ifstream m_reader;
	string m_fileName;
	int m_aggregator;

	void startParty(Message & message);

public:

	enum {
		START_PARTY = 42,
		START_PARTY_OK
	};


	GenomeGraphReader();
	~GenomeGraphReader();
	void receive(Message & message);
	void readLine();
	void setFileName(string & fileName);
};

#endif
