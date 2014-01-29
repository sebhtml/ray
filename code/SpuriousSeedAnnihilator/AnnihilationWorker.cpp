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

//#define DEBUG_CODE_PATH

#include <stack>
using namespace std;

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
	if(m_step == STEP_CHECK_LENGTH){

#ifdef DEBUG_CODE_PATH
		cout<<"Worker " << m_identifier << " STEP_CHECK_LENGTH"<<endl;
#endif

		if(m_seed->size() > 3 * m_parameters->getWordSize()){
			m_done = true;

		}

		m_step++;

	}else if(m_step == STEP_CHECK_DEAD_END_ON_THE_LEFT){

		if(checkDeadEndOnTheLeft())
			m_step++;

	}else if(m_step == STEP_CHECK_DEAD_END_ON_THE_RIGHT){

		if(checkDeadEndOnTheRight())
			m_step++;

	}else if(m_step == STEP_CHECK_BUBBLE_PATTERNS){

		if(checkBubblePatterns())
			m_done = true;
	}
}

bool AnnihilationWorker::searchGraphForNiceThings(int direction){

	if(!m_searchIsStarted) {

#ifdef CONFIG_ASSERT
		assert(direction == DIRECTION_PARENTS || direction == DIRECTION_CHILDREN);
#endif

		while(!m_depths.empty())
			m_depths.pop();

		while(!m_vertices.empty())
			m_vertices.pop();

		int depth=0;
		m_actualMaximumDepth=0;

		Kmer startingPoint;
		int index = -1;

		if(direction == DIRECTION_PARENTS)
			index = 0;
		else if(direction == DIRECTION_CHILDREN)
			index = m_seed->size() -1;

#ifdef CONFIG_ASSERT
		assert(index == 0 || index == m_seed->size()-1);
#endif
		m_seed->at(index, &startingPoint);

		m_vertices.push(startingPoint);
		m_depths.push(depth);
		m_visited.clear();

		m_attributeFetcher.reset();

		m_searchIsStarted = true;

	}else if(!m_vertices.empty()){

		Kmer kmer = m_vertices.top();
		int depth = m_depths.top();

#ifdef DEBUG_LEFT_EXPLORATION
		//cout<<"Stack is not empty"<<endl;
#endif

		if(depth > m_actualMaximumDepth)
			m_actualMaximumDepth = depth;

// too deep
		if(depth == m_maximumAllowedDepth){

#ifdef DEBUG_LEFT_EXPLORATION
			cout<<"Reached maximum"<<endl;
#endif

			m_vertices.pop();
			m_depths.pop();

// working ...
		}else if(!m_attributeFetcher.fetchObjectMetaData(&kmer)){

		}else if((int)m_attributeFetcher.getDepth() > m_maximumDepthForExploration) {

			return true;
		}else{

// need to pop the thing now !
			m_vertices.pop();
			m_depths.pop();

#ifdef DEBUG_LEFT_EXPLORATION
			cout<<"fetchObjectMetaData is done... " << m_attributeFetcher.getParents()->size() << " links "<<endl;
#endif

			m_visited.insert(kmer);
// explore links

			vector<Kmer> * links = NULL;

			if(direction == DIRECTION_PARENTS)
				links = m_attributeFetcher.getParents();
			else if(direction == DIRECTION_CHILDREN)
				links = m_attributeFetcher.getChildren();

#ifdef CONFIG_ASSERT
			assert(links != NULL);
#endif

			for(int i = 0 ; i < (int)links->size() ; i++){

				Kmer parent = links->at(i);

				if(m_visited.count(parent)>0)
					continue;

				m_vertices.push(parent);
				m_depths.push( depth + 1 );
			}


// prepare the system for the next wave.

			m_attributeFetcher.reset();
		}

// the exploration is finished
// and we did not go far.
	}else if(m_actualMaximumDepth < m_maximumAllowedDepth){ 

		m_valid = false;

		return true;
	}else{
		return true;
	}

	return false;
}

// #define DEBUG_LEFT_EXPLORATION

bool AnnihilationWorker::checkDeadEndOnTheLeft(){

	if(!m_startedToCheckDeadEndOnTheLeft){

#ifdef DEBUG_CODE_PATH
		cout<<"Worker " << m_identifier << " STEP_CHECK_DEAD_END_ON_THE_LEFT"<<endl;
#endif
#ifdef DEBUG_LEFT_EXPLORATION
		cout<<"Starting checkDeadEndOnTheLeft"<<endl;
#endif

		m_searchIsStarted = false;
		m_startedToCheckDeadEndOnTheLeft=true;

	}else if(!searchGraphForNiceThings(DIRECTION_PARENTS)){

		// wait a little bit now

	}else if(!m_valid){

		m_done = true;

		return true;

	}else{
		return true;
	}

	return false;
}

