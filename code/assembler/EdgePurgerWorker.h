/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#ifndef _EdgePurgerWorker_H
#define _EdgePurgerWorker_H

#include <stdint.h>
#include <structures/Kmer.h>
#include <structures/Vertex.h>
#include <memory/RingAllocator.h>
#include <graph/GridTable.h>
#include <core/Parameters.h>
#include <structures/StaticVector.h>
#include <communication/VirtualCommunicator.h>

/**
 * An EdgePurgerWorker actually does the job.
 * It purges arcs pointing to unexistant vertices
 */
class EdgePurgerWorker{
	bool m_outgoingInitialised;
	bool m_ingoingInitialised;
	int m_iterator;
	vector<Kmer> m_edges;
	bool m_doneIngoingEdges;
	bool m_doneOutgoingEdges;
	bool m_coverageRequested;
	bool m_isDone;
	Parameters*m_parameters;
	GridTable*m_subgraph;
	Kmer m_currentKmer;
	Vertex*m_vertex;
	uint64_t m_workerId;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	VirtualCommunicator*m_virtualCommunicator;
public:
	bool isDone();
	void work();
	void constructor(uint64_t workerId,Vertex*vertex,Kmer*currentKmer,GridTable*subgraph,VirtualCommunicator*virtualCommunicator,RingAllocator*outboxAllocator,Parameters*parameters,
		StaticVector*inbox,StaticVector*outbox);
};

#endif
