/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

\file GenomeNeighbourhood.h
\author Sébastien Boisvert
*/


#ifndef _GenomeNeighbourhood_h
#define _GenomeNeighbourhood_h

#include <profiling/TimePrinter.h>
#include <core/ComputeCore.h>
#include <plugins/CorePlugin.h>
#include <plugin_GenomeNeighbourhood/Neighbour.h>
#include <plugin_GenomeNeighbourhood/NeighbourPair.h>
#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <communication/VirtualCommunicator.h>
#include <application_core/Parameters.h>

#include <vector>
#include <string>
#include <stdint.h> /* for uint64_t */
#include <stack>
using namespace std;


/**
 * The plugin GenomeNeighbourhood outputs a file file
 * containing contig neighbourhoods.
 *
 * This is useful to know where is located a drug-resistance gene,
 * amongst other things.
 *
 * \author Sébastien Boisvert
 * \
 * */
class GenomeNeighbourhood: public CorePlugin{

	bool m_pluginIsEnabled;
	Parameters*m_parameters;

	stack<Kmer> m_stackOfVertices;
	stack<int> m_stackOfDepths;

/** states of the state machine */
	int m_contigIndex;
	bool m_started;
	bool m_slaveStarted;
	bool m_doneLeftSide;
	bool m_doneRightSide;
	bool m_startedLeft;
	bool m_startedRight;

	bool m_doneSide;
	bool m_startedSide;

	/* graph surfing */
	set<Kmer> m_visited;
	set<PathHandle > m_foundContigs;
	int m_maximumDepth;
	bool m_linksRequested;
	bool m_linksReceived;
	int m_paths;
	bool m_foundPathsForThisVertex;
	int m_pathIndex;
	bool m_numberOfPathsRequested;
	bool m_numberOfPathsReceived;
	bool m_fetchedPaths;
	bool m_receivedPath;
	bool m_requestedPath;
	bool m_reverseStrand;
	
	bool m_directDone;
	bool m_reverseDone;

	/* virtual communication */
	VirtualCommunicator*m_virtualCommunicator;
	vector<WorkerHandle> m_activeWorkers;
	Rank m_rank;
	WorkerHandle m_workerId;
	RingAllocator*m_outboxAllocator;

	ComputeCore*m_core;

	TimePrinter*m_timePrinter;

	SlaveMode RAY_SLAVE_MODE_NEIGHBOURHOOD;

	MasterMode RAY_MASTER_MODE_KILL_RANKS;
	MasterMode RAY_MASTER_MODE_NEIGHBOURHOOD;

	MessageTag RAY_MPI_TAG_NEIGHBOURHOOD;
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;
	MessageTag RAY_MPI_TAG_NEIGHBOURHOOD_DATA;
	MessageTag RAY_MPI_TAG_NEIGHBOURHOOD_DATA_REPLY;


	/** contig paths */
	vector<vector<Kmer> >*m_contigs;
	vector<PathHandle>*m_contigNames;

/** genome folks in the neighbourhood **/
	vector<Neighbour> m_leftNeighbours;
	vector<Neighbour> m_rightNeighbours;

	vector<NeighbourPair> m_finalList;

/** sending neighbours **/

	bool m_sentRightNeighbours;
	bool m_sentLeftNeighbours;
	int m_neighbourIndex;
	bool m_selectedHits;
	bool m_receivedReply;
	bool m_sentEntry;

	map<PathHandle,int>*m_contigLengths;

/** private parts **/

	void createStacks(Kmer a);
	void processSide(int mode);
	void processLinks(int mode);
	void resetKmerStates();
	void fetchPaths(int mode);
	void selectHits();
	void sendLeftNeighbours();
	void sendRightNeighbours();
	void processFinalList();

public:

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

	void call_RAY_SLAVE_MODE_NEIGHBOURHOOD();
	void call_RAY_MASTER_MODE_NEIGHBOURHOOD();
	void call_RAY_MPI_TAG_NEIGHBOURHOOD_DATA(Message*message);
};

#endif
