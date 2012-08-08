/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _SequencesLoader
#define _SequencesLoader

#include <application_core/Parameters.h>
#include <memory/RingAllocator.h>
#include <plugin_SequencesLoader/Loader.h>
#include <memory/MyAllocator.h>
#include <structures/StaticVector.h>
#include <communication/Message.h>
#include <plugin_SequencesLoader/Read.h>
#include <plugin_SeedExtender/BubbleData.h>
#include <core/ComputeCore.h>

#include <time.h>
#include <vector>
using namespace std;


/*
 * Computes the partition on reads (MASTER_RANK).
 * Loads the appropriate slice of reads (all MPI ranks).
 *
 * \author Sébastien Boisvert
 */
class SequencesLoader : public CorePlugin{

	MessageTag RAY_MPI_TAG_SEQUENCES_READY;

	SlaveMode RAY_SLAVE_MODE_LOAD_SEQUENCES;
	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;


	MyAllocator*m_persistentAllocator;
	ArrayOfReads*m_myReads;
	Parameters*m_parameters;
	LargeIndex m_distribution_currentSequenceId;
	int m_distribution_file_id;
	LargeIndex m_distribution_sequence_id;
	bool m_LOADER_isLeftFile;
	bool m_LOADER_isRightFile;
	Loader m_loader;
	bool m_isInterleavedFile;
	Rank m_rank;
	int m_size;
	
	StaticVector*m_outbox;
	SlaveMode*m_mode;

	void registerSequence();

public:
	bool call_RAY_SLAVE_MODE_LOAD_SEQUENCES();

	bool writeSequencesToAMOSFile(int rank,int size,StaticVector*m_outbox,
	RingAllocator*m_outboxAllocator,
	bool*m_loadSequenceStep,
	BubbleData*m_bubbleData,
	time_t*m_lastTime,
	Parameters*m_parameters,int*m_master_mode,int*m_mode);

	void constructor(int size,MyAllocator*m_persistentAllocator,ArrayOfReads*m_myReads,
		Parameters*parameters,StaticVector*outbox,SlaveMode*mode);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};
#endif
