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
 * See the initialize method for steps.
 *
 * \author Sébastien Boisvert
 */
void AnnihilationWorker::work(){

/*
 * This is only useful for bubbles I think. -Séb
 */
	if(m_seed->size() > 3 * m_parameters->getWordSize()){
		m_done = true;

	}else if(m_step == STEP_CHECK_DEAD_END_ON_THE_LEFT){

		checkDeadEndOnTheLeft();

	}else if(m_step == STEP_CHECK_DEAD_END_ON_THE_RIGHT){

		checkDeadEndOnTheRight();

	}else if(m_step==STEP_FETCH_FIRST_PARENT){

		Kmer startingPoint;
		int index = 0;
		m_seed->at(index, &startingPoint);

		if(!fetchObjectMetaData(&startingPoint)){

		}else{

			if(m_parents.size() != 1){

				m_done = true;
			}else{

				m_parent=m_parents[0];
			}

			m_step ++;

			m_initializedFetcher = false;
		}

// TODO: the code path for STEP_FETCH_SECOND_PARENT is mostly identical to that of STEP_FETCH_FIRST_PARENT
// so maybe this should be pushed in a method.

	}else if(m_step == STEP_FETCH_SECOND_PARENT){

		if(!fetchObjectMetaData(&m_parent)){
			// this is not yet finished.
		}else{
			if(m_parents.size() != 1){

				m_done = true;
			}else{

				m_grandparent=m_parents[0];
			}

			m_step ++;
			m_queryWasSent = false;

			m_initializedFetcher = false;
		}

	}else if(m_step == STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS){

		if(!m_queryWasSent){

			m_queryWasSent = true;

		}else{
			m_step ++;
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

void AnnihilationWorker::checkDeadEndOnTheLeft(){

	if(!m_startedToCheckDeadEndOnTheLeft){

		m_startedToCheckDeadEndOnTheLeft=true;
	}else{
		m_step++;

		m_initializedFetcher = false;
	}
}

void AnnihilationWorker::checkDeadEndOnTheRight(){

	if(!m_startedToCheckDeadEndOnTheRight){

		m_startedToCheckDeadEndOnTheRight=true;
	}else{
		m_step++;

		m_initializedFetcher = false;
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

	STEP_CHECK_DEAD_END_ON_THE_LEFT = stepValue ++;
	STEP_CHECK_DEAD_END_ON_THE_RIGHT = stepValue ++;
	STEP_FETCH_FIRST_PARENT = stepValue ++;
	STEP_FETCH_SECOND_PARENT = stepValue ++;
	STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS = stepValue ++;
	STEP_GET_SEED_SEQUENCE_NOW = stepValue ++;

	m_step = STEP_CHECK_DEAD_END_ON_THE_LEFT;

	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;
	m_rank = m_parameters->getRank();
	m_outboxAllocator = outboxAllocator;

	m_startedToCheckDeadEndOnTheLeft = false;
	m_startedToCheckDeadEndOnTheRight = false;
}

bool AnnihilationWorker::fetchObjectMetaData(Kmer * object){

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
#ifdef ASSERT
		assert(m_depth>= 1);
#endif

		m_parents = object->getIngoingEdges(edges, m_parameters->getWordSize());
		m_children = object->getOutgoingEdges(edges, m_parameters->getWordSize());

		m_queryWasSent = false;

		return true;
	}

	return false;
}
