

#ifndef GenomeGraphReaderHeader
#define GenomeGraphReaderHeader

#include <RayPlatform/actors/Actor.h>

class GenomeGraphReader: public Actor {

public:
	GenomeGraphReader();
	~GenomeGraphReader();
	void receive(Message & message);
};

#endif
