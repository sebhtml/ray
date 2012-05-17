/*
 	Ray
    Copyright (C)  2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _FusionWorker_H
#define _FusionWorker_H

#include <communication/VirtualCommunicator.h>
#include <scheduling/Worker.h>
#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <application_core/Parameters.h>

#include <stdint.h>
#include <map>
#include <vector>
using namespace std;

/**
 * FusionWorker merge a path with another path
 */
class FusionWorker: public Worker{

	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_GET_PATH_LENGTH;

	bool m_requestedNumberOfPaths;
	WorkerHandle m_workerIdentifier;
	bool m_isDone;
	vector<Kmer>*m_path;
	PathHandle m_identifier;
	bool m_reverseStrand;
	bool m_eliminated;
	int m_position;
	int m_numberOfPaths;
	VirtualCommunicator*m_virtualCommunicator;
	Parameters*m_parameters;
	bool m_receivedNumberOfPaths;
	RingAllocator*m_outboxAllocator;
	bool m_gatheredHits;
	bool m_initializedGathering;
	bool m_requestedHitLength;
	vector<PathHandle> m_hitNames;
	vector<int> m_hitLengths;
	int m_hitIterator;
	map<PathHandle,int> m_hits;
	int m_pathIndex;
	bool m_receivedPath;
	bool m_requestedPath;
public:
	void constructor(WorkerHandle i,vector<Kmer>*path,PathHandle identifier,bool reverseStrand,
VirtualCommunicator*virtualCommunicator,Parameters*parameters,RingAllocator*outboxAllocator,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
	MessageTag RAY_MPI_TAG_GET_PATH_LENGTH


);

	/* a method for Worker interface */
	void work();
	/* a method for Worker interface */
	bool isDone();
	/* a method for Worker interface */
	WorkerHandle getWorkerIdentifier();

	bool isPathEliminated();
	PathHandle getPathIdentifier();
};

#endif

