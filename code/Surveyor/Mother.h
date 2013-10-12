
#ifndef MotherHeader
#define MotherHeader

#include <code/Mock/Parameters.h>
#include <RayPlatform/actors/Actor.h>

#define PLAN_RANK_ACTORS_PER_RANK 1
#define PLAN_MOTHER_ACTORS_PER_RANK 1
#define PLAN_STORE_KEEPER_ACTORS_PER_RANK 100
#define PLAN_GENOME_GRAPH_READER_ACTORS_PER_RANK 2

#include <vector>
using namespace std;

/**
 *
 * Map (assuming N ranks)
 *
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
 * \author Sébastien Boisvert
 */
class Mother: public Actor {

	Parameters * m_parameters;

	vector<int> m_storeKeepers;
	vector<int> m_readers;

public:
	Mother();
	~Mother();
	void receive(Message & message);
	void hello(Message & message);
	void boot(Message & message);

	enum {
		HELLO = 10100
	};

	void setParameters(Parameters * parameters);
	void startSurveyor();
};

#endif

