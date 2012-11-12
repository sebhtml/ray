/*
 	Ray
    Copyright (C)  2011, 2012  Sébastien Boisvert

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

#ifndef _FusionTaskCreator_H
#define _FusionTaskCreator_H

#include <code/plugin_Mock/Parameters.h>
#include <code/plugin_KmerAcademyBuilder/Kmer.h>

#include <RayPlatform/scheduling/Worker.h>
#include <RayPlatform/scheduling/TaskCreator.h>
#include <RayPlatform/scheduling/VirtualProcessor.h>
#include <RayPlatform/structures/StaticVector.h>
#include <RayPlatform/communication/VirtualCommunicator.h>
#include <RayPlatform/memory/RingAllocator.h>
#include <RayPlatform/handlers/MasterModeHandler.h>
#include <RayPlatform/handlers/SlaveModeHandler.h>
#include <RayPlatform/core/ComputeCore.h>

#include <vector>
#include <set>
using namespace std;

__DeclarePlugin(FusionTaskCreator);

__DeclareSlaveModeAdapter(FusionTaskCreator,RAY_SLAVE_MODE_FUSION);

/**
 * The class creates and kills workers for the fusion of
 * similar paths
 */
class FusionTaskCreator: public TaskCreator, public CorePlugin {

	__AddAdapter(FusionTaskCreator,RAY_SLAVE_MODE_FUSION); 

	MessageTag RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED_REPLY_REPLY;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_GET_PATH_LENGTH;
	MessageTag RAY_MPI_TAG_FUSION_DONE;

	SlaveMode RAY_SLAVE_MODE_FUSION;
	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;

	VirtualCommunicator*m_virtualCommunicator;
	RingAllocator*m_outboxAllocator;
	Parameters*m_parameters;
	StaticVector*m_outbox;
	int*m_slaveMode;
	vector<vector<Kmer> >*m_paths;
	vector<PathHandle>*m_pathIdentifiers;

	set<PathHandle>*m_eliminated;

	uint64_t m_iterator;
	WorkerHandle m_currentWorkerIdentifier;
	bool m_reverseStrand;

	bool m_finishedInPreviousCycle;

	// fast run

	bool m_fastRun;

public:
	void constructor( VirtualProcessor*virtualProcessor,StaticVector*outbox,
		RingAllocator*outboxAllocator,int*mode,Parameters*parameters,vector<vector<Kmer> >*paths,vector<PathHandle >*pathIdentifiers,
		set<PathHandle>*eliminated,VirtualCommunicator*virtualCommunicator);

	void call_RAY_SLAVE_MODE_FUSION();

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

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif
