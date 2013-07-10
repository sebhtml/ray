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

#ifndef _AnnotationFetcher_h
#define _AnnotationFetcher_h

#include <code/SeedExtender/Direction.h>
#include <code/KmerAcademyBuilder/Kmer.h>
#include <code/Mock/Parameters.h>

#include <RayPlatform/communication/VirtualCommunicator.h>
#include <RayPlatform/scheduling/Worker.h>

#include <vector>
using namespace std;

/**
 * This is a building block to fetch annotations.
 * This is compatible with seeds and contigs.
 *
 * \author Sébastien Boisvert
 */
class AnnotationFetcher{

	Rank m_rank;
	Parameters*m_parameters;
	VirtualCommunicator*m_virtualCommunicator;
	WorkerHandle m_identifier;
	RingAllocator * m_outboxAllocator;
	bool m_queryWasSent;
	bool m_initializedDirectionFetcher;
	bool m_fetchedCount;
	int m_pathIndex;
	int m_numberOfPaths;
	vector<Direction> m_directions;
	bool m_reverseStrand;

	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH;
public:
	void initialize(Parameters*parameters, VirtualCommunicator*virtualCommunicator,
			WorkerHandle identifier, RingAllocator * outboxAllocator,
			MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
			MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH);

	bool fetchDirections(Kmer*kmer);

	void reset();

	vector<Direction>* getDirections();
};

#endif
