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

#ifndef _FusionData
#define _FusionData

class SeedingData;

#include <plugin_SeedExtender/Direction.h>
#include <application_core/Parameters.h>
#include <structures/StaticVector.h>
#include <plugin_SeedingData/SeedingData.h>
#include <plugin_SeedExtender/ExtensionData.h>
#include <memory/RingAllocator.h>
#include <communication/BufferedData.h>
#include <profiling/TimePrinter.h>
#include <handlers/SlaveModeHandler.h>

#include <core/ComputeCore.h>

#include <vector>
#include <map>
#include <set>
using namespace std;


/*
 * Contains information regarding fusion of extensions.
 * This is a legacy class with public attributes, 
 * TODO: make attributes private..
 *
 * \author Sébastien Boisvert
 */
class FusionData : public CorePlugin {

	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS;
	MessageTag RAY_MPI_TAG_DISTRIBUTE_FUSIONS_FINISHED;
	MessageTag RAY_MPI_TAG_FINISH_FUSIONS_FINISHED;
	MessageTag RAY_MPI_TAG_GET_PATH_LENGTH;
	MessageTag RAY_MPI_TAG_GET_PATH_VERTEX;
	MessageTag RAY_MPI_TAG_SAVE_WAVE_PROGRESSION_WITH_REPLY;

	SlaveMode RAY_SLAVE_MODE_DO_NOTHING;
	SlaveMode RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS;


	/** debug fusion code */
	bool m_debugFusionCode;

	/* indicator for checkpoint */
	bool m_processedCheckpoint;

	SplayTree<Kmer ,Direction*>m_cacheForRepeatedVertices;
	MyAllocator m_cacheAllocator;

	bool m_mappingConfirmed;
	int m_validationPosition;
	bool m_checkedValidity;
	int*m_mode;

	SeedingData*m_seedingData;
	int m_wordSize;
	bool m_FINISH_vertex_requested;
	int m_size;
	int m_rank;
	ExtensionData*m_ed;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	BufferedData m_buffers;
	int m_ready;
	vector<vector<Direction> >*m_FINISH_pathsForPosition;

	int m_FINISH_positionStart;
	bool m_FINISH_hasHit;

	PathHandle m_selectedPath;
	int m_selectedPosition;

public:

	bool m_FINISH_vertex_received;
	bool m_FINISH_fusionOccured;
	vector<vector<Kmer > > m_FINISH_newFusions;
	vector<int> m_FINISH_coverages;
	map<PathHandle,int> m_FINISH_pathLengths;
	Kmer  m_FINISH_received_vertex;

	bool m_Machine_getPaths_INITIALIZED;
	bool m_Machine_getPaths_DONE;
	vector<Direction> m_Machine_getPaths_result;

	// FUSION
	bool m_fusionStarted;
	bool m_FUSION_direct_fusionDone;
	bool m_FUSION_first_done;
	int m_FUSION_numberOfRanksDone;
	bool m_FUSION_last_done;
	int m_FUSION_path_id;
	int m_FUSION_numberOfPaths;
	bool m_FUSION_paths_requested;
	bool m_FUSION_paths_received;
	vector<Direction> m_FUSION_firstPaths;
	bool m_FUSION_path_received;
	int m_FUSION_receivedLength;
	bool m_FUSION_reverse_fusionDone;
	vector<Direction> m_FUSION_lastPaths;
	vector<uint64_t> m_FUSION_matches;
	bool m_FUSION_matches_done;
	bool m_FUSION_pathLengthReceived;
	bool m_FUSION_matches_length_done;
	int m_FUSION_match_index;
	bool m_FUSION_pathLengthRequested;
	vector<Direction> m_FUSION_receivedPaths;
	bool m_FUSION_path_requested;
	Direction m_FUSION_receivedPath;
	Parameters*m_parameters;
	map<PathHandle,int> m_FUSION_identifier_map;

	set<PathHandle> m_FUSION_eliminated;

	void call_RAY_SLAVE_MODE_DISTRIBUTE_FUSIONS();

	void constructor(int size,int maxSize,int rank,StaticVector*m_outbox,
		RingAllocator*m_outboxAllocator,int wordSize,
	ExtensionData*ed,SeedingData*seedingData,int*m_mode,Parameters*parameters);

	void setReadiness();
	bool isReady();
	FusionData();

	int getRank();
	int getSize();
	void finishFusions();
	void readyBuffers();
	void getPaths(Kmer vertex);
	void initialise();

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif
