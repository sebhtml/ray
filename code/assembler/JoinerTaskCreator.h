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

#ifndef _JoinerTaskCreator_H
#define _JoinerTaskCreator_H

#include <scheduling/Worker.h>
#include <scheduling/TaskCreator.h>
#include <scheduling/VirtualProcessor.h>
#include <structures/StaticVector.h>
#include <core/Parameters.h>
#include <vector>
#include <communication/VirtualCommunicator.h>
#include <memory/RingAllocator.h>
#include <set>
#include <structures/Kmer.h>
using namespace std;

/**
 * The class creates and kills workers for the fusion of
 * similar paths */
class JoinerTaskCreator: public TaskCreator{
	VirtualCommunicator*m_virtualCommunicator;
	RingAllocator*m_outboxAllocator;
	Parameters*m_parameters;
	StaticVector*m_outbox;
	int*m_slaveMode;
	vector<vector<Kmer> >*m_paths;
	vector<uint64_t>*m_pathIdentifiers;

	set<uint64_t>*m_eliminated;

	uint64_t m_iterator;
	uint64_t m_currentWorkerIdentifier;
	bool m_reverseStrand;
public:
	void constructor( VirtualProcessor*virtualProcessor,StaticVector*outbox,
		RingAllocator*outboxAllocator,int*mode,Parameters*parameters,vector<vector<Kmer> >*paths,vector<uint64_t>*pathIdentifiers,
		set<uint64_t>*eliminated,VirtualCommunicator*virtualCommunicator,
		vector<vector<Kmer> >*newPaths
);

	void finishFusions();

	/** initialize the whole thing */
	void initializeMethod();

	/** finalize the whole thing */
	void finalizeMethod();

	/** has an unassigned task left to compute */
	bool hasUnassignedTask();

	/** assign the next task to a worker and return this worker */
	Worker*assignNextTask();

	/** get the result of a worker */
	void processWorkerResult(Worker*worker);

	/** destroy a worker */
	void destroyWorker(Worker*worker);

};

#endif
