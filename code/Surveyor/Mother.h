
#ifndef MotherHeader
#define MotherHeader

#include <RayPlatform/actors/Actor.h>

class Mother: public Actor {

public:
	Mother();
	~Mother();
	void receive(Message * message);
	void boot(Message * message);
};

#endif

