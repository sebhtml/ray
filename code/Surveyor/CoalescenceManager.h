
#ifndef CoalescenceManagerHeader
#define CoalescenceManagerHeader

#include <RayPlatform/actors/Actor.h>

class CoalescenceManager : public Actor {

public:

	void receive(Message & message);
};

#endif
