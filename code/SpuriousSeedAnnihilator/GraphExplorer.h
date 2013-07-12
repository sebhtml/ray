/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
 *  Copyright (C) 2013 Sébastien Boisvert
 *
 *  http://DeNovoAssembler.SourceForge.Net/
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You have received a copy of the GNU General Public License
 *  along with this program (gpl-3.0.txt).
 *  see <http://www.gnu.org/licenses/>
 */

#ifndef GraphExplorer_header
#define GraphExplorer_header

#include "AttributeFetcher.h"
#include "AnnotationFetcher.h"

#include <code/Mock/Parameters.h>
#include <code/Mock/Parameters.h>

#include <RayPlatform/core/ComputeCore.h>
#include <RayPlatform/communication/VirtualCommunicator.h>

#include <stack>
using namespace std;

#define EXPLORER_LEFT 0x89
#define EXPLORER_RIGHT 0x452

/**
 * This class is an explorer to find paths leading to new paths.
 *
 * \author Sébastien Boisvert
 */
class GraphExplorer {

	stack<Kmer> m_currentPath;

	stack<Kmer> m_verticesToVisit;
	stack<int> m_depths;

	AnnotationFetcher m_annotationFetcher;
	AttributeFetcher m_attributeFetcher;

	WorkerHandle m_key;
	int m_direction;
	PathHandle m_seedName;

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;

	bool m_haveAttributes;
	bool m_haveAnnotations;

	bool m_stopAtFirstHit;
	bool m_done;
	int m_maximumDepth;

	VirtualCommunicator * m_virtualCommunicator;
	Parameters * m_parameters;
	int m_maximumVisitedVertices;
	int m_visitedVertices;
	RingAllocator*m_outboxAllocator;
public:
	void start(WorkerHandle worker, Kmer * start, int direction, Parameters * parameters,
		VirtualCommunicator * virtualCommunicator,
		RingAllocator * outboxAllocator,
		MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
		MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH
	);

	bool work();
};

#endif /* GraphExplorer_header */


