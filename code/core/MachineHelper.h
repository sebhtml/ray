/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

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

#ifndef _MachineHelper_h
#define _MachineHelper_h

#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <scheduling/SwitchMan.h>
#include <handlers/MasterModeHandler.h>
#include <handlers/SlaveModeHandler.h>
#include <assembler/ExtensionData.h>
#include <assembler/FusionData.h>
#include <profiling/Profiler.h>
#include <communication/NetworkTest.h>
#include <assembler/Partitioner.h>
#include <assembler/SequencesLoader.h>
#include <assembler/SeedingData.h>
#include <communication/MessagesHandler.h>
#include <scaffolder/Scaffolder.h>
#include <assembler/SeedExtender.h>
#include <profiling/TimePrinter.h>
#include <heuristics/OpenAssemblerChooser.h>
#include <assembler/BubbleData.h>
#include <search-engine/Searcher.h>
#include <structures/ArrayOfReads.h>
#include <assembler/VerticesExtractor.h>
#include <assembler/EdgePurger.h>
#include <graph/GridTable.h>
#include <assembler/KmerAcademyBuilder.h>
#include <communication/VirtualCommunicator.h>
#include <graph/CoverageGatherer.h>
#include <assembler/SequencesIndexer.h>
#include <core/ComputeCore.h>

#include <stdint.h>
#include <map>
using namespace std;

#include <core/MachineHelper_adapters.h>

/** this file contains __legacy code__
 * Old handlers are here.
 * TODO: move them elsewhere ?
 * \author Sébastien Boisvert */
class MachineHelper: public CorePlugin{

	Adapter_RAY_MASTER_MODE_LOAD_CONFIG m_adapter_RAY_MASTER_MODE_LOAD_CONFIG;
	Adapter_RAY_MASTER_MODE_SEND_COVERAGE_VALUES m_adapter_RAY_MASTER_MODE_SEND_COVERAGE_VALUES;
	Adapter_RAY_MASTER_MODE_WRITE_KMERS m_adapter_RAY_MASTER_MODE_WRITE_KMERS;
	Adapter_RAY_MASTER_MODE_LOAD_SEQUENCES m_adapter_RAY_MASTER_MODE_LOAD_SEQUENCES;
	Adapter_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION m_adapter_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION;
	Adapter_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING m_adapter_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING;
	Adapter_RAY_MASTER_MODE_PURGE_NULL_EDGES m_adapter_RAY_MASTER_MODE_PURGE_NULL_EDGES;
	Adapter_RAY_MASTER_MODE_TRIGGER_INDEXING m_adapter_RAY_MASTER_MODE_TRIGGER_INDEXING;
	Adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS m_adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS;
	Adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS m_adapter_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS;
	Adapter_RAY_MASTER_MODE_PREPARE_SEEDING m_adapter_RAY_MASTER_MODE_PREPARE_SEEDING;
	Adapter_RAY_MASTER_MODE_TRIGGER_SEEDING m_adapter_RAY_MASTER_MODE_TRIGGER_SEEDING;
	Adapter_RAY_MASTER_MODE_TRIGGER_DETECTION m_adapter_RAY_MASTER_MODE_TRIGGER_DETECTION;
	Adapter_RAY_MASTER_MODE_ASK_DISTANCES m_adapter_RAY_MASTER_MODE_ASK_DISTANCES;
	Adapter_RAY_MASTER_MODE_START_UPDATING_DISTANCES m_adapter_RAY_MASTER_MODE_START_UPDATING_DISTANCES;
	Adapter_RAY_MASTER_MODE_TRIGGER_EXTENSIONS m_adapter_RAY_MASTER_MODE_TRIGGER_EXTENSIONS;
	Adapter_RAY_MASTER_MODE_TRIGGER_FUSIONS m_adapter_RAY_MASTER_MODE_TRIGGER_FUSIONS;
	Adapter_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS m_adapter_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS;
	Adapter_RAY_MASTER_MODE_START_FUSION_CYCLE m_adapter_RAY_MASTER_MODE_START_FUSION_CYCLE;
	Adapter_RAY_MASTER_MODE_ASK_EXTENSIONS m_adapter_RAY_MASTER_MODE_ASK_EXTENSIONS;
	Adapter_RAY_MASTER_MODE_SCAFFOLDER m_adapter_RAY_MASTER_MODE_SCAFFOLDER;
	Adapter_RAY_MASTER_MODE_KILL_RANKS m_adapter_RAY_MASTER_MODE_KILL_RANKS;
	Adapter_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS m_adapter_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS;
	Adapter_RAY_SLAVE_MODE_WRITE_KMERS m_adapter_RAY_SLAVE_MODE_WRITE_KMERS;
	Adapter_RAY_SLAVE_MODE_ASSEMBLE_WAVES m_adapter_RAY_SLAVE_MODE_ASSEMBLE_WAVES;
	Adapter_RAY_SLAVE_MODE_SEND_EXTENSION_DATA m_adapter_RAY_SLAVE_MODE_SEND_EXTENSION_DATA;
	Adapter_RAY_SLAVE_MODE_EXTENSION m_adapter_RAY_SLAVE_MODE_EXTENSION;
	Adapter_RAY_SLAVE_MODE_DIE m_adapter_RAY_SLAVE_MODE_DIE;

