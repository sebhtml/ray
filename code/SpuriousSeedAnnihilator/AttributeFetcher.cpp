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

#include "AttributeFetcher.h"

bool AttributeFetcher::fetchObjectMetaData(Kmer * object){

	if(!m_initializedFetcher){

		m_queryWasSent=false;
		m_initializedFetcher=true;

	}else if(!m_queryWasSent){

		MessageTag tag = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;

		Rank destination = m_parameters->vertexRank(object);
		MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
		int bufferPosition=0;
		object->pack(message,&bufferPosition);
		Message aMessage(message,m_virtualCommunicator->getElementsPerQuery(tag),
			destination, tag, m_rank);

		m_virtualCommunicator->pushMessage(m_identifier, &aMessage);

		m_queryWasSent = true;

	}else if(m_virtualCommunicator->isMessageProcessed(m_identifier)){

		vector<MessageUnit> elements;
		m_virtualCommunicator->getMessageResponseElements(m_identifier, &elements);

		int bufferPosition=0;

		uint8_t edges = elements[bufferPosition++];

		m_depth=elements[bufferPosition++];
#ifdef CONFIG_ASSERT
		assert(m_depth>= 1);
#endif

		m_parents = object->getIngoingEdges(edges, m_parameters->getWordSize());
		m_children = object->getOutgoingEdges(edges, m_parameters->getWordSize());

		m_queryWasSent = false;

		return true;
	}

	return false;
}

void AttributeFetcher::initialize(Parameters*parameters, VirtualCommunicator*virtualCommunicator,
		WorkerHandle identifier, RingAllocator * outboxAllocator,
		MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT){

	m_virtualCommunicator = virtualCommunicator;
	m_parameters = parameters;
	m_identifier = identifier;
	m_outboxAllocator = outboxAllocator;
	m_rank = m_parameters->getRank();
	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;

	reset();
}

void AttributeFetcher::reset(){
	m_initializedFetcher = false;
}

CoverageDepth AttributeFetcher::getDepth(){
	return m_depth;
}

vector<Kmer>* AttributeFetcher::getParents(){
	return &m_parents;
}

vector<Kmer>* AttributeFetcher::getChildren(){
	return &m_children;
}
