

#ifndef StoreKeeperHeader
#define StoreKeeperHeader

#include <RayPlatform/actors/Actor.h>

class StoreKeeper: public Actor {

public:
	StoreKeeper();
	~StoreKeeper();
	void receive(Message & message);
};

#endif