	SequencesLoader*m_sl;
	time_t*m_lastTime;
	bool*m_writeKmerInitialised;
	Partitioner*m_partitioner;
	map<int,map<int,uint64_t> > m_edgeDistribution;

	VirtualCommunicator*m_virtualCommunicator;
	KmerAcademyBuilder*m_kmerAcademyBuilder;

	int*m_mode_send_vertices_sequence_id;
	CoverageGatherer*m_coverageGatherer;
	GridTable*m_subgraph;
	SequencesIndexer*m_si;

	ArrayOfReads*m_myReads;
	int*m_last_value;
	VerticesExtractor*m_verticesExtractor;
	EdgePurger*m_edgePurger;

	int m_coverageRank;
	Searcher*m_searcher;

	int*m_numberOfRanksDoneSeeding;
	int*m_numberOfRanksDoneDetectingDistances;
	int*m_numberOfRanksDoneSendingDistances;
	bool m_loadSequenceStep;

	bool m_cycleStarted;
	int*m_CLEAR_n;
	int*m_DISTRIBUTE_n;
	int*m_FINISH_n;
	OpenAssemblerChooser*m_oa;
	bool*m_isFinalFusion;
	BubbleData*m_bubbleData;
	bool*m_alive;
	TimePrinter*m_timePrinter;
	SeedExtender*m_seedExtender;
	Scaffolder*m_scaffolder;
	MessagesHandler*m_messagesHandler;

	bool m_coverageInitialised;
	int m_currentCycleStep;
	int m_cycleNumber;
	ExtensionData*m_ed;
	FusionData*m_fusionData;
	Profiler*m_profiler;
	NetworkTest*m_networkTest;
	SeedingData*m_seedingData;

	bool m_mustStop;
	bool*m_reductionOccured;
	/** indicator of the killer initialization */
	bool m_initialisedKiller;

	int m_machineRank;
	int m_numberOfRanksDone;

	int*m_numberOfMachinesDoneSendingVertices;
	bool*m_initialisedAcademy;
	int*m_repeatedLength;
	int*m_readyToSeed;
	int*m_ranksDoneAttachingReads;
	// SEQUENCE DISTRIBUTION
	bool m_reverseComplementVertex;

	int m_argc;
	char**m_argv;
	Parameters*m_parameters;
	SwitchMan*m_switchMan;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	bool*m_aborted;
	map<int,uint64_t>*m_coverageDistribution;
	int*m_numberOfMachinesDoneSendingCoverage;
	int*m_numberOfRanksWithCoverageData;

