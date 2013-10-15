
#ifndef MotherHeader
#define MotherHeader

#include "CoalescenceManager.h"

#include <code/Mock/Parameters.h>
#include <RayPlatform/actors/Actor.h>

#define PLAN_RANK_ACTORS_PER_RANK 1
#define PLAN_MOTHER_ACTORS_PER_RANK 1
#define PLAN_STORE_KEEPER_ACTORS_PER_RANK 100
#define PLAN_GENOME_GRAPH_READER_ACTORS_PER_RANK 2

#include <vector>
#include <string>
using namespace std;

/**
 *
 * Map (assuming N ranks)
 *
 * The following map is not used anymore because it is stupid.
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
 * It would be nice to have a number of tokens / second that can be exchanged and also
 * the point-to-point latency for this actor implementation.
 *
 * \author Sébastien Boisvert
 */
class Mother: public Actor {

	Parameters * m_parameters;

	CoalescenceManager * m_coalescenceManager;
	int m_fileIterator;
	vector<int> m_filesToSpawn;

	vector<int> m_storeKeepers;
	vector<int> m_readers;

	vector<string> m_sampleNames;
	vector<string> m_graphFileNames;

	int m_aliveReaders;

	void spawnReader();
	void startSurveyor();
	void hello(Message & message);
	void boot(Message & message);
	void stop();

public:
	Mother();
	~Mother();
	void receive(Message & message);

	enum {
		HELLO = 10200
	};

	void setParameters(Parameters * parameters);
};

#endif

