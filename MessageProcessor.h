/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#ifndef _MessageProcessor
#define _MessageProcessor

#include<OpenAssemblerChooser.h>
#include<Message.h>
#include<vector>
#include<SplayTree.h>
#include<FusionData.h>
#include<ReadAnnotation.h>
#include<MyForest.h>
#include<MyAllocator.h>
#include<Vertex.h>
using namespace std;

class MessageProcessor{
public:
	void processMessage(Message*message,
			MyForest*m_subgraph,
			MyAllocator*m_outboxAllocator,
				int rank,
			vector<ReadAnnotation>*m_EXTENSION_receivedReads,
			int*m_numberOfMachinesDoneSendingEdges,
			FusionData*m_fusionData,
			vector<vector<VERTEX_TYPE> >*m_EXTENSION_contigs,
			int*m_wordSize,
			int*m_minimumCoverage,
			int*m_seedCoverage,
			int*m_peakCoverage,
			vector<Read*>*m_myReads,
			bool*m_EXTENSION_currentRankIsDone,
	vector<vector<VERTEX_TYPE> >*m_FINISH_newFusions,
		int size,
	MyAllocator*m_inboxAllocator,
	MyAllocator*m_persistentAllocator,
	vector<int>*m_identifiers,
	bool*m_mode_sendDistribution,
	bool*m_alive,
	vector<VERTEX_TYPE>*m_SEEDING_receivedIngoingEdges,
	VERTEX_TYPE*m_SEEDING_receivedKey,
	int*m_SEEDING_i,
	bool*m_colorSpaceMode,
	bool*m_FINISH_fusionOccured,
	bool*m_Machine_getPaths_INITIALIZED,
	int*m_calibration_numberOfMessagesSent,
	int*m_mode,
	vector<vector<VERTEX_TYPE> >*m_allPaths,
	bool*m_EXTENSION_VertexAssembled_received,
	int*m_EXTENSION_numberOfRanksDone,
	int*m_EXTENSION_currentPosition,
	int*m_last_value,
	vector<int>*m_EXTENSION_identifiers,
	int*m_ranksDoneAttachingReads,
	bool*m_SEEDING_edgesReceived,
	PairedRead*m_EXTENSION_pairedRead,
	bool*m_mode_EXTENSION,
	vector<VERTEX_TYPE>*m_SEEDING_receivedOutgoingEdges,
	int*m_DISTRIBUTE_n,
	vector<VERTEX_TYPE>*m_SEEDING_nodes,
	bool*m_EXTENSION_hasPairedReadReceived,
	int*m_numberOfRanksDoneSeeding,
	bool*m_SEEDING_vertexKeyAndCoverageReceived,
	int*m_SEEDING_receivedVertexCoverage,
	bool*m_EXTENSION_readLength_received,
	int*m_calibration_MaxSpeed,
	bool*m_Machine_getPaths_DONE,
	int*m_CLEAR_n,
	bool*m_FINISH_vertex_received,
	bool*m_EXTENSION_initiated,
	int*m_readyToSeed,
	bool*m_SEEDING_NodeInitiated,
	int*m_FINISH_n,
	bool*m_nextReductionOccured,
	bool*m_EXTENSION_hasPairedReadAnswer,
	MyAllocator*m_directionsAllocator,
	map<int,int>*m_FINISH_pathLengths,
	bool*m_EXTENSION_pairedSequenceReceived,
	int*m_EXTENSION_receivedLength,
	int*m_mode_send_coverage_iterator,
	map<int,VERTEX_TYPE>*m_coverageDistribution,
	VERTEX_TYPE*m_FINISH_received_vertex,
	bool*m_EXTENSION_read_vertex_received,
	int*m_sequence_ready_machines,
	bool*m_SEEDING_InedgesReceived,
	bool*m_EXTENSION_vertexIsAssembledResult,
	bool*m_SEEDING_vertexCoverageReceived,
	VERTEX_TYPE*m_EXTENSION_receivedReadVertex,
	int*m_numberOfMachinesReadyForEdgesDistribution,
	int*m_numberOfMachinesReadyToSendDistribution,
	bool*m_mode_send_outgoing_edges,
	int*m_mode_send_edge_sequence_id,
	int*m_mode_send_vertices_sequence_id,
	bool*m_mode_send_vertices,
	int*m_numberOfMachinesDoneSendingVertices,
	int*m_numberOfMachinesDoneSendingCoverage,
	bool*m_EXTENSION_reads_received,
				vector<Message>*m_outbox,
		map<int,int>*m_allIdentifiers,OpenAssemblerChooser*m_oa);
};

#endif