bool AnnihilationWorker::checkDeadEndOnTheRight(){

	if(!m_startedToCheckDeadEndOnTheRight){

#ifdef DEBUG_CODE_PATH
		cout<<"Worker " << m_identifier << " STEP_CHECK_DEAD_END_ON_THE_RIGHT"<<endl;
#endif
#ifdef DEBUG_LEFT_EXPLORATION
		cout<<"Starting checkDeadEndOnTheRight"<<endl;
#endif

		m_searchIsStarted = false;
		m_startedToCheckDeadEndOnTheRight = true;

	}else if(!searchGraphForNiceThings(DIRECTION_CHILDREN)){

		// wait a little bit now

	}else if(!m_valid){

		m_done = true;

		return true;
	}else{

#ifdef DEBUG_LEFT_EXPLORATION
		cout << "Next is bubble check"<<endl;
#endif
		return true;

	}

	return false;
}

bool AnnihilationWorker::isDone(){

	return m_done;
}

WorkerHandle AnnihilationWorker::getWorkerIdentifier(){

	return m_identifier;
}

// TODO: skip this if the length is too short.
bool AnnihilationWorker::checkBubblePatterns(){

	if(!m_fetchedFirstParent){

		Kmer startingPoint;
		int index = 0;
		m_seed->at(index, &startingPoint);

		if(!m_attributeFetcher.fetchObjectMetaData(&startingPoint)){

		}else{

#ifdef DEBUG_CODE_PATH
			cout<<"Worker " << m_identifier << " STEP_CHECK_BUBBLE_PATTERNS"<<endl;
#endif
			if(m_attributeFetcher.getParents()->size() != 1){

				return true;
			}else{

				m_parent=m_attributeFetcher.getParents()->at(0);
			}

			m_fetchedFirstParent = true;
			m_fetchedSecondParent = false;
			m_attributeFetcher.reset();
		}

	}else if(!m_fetchedSecondParent){

		if(!m_attributeFetcher.fetchObjectMetaData(&m_parent)){

		}else{
			if(m_attributeFetcher.getParents()->size() != 1){

				return true;
			}else{

				m_grandparent=m_attributeFetcher.getParents()->at(0);
			}

			m_fetchedSecondParent = true;
			m_fetchedGrandparentDirections = false;
			m_annotationFetcher.reset();

#ifdef DEBUG_LEFT_EXPLORATION
			cout << "Next is to fetch directions " <<endl;
#endif
		}
	}else if(!m_fetchedGrandparentDirections){

		if(!m_annotationFetcher.fetchDirections(&m_grandparent)){

			// work a bit here
		}else{
			m_leftDirections = *(m_annotationFetcher.getDirections() );

			m_fetchedGrandparentDirections = true;
			m_fetchedGrandparentReverseDirections = false;
			m_annotationFetcher.reset();
		}
	}else if(!m_fetchedGrandparentReverseDirections){

		Kmer theKmer;
		theKmer = m_grandparent.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

		if(!m_annotationFetcher.fetchDirections(&theKmer)){

			// work a bit here
		}else{
			for(int i=0;i< (int) m_annotationFetcher.getDirections()->size(); i++){
				Direction direction;
				direction.constructor(m_annotationFetcher.getDirections()->at(i).getWave(),
						m_annotationFetcher.getDirections()->at(i).getProgression(),
						true);

				m_leftDirections.push_back(direction);
			}

			m_fetchedGrandparentReverseDirections= true;

			m_fetchedFirstChild = false;
			m_attributeFetcher.reset();
		}

	}else if(!m_fetchedFirstChild){

		Kmer startingPoint;
		int index = m_seed->size()-1;
		m_seed->at(index, &startingPoint);

		if(!m_attributeFetcher.fetchObjectMetaData(&startingPoint)){

		}else{
			if(m_attributeFetcher.getChildren()->size() != 1){

				return true;
			}else{

				m_child = m_attributeFetcher.getChildren()->at(0);
			}

			m_fetchedFirstChild = true;
			m_fetchedSecondChild = false;
			m_attributeFetcher.reset();
		}

	}else if(!m_fetchedSecondChild){

		if(!m_attributeFetcher.fetchObjectMetaData(&m_child)){
		}else{
			if(m_attributeFetcher.getChildren()->size() != 1){

				return true;
			}else{

				m_grandchild = m_attributeFetcher.getChildren()->at(0);
			}

			m_fetchedSecondChild= true;
			m_fetchedGrandchildDirections = false;
			m_annotationFetcher.reset();

#ifdef DEBUG_LEFT_EXPLORATION
			cout << "Next is to fetch directions " <<endl;
#endif
		}
	}else if(!m_fetchedGrandchildDirections){

		if(!m_annotationFetcher.fetchDirections(&m_grandchild)){

		}else{
			m_rightDirections = *(m_annotationFetcher.getDirections() );

			m_fetchedGrandchildDirections= true;

			m_fetchedGrandchildReverseDirections = false;
			m_annotationFetcher.reset();
		}
	}else if(!m_fetchedGrandchildReverseDirections){

		Kmer theKmer;
		theKmer = m_grandchild.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

		if(!m_annotationFetcher.fetchDirections(&theKmer)){

			// work a bit here
		}else{
			for(int i=0;i< (int) m_annotationFetcher.getDirections()->size(); i++){
				Direction direction;
				direction.constructor(m_annotationFetcher.getDirections()->at(i).getWave(),
						m_annotationFetcher.getDirections()->at(i).getProgression(),
						true);

				m_rightDirections.push_back(direction);
			}

			m_fetchedGrandchildReverseDirections = true;
		}
	}else{

		if(m_isPerfectBubble){

#ifdef DEBUG_CODE_PATH
			cout<<"BUBBLE_HIT first=";
#endif

			Kmer startingPoint;
			int index = 0;
			m_seed->at(index, &startingPoint);
			cout << startingPoint.idToWord(m_parameters->getWordSize(), m_parameters->getColorSpaceMode());

#if 0
			cout << " grandparent= ";
			cout << m_grandparent.idToWord(m_parameters->getWordSize(), m_parameters->getColorSpaceMode());
#endif

#ifdef DEBUG_CODE_PATH
			cout<<" LeftPaths: " << m_leftDirections.size();
			cout<<" RightPaths: " << m_rightDirections.size() << endl;
#endif
		}

		map<PathHandle,int> counts;

		for(int i=0; i < (int) m_leftDirections.size() ; i ++ )
			counts[m_leftDirections[i].getWave()]++;

		for(int i=0; i < (int) m_rightDirections.size() ; i ++ )
			counts[m_rightDirections[i].getWave()]++;

		for(map<PathHandle,int>::iterator i = counts.begin() ; i != counts.end() ; i ++){

			// another, longer, seed covers this case.
			if(i->second == 2){

				m_valid = false;
			}
		}

		// this is over.
		return true;
	}

	return false;
}

