/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#ifndef _MessageProcessor
#define _MessageProcessor

#include <memory/RingAllocator.h>
#include <assembler/Library.h>
#include <assembler/SequencesIndexer.h>
#include <assembler/SeedingData.h>
#include <assembler/OpenAssemblerChooser.h>
#include <structures/ArrayOfReads.h>
#include <communication/Message.h>
#include <vector>
#include <structures/SplayTree.h>
#include <structures/StaticVector.h>
#include <assembler/SeedExtender.h>
#include <communication/MessagesHandler.h>
#include <assembler/SequencesLoader.h>
#include <assembler/FusionData.h>
#include <structures/ReadAnnotation.h>
#include <assembler/VerticesExtractor.h>
#include <structures/MyForest.h>
#include <graph/GridTable.h>
#include <core/Parameters.h>
#include <assembler/MemoryConsumptionReducer.h>
#include <communication/BufferedData.h>
#include <memory/MyAllocator.h>
#include <structures/Vertex.h>
#include <scaffolder/Scaffolder.h>
#include <communication/VirtualCommunicator.h>
using namespace std;


class MessageProcessor;
typedef void (MessageProcessor::*FNMETHOD) (Message*message);

class MessageProcessor{
	VirtualCommunicator*m_virtualCommunicator;
	Scaffolder*m_scaffolder;
	int m_count;

	MemoryConsumptionReducer*m_reducer;
	uint64_t m_lastSize;

	SequencesLoader*m_sequencesLoader;

	FNMETHOD m_methods[256];

	uint64_t m_sentinelValue;

	SeedingData*m_seedingData;

	// data for processing
	bool*m_ready;
	int m_consumed;
	time_t m_last;

	Library*m_library;
	bool*m_isFinalFusion;
	int*m_master_mode;
	ExtensionData*m_ed;
	int*m_numberOfRanksDoneDetectingDistances;
	int*m_numberOfRanksDoneSendingDistances;
	Parameters*m_parameters;

	MessagesHandler*m_messagesHandler;

	GridTable*m_subgraph;

	RingAllocator*m_outboxAllocator;
	int rank;
	int*m_numberOfMachinesDoneSendingEdges;
	FusionData*m_fusionData;
	int*m_wordSize;
	int*m_minimumCoverage;
	int*m_seedCoverage;
	int*m_peakCoverage;
	ArrayOfReads*m_myReads;
	int size;
	SequencesIndexer*m_si;

	RingAllocator*m_inboxAllocator;
	MyAllocator*m_persistentAllocator;
	vector<uint64_t>*m_identifiers;
	bool*m_mode_sendDistribution;
	bool*m_alive;
	int*m_mode;
	vector<vector<uint64_t> >*m_allPaths;
	int*m_last_value;
	int*m_ranksDoneAttachingReads;
	int*m_DISTRIBUTE_n;
	int*m_numberOfRanksDoneSeeding;
	int*m_CLEAR_n;
	int*m_readyToSeed;
	int*m_FINISH_n;
	bool*m_nextReductionOccured;
	MyAllocator*m_directionsAllocator;
	int*m_mode_send_coverage_iterator;
	map<int,uint64_t>*m_coverageDistribution;
	int*m_sequence_ready_machines;
	int*m_numberOfMachinesReadyForEdgesDistribution;
	int*m_numberOfMachinesReadyToSendDistribution;
	bool*m_mode_send_outgoing_edges;
	int*m_mode_send_vertices_sequence_id;
	bool*m_mode_send_vertices;
	int*m_numberOfMachinesDoneSendingVertices;
	VerticesExtractor*m_verticesExtractor;
	int*m_numberOfMachinesDoneSendingCoverage;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	map<int,int>*m_allIdentifiers;
	OpenAssemblerChooser*m_oa;
	int*m_numberOfRanksWithCoverageData;
	SeedExtender*seedExtender;

	void assignHandlers();

	/*
 * generate prototypes with the list and a macro
 */
	#define MACRO_LIST_ITEM(x) void call_ ## x ( Message*m ) ;
	#include <communication/mpi_tag_macros.h>
	#undef MACRO_LIST_ITEM
	
public:
	void constructor(MessagesHandler*m_messagesHandler,
SeedingData*seedingData,
Library*m_library,
bool*m_ready,
VerticesExtractor*m_verticesExtractor,
SequencesLoader*m_sequencesLoader,
ExtensionData*ed,
			int*m_numberOfRanksDoneDetectingDistances,
			int*m_numberOfRanksDoneSendingDistances,
			Parameters*parameters,
			GridTable*m_subgraph,
			RingAllocator*m_outboxAllocator,
				int rank,
			int*m_numberOfMachinesDoneSendingEdges,
			FusionData*m_fusionData,
			int*m_wordSize,
			int*m_minimumCoverage,
			int*m_seedCoverage,
			int*m_peakCoverage,
			ArrayOfReads*m_myReads,
		int size,
	RingAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<uint64_t>*m_identifiers,
	bool*m_mode_sendDistribution,
	bool*m_alive,
	int*m_mode,
	vector<vector<uint64_t> >*m_allPaths,
	int*m_last_value,
	int*m_ranksDoneAttachingReads,
	int*m_DISTRIBUTE_n,
	int*m_numberOfRanksDoneSeeding,
	int*m_CLEAR_n,
	int*m_readyToSeed,
	int*m_FINISH_n,
	bool*m_nextReductionOccured,
	MyAllocator*m_directionsAllocator,
	int*m_mode_send_coverage_iterator,
	map<int,uint64_t>*m_coverageDistribution,
	int*m_sequence_ready_machines,
	int*m_numberOfMachinesReadyForEdgesDistribution,
	int*m_numberOfMachinesReadyToSendDistribution,
	bool*m_mode_send_outgoing_edges,
	int*m_mode_send_vertices_sequence_id,
	bool*m_mode_send_vertices,
	int*m_numberOfMachinesDoneSendingVertices,
	int*m_numberOfMachinesDoneSendingCoverage,
				StaticVector*m_outbox,
				StaticVector*m_inbox,
		OpenAssemblerChooser*m_oa,
int*m_numberOfRanksWithCoverageData,
SeedExtender*seedExtender,int*m_master_mode,bool*m_isFinalFusion,
SequencesIndexer*m_si
);

	void processMessage(Message*message);
	MessageProcessor();

	void flushBuffers();
	void setReducer(MemoryConsumptionReducer*reducer);
	void setScaffolder(Scaffolder*a);
	void setVirtualCommunicator(VirtualCommunicator*a);
};

#endif
