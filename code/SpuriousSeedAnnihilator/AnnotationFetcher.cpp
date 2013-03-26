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

#include "AnnotationFetcher.h"

void AnnotationFetcher::initialize(Parameters*parameters, VirtualCommunicator*virtualCommunicator,
		WorkerHandle identifier, RingAllocator * outboxAllocator,
		MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
		MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH
		){

	m_virtualCommunicator = virtualCommunicator;
	m_parameters = parameters;
	m_identifier = identifier;
	m_outboxAllocator = outboxAllocator;
	m_rank = m_parameters->getRank();

	this->RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE = RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	this->RAY_MPI_TAG_ASK_VERTEX_PATH = RAY_MPI_TAG_ASK_VERTEX_PATH;

	reset();
}

void AnnotationFetcher::reset(){
	m_initializedDirectionFetcher = false;
}

bool AnnotationFetcher::fetchDirections(Kmer*kmer){

	if(!m_initializedDirectionFetcher){

		m_initializedDirectionFetcher =  true;

		m_fetchedCount = false;
		m_queryWasSent = false;

		m_directions.clear();

	}else if(!m_fetchedCount){

		if(!m_queryWasSent){
			MessageTag tag = RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
			int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(tag);

#ifdef DEBUG_LEFT_EXPLORATION
			cout << "Sending message for count" << endl;
#endif

			Rank destination = m_parameters->vertexRank(kmer);
			MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(elementsPerQuery*sizeof(MessageUnit));
			int bufferPosition=0;
			kmer->pack(message,&bufferPosition);
			Message aMessage(message, elementsPerQuery,
				destination, tag, m_rank);

			m_virtualCommunicator->pushMessage(m_identifier, &aMessage);

			m_queryWasSent = true;

		}else if(m_virtualCommunicator->isMessageProcessed(m_identifier)){
			vector<MessageUnit> elements;
			m_virtualCommunicator->getMessageResponseElements(m_identifier, &elements);

			m_numberOfPaths = elements[0];

			m_fetchedCount = true;

#ifdef DEBUG_LEFT_EXPLORATION
			cout<<"Paths: "<<m_numberOfPaths << endl;
#endif
			m_pathIndex = 0;

			m_queryWasSent = false;
		}
	}else if(m_pathIndex < m_numberOfPaths){

		m_pathIndex ++;
	}else{
		return true;
	}

	return false;
}

vector<Direction>* AnnotationFetcher::getDirections(){

	return &m_directions;
}
