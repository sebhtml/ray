
#ifndef CoalescenceManagerHeader
#define CoalescenceManagerHeader

#include <RayPlatform/actors/Actor.h>

class CoalescenceManager : public Actor {

	int m_kmerLength;
	bool m_colorSpaceMode;

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
		DIE
	};
};

#endif