	int getRank();
	int getSize();

public:
	void constructor(int argc,char**argv,Parameters*parameters,
		SwitchMan*switchMan,RingAllocator*outboxAllocator,
		StaticVector*outbox,bool*aborted,
	map<int,uint64_t>*coverageDistribution,
	int*numberOfMachinesDoneSendingCoverage,int*numberOfRanksWithCoverageData,
bool*reductionOccured,ExtensionData*ed,FusionData*fusionData,
Profiler*p,NetworkTest*nt,SeedingData*sd,
TimePrinter*timePrinter,SeedExtender*seedExtender,Scaffolder*scaffolder,MessagesHandler*messagesHandler,
StaticVector*inbox,	OpenAssemblerChooser*oa,	bool*isFinalFusion,	BubbleData*bubbleData, bool*alive,
 int*CLEAR_n,int*DISTRIBUTE_n,int*FINISH_n,Searcher*searcher,
	int*numberOfRanksDoneSeeding,	int*numberOfRanksDoneDetectingDistances,	int*numberOfRanksDoneSendingDistances,
	ArrayOfReads*myReads,	int*last_value,	VerticesExtractor*verticesExtractor,	EdgePurger*edgePurger,
int*mode_send_vertices_sequence_id,CoverageGatherer*coverageGatherer,GridTable*m_subgraph,SequencesIndexer*m_si,
VirtualCommunicator*virtualCommunicator,KmerAcademyBuilder*kmerAcademyBuilder,
	int*numberOfMachinesDoneSendingVertices,
	bool*initialisedAcademy,
	int*repeatedLength,
	int*readyToSeed,
	int*ranksDoneAttachingReads,
SequencesLoader*sl,time_t*lastTime,bool*writeKmerInitialised,Partitioner*partitioner
);


	void call_RAY_MASTER_MODE_LOAD_CONFIG();
	void call_RAY_MASTER_MODE_SEND_COVERAGE_VALUES();
	void call_RAY_MASTER_MODE_WRITE_KMERS();
	void call_RAY_MASTER_MODE_LOAD_SEQUENCES();
	void call_RAY_MASTER_MODE_TRIGGER_VERTICE_DISTRIBUTION();
	void call_RAY_MASTER_MODE_TRIGGER_GRAPH_BUILDING();
	void call_RAY_MASTER_MODE_PURGE_NULL_EDGES();
	void call_RAY_MASTER_MODE_TRIGGER_INDEXING();
	void call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS();
	void call_RAY_MASTER_MODE_PREPARE_DISTRIBUTIONS_WITH_ANSWERS();
	void call_RAY_MASTER_MODE_PREPARE_SEEDING();
	void call_RAY_MASTER_MODE_TRIGGER_SEEDING();
	void call_RAY_MASTER_MODE_TRIGGER_DETECTION();
	void call_RAY_MASTER_MODE_ASK_DISTANCES();
	void call_RAY_MASTER_MODE_START_UPDATING_DISTANCES();
	void call_RAY_MASTER_MODE_TRIGGER_EXTENSIONS();
	void call_RAY_MASTER_MODE_TRIGGER_FUSIONS();
	void call_RAY_MASTER_MODE_TRIGGER_FIRST_FUSIONS();
	void call_RAY_MASTER_MODE_START_FUSION_CYCLE();
	void call_RAY_MASTER_MODE_ASK_EXTENSIONS();
	void call_RAY_MASTER_MODE_SCAFFOLDER();
	void call_RAY_MASTER_MODE_KILL_RANKS();
	void call_RAY_MASTER_MODE_KILL_ALL_MPI_RANKS();

	void call_RAY_SLAVE_MODE_WRITE_KMERS();
	void call_RAY_SLAVE_MODE_ASSEMBLE_WAVES();
	void call_RAY_SLAVE_MODE_SEND_EXTENSION_DATA();
	void call_RAY_SLAVE_MODE_DIE();

	void registerPlugin(ComputeCore*core);
};

#endif


