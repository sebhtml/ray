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

#include "AnnihilationWorker.h"

/**
 *
 * This is a bubble caused by a polymorphism (a SNP).
 *
 * For sequencing error, one of the branches will be weak.
 *
 *
 *                  32     31     29     28     27     31     33     34     27
 *
 *                  (x)--->(x)--->(x)--->(x)--->(x)--->(x)--->(x)--->(x)--->(x)
 *  63    64    61  /
 *                 /      =====================================================>
 *  (x)---(x)--->(x)
 *                 \      =====================================================>
 *               |  \
 *               |  (x)--->(x)--->(x)--->(x)--->(x)--->(x)--->(x)--->(x)--->(x)
 *               |
 *               |  25     26     27     29     30     31     32     26     29
 *               |
 *               |  |
 *               |  |
 *               |  |
 *               |  v
 *               |  STEP_FETCH_FIRST_PARENT operation
 *               |
 *               v
 *               STEP_FETCH_SECOND_PARENT operation
 *               |
 *               |
 *               |
 *               |
 *               v
 *               STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS operation
 *
 * Algorithm:
 *
 * 1. STEP_FETCH_FIRST_PARENT
 * 2. STEP_FETCH_SECOND_PARENT
 * 3. STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS
 * 4. STEP_GET_SEED_SEQUENCE_NOW
 *
 * \author Sébastien Boisvert
 */
void AnnihilationWorker::work(){

/*
 * This is only useful for bubbles I think. -Séb
 */
	if(m_seed->size() > 3 * m_parameters->getWordSize()){
		m_done = true;

	}else if(m_step==STEP_FETCH_FIRST_PARENT){

		if(!m_queryWasSent){
			if(m_seed->size() == 0){
				m_done = true;
				return;
			}

			MessageTag tag = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;

			Kmer startingPoint;
			int index = 0;
			m_seed->at(index, &startingPoint);

			Rank destination = m_parameters->vertexRank(&startingPoint);
			MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
			int bufferPosition=0;
			startingPoint.pack(message,&bufferPosition);
			Message aMessage(message,m_virtualCommunicator->getElementsPerQuery(tag),
				destination, tag, m_rank);

			m_virtualCommunicator->pushMessage(m_identifier, &aMessage);

			m_queryWasSent = true;

		}else if(m_virtualCommunicator->isMessageProcessed(m_identifier)){

			vector<MessageUnit> elements;
			m_virtualCommunicator->getMessageResponseElements(m_identifier, &elements);

			int bufferPosition=0;

			uint8_t edges = elements[bufferPosition++];

#ifdef ASSERT
			CoverageDepth coverage=elements[bufferPosition++];
			assert(coverage >= 1);
#endif

			vector<Kmer> parents;
			vector<Kmer> children;

			Kmer kmer;
			int index = 0;
			m_seed->at(index, &kmer);

			parents = kmer.getIngoingEdges(edges, m_parameters->getWordSize());
			children = kmer.getOutgoingEdges(edges, m_parameters->getWordSize());

// TODO: actually the code should supports more than 1 parent for STEP_FETCH_SECOND_PARENT
// by definition, STEP_FETCH_FIRST_PARENT will always go through exactly one parent.

			if(parents.size() != 1){

				m_done = true;
			}else{

				m_parent=parents[0];
			}

			m_step = STEP_FETCH_SECOND_PARENT;
			m_queryWasSent = false;
		}

// TODO: the code path for STEP_FETCH_SECOND_PARENT is mostly identical to that of STEP_FETCH_FIRST_PARENT
// so maybe this should be pushed in a method.

	}else if(m_step == STEP_FETCH_SECOND_PARENT){

		if(!m_queryWasSent){

			MessageTag tag = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;

			MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(MessageUnit));
			int bufferPosition=0;

			Rank destination = m_parameters->vertexRank(&m_parent);
			m_parent.pack(message,&bufferPosition);
			Message aMessage(message,m_virtualCommunicator->getElementsPerQuery(tag),
				destination, tag, m_rank);

			m_virtualCommunicator->pushMessage(m_identifier, &aMessage);

			m_queryWasSent = true;

		}else if(m_virtualCommunicator->isMessageProcessed(m_identifier)){

			vector<MessageUnit> elements;
			m_virtualCommunicator->getMessageResponseElements(m_identifier, &elements);

			int bufferPosition=0;

			uint8_t edges = elements[bufferPosition++];

#ifdef ASSERT
			CoverageDepth coverage=elements[bufferPosition++];
			assert(coverage >= 1);
#endif

			vector<Kmer> parents;
			vector<Kmer> children;

			Kmer kmer;
			int index = 0;
			m_seed->at(index, &kmer);

			parents = kmer.getIngoingEdges(edges, m_parameters->getWordSize());

// TODO: this line is useless
			children = kmer.getOutgoingEdges(edges, m_parameters->getWordSize());

// TODO: actually the code should supports more than 1 parent for STEP_FETCH_SECOND_PARENT
// by definition, STEP_FETCH_FIRST_PARENT will always go through exactly one parent.

			if(parents.size() != 1){

				m_done = true;
			}else{

				m_grandparent=parents[0];
			}

			m_step = STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS;
			m_queryWasSent = false;
		}


	}else if(m_step == STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS){

		if(!m_queryWasSent){

			m_queryWasSent = true;

		}else{
			m_step = STEP_GET_SEED_SEQUENCE_NOW;
			m_queryWasSent = false;
		}

	}else if(m_step == STEP_GET_SEED_SEQUENCE_NOW){

		if(!m_queryWasSent){
			m_queryWasSent = true;
		}else{
			m_done = true;
		}
	}
}

bool AnnihilationWorker::isDone(){

	return m_done;
}

WorkerHandle AnnihilationWorker::getWorkerIdentifier(){

	return m_identifier;
}

void AnnihilationWorker::initialize(uint64_t identifier,GraphPath*seed, Parameters * parameters,
	VirtualCommunicator * virtualCommunicator, MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
	RingAllocator*outboxAllocator){

	m_identifier = identifier;
	m_done = false;

	m_seed = seed;

	m_virtualCommunicator = virtualCommunicator;
	m_parameters = parameters;

	int stepValue = 0;
	STEP_FETCH_FIRST_PARENT = stepValue ++;
	STEP_FETCH_SECOND_PARENT = stepValue ++;
	STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS = stepValue ++;
	STEP_GET_SEED_SEQUENCE_NOW = stepValue ++;

	m_step = STEP_FETCH_FIRST_PARENT;

	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	m_rank = m_parameters->getRank();
	m_outboxAllocator = outboxAllocator;
}