bool AnnihilationWorker::getBestParent(Kmer*kmer){
	return true;
}

bool AnnihilationWorker::getBestChild(Kmer*kmer){

	return true;
}

bool AnnihilationWorker::getOtherBestChild(Kmer*kmer){
	return true;

}

bool AnnihilationWorker::getOtherBestParent(Kmer*kmer){

	return true;
}

/**
 * RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE
 * RAY_MPI_TAG_ASK_VERTEX_PATH
 */
void AnnihilationWorker::initialize(uint64_t identifier,GraphPath*seed, Parameters * parameters,
	VirtualCommunicator * virtualCommunicator, RingAllocator*outboxAllocator,
	MessageTag RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,
	MessageTag RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE, MessageTag RAY_MPI_TAG_ASK_VERTEX_PATH
	){

	m_identifier = identifier;
	m_done = false;

	m_seed = seed;

	m_virtualCommunicator = virtualCommunicator;
	m_parameters = parameters;

	int stepValue = 0;

	STEP_CHECK_LENGTH = stepValue++;
	STEP_CHECK_DEAD_END_ON_THE_LEFT = stepValue ++;
	STEP_CHECK_DEAD_END_ON_THE_RIGHT = stepValue ++;
	STEP_CHECK_BUBBLE_PATTERNS= stepValue ++;
	STEP_FETCH_FIRST_PARENT = stepValue ++;
	STEP_FETCH_SECOND_PARENT = stepValue ++;
	STEP_DOWNLOAD_ORIGINAL_ANNOTATIONS = stepValue ++;
	STEP_GET_SEED_SEQUENCE_NOW = stepValue ++;

	m_step = STEP_CHECK_DEAD_END_ON_THE_LEFT;

	this->RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT = RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT;

	this->RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE = RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE;
	this->RAY_MPI_TAG_ASK_VERTEX_PATH = RAY_MPI_TAG_ASK_VERTEX_PATH;

	m_rank = m_parameters->getRank();
	m_outboxAllocator = outboxAllocator;

	m_startedToCheckDeadEndOnTheLeft = false;
	m_startedToCheckDeadEndOnTheRight = false;

	m_valid = true;

	DIRECTION_PARENTS = 0;
	DIRECTION_CHILDREN = 1;

	m_fetchedFirstParent = false;
	m_fetchedSecondParent = false;

	m_attributeFetcher.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT);

	m_annotationFetcher.initialize(parameters, virtualCommunicator,
			identifier, outboxAllocator,
			RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,
			RAY_MPI_TAG_ASK_VERTEX_PATH);

	int nucleotides = m_seed->size() + m_parameters->getWordSize() -1;
	int bubbleSize = 2 * m_parameters->getWordSize() - 3;

	m_isPerfectBubble = false;

	if(nucleotides == bubbleSize)
		m_isPerfectBubble = true;

#ifdef DEBUG_ISSUE_136
	if(m_isPerfectBubble)
		cout<<"BUBBLE_ITEM"<<endl;
#endif

/*
 * the maximum depth for dead ends.
 */
	m_maximumDepthForExploration = 256;

/*
 * Maximum search depth for dead ends.
 */
	m_maximumAllowedDepth = m_parameters->getWordSize();
}

bool AnnihilationWorker::isValid(){
	return m_valid;
}
