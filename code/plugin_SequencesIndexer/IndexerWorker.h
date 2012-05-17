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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _IndexerWorker
#define _IndexerWorker

#include <application_core/Parameters.h>
#include <memory/RingAllocator.h>
#include <communication/VirtualCommunicator.h>
#include <stdint.h>
#include <memory/MyAllocator.h>
#include <plugin_SequencesLoader/ArrayOfReads.h>
#include <plugin_SequencesIndexer/DynamicVector.h>
#include <scheduling/Worker.h>
#include <fstream>
using namespace std;

/**
 * this class is a worker for sequence indexing
 * Optimal read markers are selected by IndexerWorker
 * \author Sébastien Boisvert
 */

// TODO: convert this to Worker
class IndexerWorker /*: public Worker*/{

	MessageTag RAY_MPI_TAG_ATTACH_SEQUENCE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;

	map<int,map<int,int> >*m_forwardStatistics;
	map<int,map<int,int> >*m_reverseStatistics;

	ofstream*m_readMarkerFile;

	ArrayOfReads*m_reads;
	int m_sequenceId;
	bool m_done;
	int m_position;
	VirtualCommunicator*m_virtualCommunicator;
	bool m_indexedTheVertex;
	Parameters*m_parameters;
	int m_workerId;
	bool m_checkedCoverage;
	RingAllocator*m_outboxAllocator;
	bool m_forwardIndexed;
	bool m_reverseIndexed;
	bool m_vertexInitiated;
	bool m_vertexIsDone;
	bool m_coverageRequested;
	bool m_fetchedCoverageValues;
	MyAllocator*m_allocator;

	DynamicVector<Kmer> m_vertices;
	DynamicVector<int> m_coverages;

	int getThreshold(vector<int>*data);

public:
	void constructor(int sequenceId,Parameters*parameters,RingAllocator*outboxAllocator,
		VirtualCommunicator*vc,WorkerHandle workerId,ArrayOfReads*a,MyAllocator*allocator,
	ofstream*f,
	map<int,map<int,int> >*forwardStatistics,
	map<int,map<int,int> >*reverseStatistics,
	MessageTag RAY_MPI_TAG_ATTACH_SEQUENCE,
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE
);

/** work a little bit 
	 * the class Worker provides no implementation for that 
	*/
	void work();

	/** is the worker done doing its things */
	bool isDone();

	/** get the worker number */
	WorkerHandle getWorkerIdentifier();

};

#endif
