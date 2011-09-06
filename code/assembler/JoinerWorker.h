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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _JoinerWorker_H
#define _JoinerWorker_H

#include <communication/VirtualCommunicator.h>
#include <scheduling/Worker.h>
#include <stdint.h>
#include <map>
#include <vector>
#include <structures/Kmer.h>
using namespace std;

/**
 * JoinerWorker merge a path with another path
 */
class JoinerWorker: public Worker{
	bool m_requestedNumberOfPaths;
	uint64_t m_workerIdentifier;
	bool m_isDone;
	vector<Kmer>*m_path;
	uint64_t m_identifier;
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
	vector<uint64_t> m_hitNames;
	vector<int> m_hitLengths;
	int m_hitIterator;

	bool m_selectedHit;
	int m_selectedHitIndex;

	map<uint64_t,int> m_hits;
	map<uint64_t,int> m_minPosition;
	map<uint64_t,int> m_maxPosition;
	map<uint64_t,int> m_minPositionOnSelf;
	map<uint64_t,int> m_maxPositionOnSelf;

	int m_pathIndex;
	bool m_receivedPath;
	bool m_requestedPath;
public:
	void constructor(uint64_t i,vector<Kmer>*path,uint64_t identifier,bool reverseStrand,
VirtualCommunicator*virtualCommunicator,Parameters*parameters,RingAllocator*outboxAllocator);

	/* a method for Worker interface */
	void work();
	/* a method for Worker interface */
	bool isDone();
	/* a method for Worker interface */
	uint64_t getWorkerIdentifier();

	bool isPathEliminated();
	uint64_t getPathIdentifier();
};

#endif

