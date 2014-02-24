/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
 *  Copyright (C) 2013, 2014 Sébastien Boisvert
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

#ifndef _AttributeFetcher_h
#define _AttributeFetcher_h

#include <code/KmerAcademyBuilder/Kmer.h>
#include <code/Mock/Parameters.h>

#include <RayPlatform/communication/VirtualCommunicator.h>
#include <RayPlatform/scheduling/Worker.h>

#include <vector>
using namespace std;

/**
 * This is a building block to fetch the parents, the children
 * and the coverage of a k-mer.
 *
 * Compatible with the API of:
 *
 * * VirtualCommunicator
 * * VirtualProcessor
 * * Ray message pool definition
 *
 * \author Sébastien Boisvert
 */
class AttributeFetcher{

private:

	Rank m_rank;
	Parameters*m_parameters;
	VirtualCommunicator*m_virtualCommunicator;
	vector<Kmer> m_parents;
	vector<Kmer> m_children;
	CoverageDepth m_depth;

	WorkerHandle m_identifier;
	RingAllocator * m_outboxAllocator;
	bool m_initializedFetcher;
	bool m_queryWasSent;

	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
public:

	/**
	 * Initializes the object.
	 *
	 * This must be called only once.
	 */
	void initialize(Parameters*parameters, VirtualCommunicator*virtualCommunicator,
			WorkerHandle identifier, RingAllocator * outboxAllocator,
			MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);

	/**
	 * Fetches the parents, the children and the coverage of a k-mer.
	 *
	 * @returns true ifresult is available, false if additional calls are required
	 * for the request to complete.
	 */
	bool fetchObjectMetaData(Kmer * object);

	/**
	 * Resets the the object so that fetchObjectMetaData can be called on another object.
	 *
	 */
	void reset();

	/**
	 * Gets the coverage depth of the submitted object
	 *
	 * fetchObjectMetaData must have returned true before calling this.
	 */
	CoverageDepth getDepth();

	/**
	 * Gets the parents of a k-mer.
	 *
	 * fetchObjectMetaData must have returned true before calling this.
	 */
	vector<Kmer>* getParents();

	/**
	 * Gets the children of a k-mer.
	 *
	 * fetchObjectMetaData must have returned true before calling this.
	 */
	vector<Kmer>* getChildren();
};

#endif
