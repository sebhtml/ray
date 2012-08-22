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

*/

//#define DEBUG_NEIGHBOURHOOD_COMMUNICATION
//#define DEBUG_NEIGHBOURHOOD_PATHS
//#define DEBUG_NEIGHBOUR_LISTING
//#define DEBUG_SIDE
//#define DEBUG_LEFT_PATHS

#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood.h>
#include <sstream>

#ifdef ASSERT
#include <assert.h>
#endif

__CreatePlugin(GenomeNeighbourhood);

 /**/
__CreateMasterModeAdapter(GenomeNeighbourhood,RAY_MASTER_MODE_NEIGHBOURHOOD); /**/
 /**/
__CreateSlaveModeAdapter(GenomeNeighbourhood,RAY_SLAVE_MODE_NEIGHBOURHOOD); /**/
 /**/
__CreateMessageTagAdapter(GenomeNeighbourhood,RAY_MPI_TAG_NEIGHBOURHOOD_DATA); /**/
 /**/


#define FETCH_PARENTS 	0x00345678
#define FETCH_CHILDREN 	0x01810230

void GenomeNeighbourhood::call_RAY_MPI_TAG_NEIGHBOURHOOD_DATA(Message*message){
	
	MessageUnit*incoming=(MessageUnit*)message->getBuffer();
	int position=0;

	/* progressions are on the 'F' strand.
 * if a contig is on the 'R' strand, then the actual position is
 * really length(contig) - position
 *
 * it is sent this way because only master has the length on contigs
 *
 */
	PathHandle leftContig=incoming[position++];
	Strand leftVertexStrand=incoming[position++];
	int leftProgressionInContig=incoming[position++];

	PathHandle rightContig=incoming[position++];
	char rightVertexStrand=incoming[position++];
	int rightProgressionInContig=incoming[position++];

	int gapSizeInKmers=incoming[position++];

	#ifdef ASSERT
	assert(gapSizeInKmers >= 1);
	assert(m_rank==0x00);
	assert(m_contigLengths->count(leftContig)>0);
	assert(m_contigLengths->count(rightContig)>0);
	assert(leftProgressionInContig < m_contigLengths->operator[](leftContig));
	assert(rightProgressionInContig< m_contigLengths->operator[](rightContig));
	assert(leftProgressionInContig>=0);
	assert(rightProgressionInContig>=0);
	assert(leftVertexStrand=='F' || leftVertexStrand=='R');
	assert(rightVertexStrand=='F' || rightVertexStrand == 'R');
	#endif

/*
	if(rightVertexStrand=='R'){
		rightProgressionInContig=m_contigLengths->operator[](rightContig)-rightProgressionInContig;
	}

	// get the position on the actual strand
	if(leftVertexStrand=='R'){
		leftProgressionInContig=m_contigLengths->operator[](leftContig)-leftProgressionInContig;
	}
*/

	NeighbourPair pair(leftContig,leftVertexStrand,leftProgressionInContig,
				rightContig,rightVertexStrand,rightProgressionInContig,
				gapSizeInKmers);

	m_finalList.push_back(pair);

	int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_NEIGHBOURHOOD_DATA);

	MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(period*sizeof(MessageUnit));
	Message aMessage(buffer,period,message->getSource(),RAY_MPI_TAG_NEIGHBOURHOOD_DATA_REPLY,
		m_parameters->getRank());

	m_core->getOutbox()->push_back(aMessage);
}

void GenomeNeighbourhood::fetchPaths(int mode){

/** stop the search when something is found **/
/** this will speed things up, but will report less hits because of repeated k-mers **/

	bool stopWhenSomethingIsFound=true;

	Kmer currentKmer=m_stackOfVertices.top();
	int depth=m_stackOfDepths.top();

	Kmer kmer=currentKmer;

	if(m_reverseStrand){
		kmer=kmer.complementVertex(m_parameters->getWordSize(),m_parameters->getColorSpaceMode());
	}

	if(!m_numberOfPathsRequested){

		#ifdef DEBUG_SIDE
		if(mode==FETCH_PARENTS){
			cout<<"[DEBUG_SIDE] FETCH_PARENTS RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE"<<endl;
		}else if(mode==FETCH_CHILDREN){
			cout<<"[DEBUG_SIDE] FETCH_CHILDREN RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE"<<endl;
		}
		#endif


		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Sending RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE"<<endl;
		#endif

		// send a message to request the links of the current vertex
		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int bufferPosition=0;
		kmer.pack(buffer,&bufferPosition);
	
		Rank destination=m_parameters->_vertexRank(&kmer);

		Message aMessage(buffer,m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE),
			destination,RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE,m_rank);

		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);

		m_numberOfPathsReceived=false;

		// keep up the good work for now
		m_linksRequested=false;
		m_linksReceived=false;

		m_numberOfPathsRequested=true;

	}else if(!m_numberOfPathsReceived && m_virtualCommunicator->isMessageProcessed(m_workerId)){

		vector<MessageUnit> elements;
		m_virtualCommunicator->getMessageResponseElements(m_workerId,&elements);

		m_paths=elements[0];

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Received reply for RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE"<<endl;
		#endif

		#ifdef DEBUG_NEIGHBOURHOOD_PATHS
		cout<<"Number of paths: "<<m_paths<<" depth: "<<depth<<" useReverse= "<<m_reverseStrand<<endl;
		#endif

		m_numberOfPathsReceived=true;
		m_pathIndex=0;

		m_requestedPath=false;
		m_receivedPath=false;

	}else if(m_numberOfPathsReceived && m_pathIndex == m_paths){

		#ifdef DEBUG_NEIGHBOURHOOD_PATHS
		cout<<"Index completed."<<endl;
		#endif

		m_fetchedPaths=true;

	
		#ifdef DEBUG_SIDE
		if(mode==FETCH_PARENTS){
			cout<<"[DEBUG_SIDE] FETCH_PARENTS finished fetching n="<<m_paths<<endl;
		}else if(mode==FETCH_CHILDREN){
			cout<<"[DEBUG_SIDE] FETCH_CHILDREN finished fetching n="<<m_paths<<endl;
		}
		#endif


	}else if(m_numberOfPathsReceived && !m_requestedPath){

		Rank destination=kmer.vertexRank(m_parameters->getSize(),
			m_parameters->getWordSize(),m_parameters->getColorSpaceMode());

		int elementsPerQuery=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_ASK_VERTEX_PATH);

		MessageUnit*message=(MessageUnit*)m_outboxAllocator->allocate(elementsPerQuery);

		int outputPosition=0;
		kmer.pack(message,&outputPosition);
		message[outputPosition++]=m_pathIndex;

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Sending RAY_MPI_TAG_ASK_VERTEX_PATH"<<endl;
		#endif

		Message aMessage(message,elementsPerQuery,destination,
			RAY_MPI_TAG_ASK_VERTEX_PATH,m_parameters->getRank());

		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);

		m_requestedPath=true;
		m_receivedPath=false;

	}else if(m_numberOfPathsReceived && !m_receivedPath && m_virtualCommunicator->isMessageProcessed(m_workerId)){
		vector<MessageUnit> response;
		m_virtualCommunicator->getMessageResponseElements(m_workerId,&response);

		int bufferPosition=0;

		/* skip the k-mer because we don't need it */
		bufferPosition+=KMER_U64_ARRAY_SIZE;
		PathHandle pathIdentifier=response[bufferPosition++];
		int progression=response[bufferPosition++];

		
		int minimumDepth=1;

		#ifdef DEBUG_NEIGHBOURHOOD_PATHS
		cout<<"Received path mode=";
		if(mode==FETCH_PARENTS){
			cout<<"FETCH_PARENTS";
		}else{
			cout<<"FETCH_CHILDREN";
		}
		cout<<" path is "<<pathIdentifier<<endl;
		#endif

		/** add the path **/

		if(pathIdentifier != (*m_contigNames)[m_contigIndex]
			//&& m_foundContigs.count(pathIdentifier)==0
			&& depth >= minimumDepth){

			char strand='F';
			if(m_reverseStrand){
				strand='R';
			}

			Neighbour friendlyNeighbour(strand,depth,pathIdentifier,progression);
		
			if(stopWhenSomethingIsFound){
				m_foundPathsForThisVertex=true;
			}

			if(mode==FETCH_PARENTS){
				m_leftNeighbours.push_back(friendlyNeighbour);

			}else if(mode == FETCH_CHILDREN){

				m_rightNeighbours.push_back(friendlyNeighbour);
			}

			m_foundContigs.insert(pathIdentifier);
		}

		m_pathIndex++;

		m_receivedPath=true;

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Received reply for RAY_MPI_TAG_ASK_VERTEX_PATH"<<endl;
		#endif

		m_requestedPath=false;
		m_receivedPath=false;
	}
}

void GenomeNeighbourhood::processLinks(int mode){

	#ifdef ASSERT
	assert(!m_stackOfVertices.empty());
	assert(m_stackOfVertices.size() == m_stackOfDepths.size());
	#endif

	Kmer currentKmer=m_stackOfVertices.top();
	int depth=m_stackOfDepths.top();

	if(!m_directDone){
		if(!m_fetchedPaths){

			fetchPaths(mode);
		}else{
		
			#ifdef DEBUG_SIDE
			if(mode==FETCH_PARENTS){
				cout<<"[DEBUG_SIDE] Fetched direct paths for mode FETCH_PARENTS, n="<<m_paths<<endl;
			}else if(mode==FETCH_CHILDREN){
				cout<<"[DEBUG_SIDE] Fetched direct paths for mode FETCH_CHILDREN, n="<<m_paths<<endl;
			}
			#endif


			#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
			cout<<"m_directDone= True"<<endl;
			#endif

			m_directDone=true;
			m_fetchedPaths=false;
			m_numberOfPathsRequested=false;
			m_numberOfPathsReceived=false;

			m_reverseDone=false;

			m_reverseStrand=true;

		}
/**/
	}else if(!m_reverseDone){
		if(!m_fetchedPaths){

			fetchPaths(mode);

		}else{
			
			#ifdef DEBUG_SIDE
			if(mode==FETCH_PARENTS){
				cout<<"[DEBUG_SIDE] Fetched reverse paths for mode FETCH_PARENTS, n="<<m_paths<<endl;
			}else if(mode==FETCH_CHILDREN){
				cout<<"[DEBUG_SIDE] Fetched reverse paths for mode FETCH_CHILDREN, n="<<m_paths<<endl;
			}
			#endif

		
			#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
			cout<<"m_reverseDone= True"<<endl;
			#endif
	
			m_reverseDone=true;
		}
/**/

	}else if(m_fetchedPaths && !m_linksRequested){

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Sending message RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT to "<<endl;
		#endif

		// send a message to request the links of the current vertex
		MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int bufferPosition=0;
		currentKmer.pack(buffer,&bufferPosition);
	
		Rank destination=m_parameters->_vertexRank(&currentKmer);

		Message aMessage(buffer,m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT),
			destination,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,m_rank);

		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);

		m_linksRequested=true;
		m_linksReceived=false;

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Message sent, RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT, will wait for a reply "<<endl;
		#endif

	}else if(m_fetchedPaths &&
		!m_linksReceived && m_virtualCommunicator->isMessageProcessed(m_workerId)){

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Message received, RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT"<<endl;
		#endif

		vector<MessageUnit> elements;
		m_virtualCommunicator->getMessageResponseElements(m_workerId,&elements);

		#ifdef ASSERT
		assert((int)elements.size()>=2);
		#endif

		uint8_t edges=elements[0];

		#ifdef ASSERT
		int coverage=elements[1];
		assert(coverage>0);
		#endif

		vector<Kmer> parents=currentKmer._getIngoingEdges(edges,m_parameters->getWordSize());
		vector<Kmer> children=currentKmer._getOutgoingEdges(edges,m_parameters->getWordSize());

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"information: "<<parents.size()<<" parents, "<<children.size()<<" children"<<endl;
		#endif

		int nextDepth=depth+1;

		// remove the current vertex from the stack
		m_stackOfVertices.pop();
		m_stackOfDepths.pop();

		m_visited.insert(currentKmer);

		// add new links
		
		vector<Kmer>*links=&parents;

		#ifdef ASSERT
		assert(mode==FETCH_CHILDREN || mode==FETCH_PARENTS);
		#endif

		if(mode==FETCH_CHILDREN){
			links=&children;

		}else if(mode==FETCH_PARENTS){
			links=&parents;
		}

		#ifdef ASSERT
		assert(m_stackOfDepths.size()==m_stackOfVertices.size());
		#endif

		/** we don't continue if we found something interesting... **/

		for(int i=0;i<(int)links->size();i++){

			#ifdef ASSERT
			assert(i<(int) links->size());
			#endif

			Kmer newKmer=links->at(i);

			if(nextDepth<= m_maximumDepth && m_visited.count(newKmer)==0

				&& !m_foundPathsForThisVertex){ /* avoid exploring too much when something is already on the table **/
		
				m_stackOfVertices.push(newKmer);
				m_stackOfDepths.push(nextDepth);
			}
		}

		m_linksReceived=true;

	}else if(m_fetchedPaths && m_linksRequested && m_linksReceived){
		
		// restart the adventure

		resetKmerStates();

	}
}

void GenomeNeighbourhood::processSide(int mode){

	#ifdef ASSERT
	assert(mode==FETCH_CHILDREN || mode==FETCH_PARENTS);
	#endif

	if(!m_startedSide){

		#ifdef ASSERT
		assert(m_contigIndex < (int)m_contigs->size());
		#endif

		int contigLength=m_contigs->at(m_contigIndex).size();

		#ifdef ASSERT
		assert(contigLength>=1);
		#endif

		Kmer kmer;

		if(mode==FETCH_CHILDREN){
			kmer=m_contigs->at(m_contigIndex).at(contigLength-1);

		}else if(mode==FETCH_PARENTS){

			kmer=m_contigs->at(m_contigIndex).at(0);
		}

		createStacks(kmer);

		resetKmerStates();

		m_visited.clear();
		m_foundContigs.clear();

/* the maximum depth
 * values are 1024, 2048 or 4096 */
		m_maximumDepth=1024;

		m_startedSide=true;

		#ifdef DEBUG_SIDE
		if(mode==FETCH_PARENTS){
			cout<<"Starting mode FETCH_PARENTS"<<endl;
		}else if(mode==FETCH_CHILDREN){
			cout<<"Starting mode FETCH_CHILDREN"<<endl;
		}
		#endif

	}else if(!m_stackOfVertices.empty()){
		
		processLinks(mode);

	}else{
		m_doneSide=true;
	}

}

void GenomeNeighbourhood::resetKmerStates(){
	m_numberOfPathsRequested=false;
	m_numberOfPathsReceived=false;
	m_fetchedPaths=false;
	m_directDone=false;
	m_reverseDone=false;

	m_reverseStrand=false;

	m_foundPathsForThisVertex=false;
}

void GenomeNeighbourhood::createStacks(Kmer a){
	while(!m_stackOfVertices.empty()){
		m_stackOfVertices.pop();
	}
	while(!m_stackOfDepths.empty()){
		m_stackOfDepths.pop();
	}

	m_stackOfVertices.push(a);
	m_stackOfDepths.push(0);
}

void GenomeNeighbourhood::processFinalList(){
/* we have a list of pairs
 * there are duplicates
 * and the list is still unfiltered.
 *
 * the first step is to select the best entry for
 * any ordered pair where (a,b) and (b,a) are
 * different
 */

/* now, select one entry for each */


/* now, select one entry for each */

/* cases:
 *
 *

VALID

1.
            *            *
 ----------->            ------------>

2.
            *            *
 ----------->            <------------

3.
            *            *
<------------            ------------->

4.
            *            *
<------------            <-------------

all other cases are invalid.

 *
 */

	ostringstream relations;

	relations<<m_parameters->getPrefix()<<"/NeighbourhoodRelations.txt";

	string file=relations.str();

	ofstream f(file.c_str());
	
	ostringstream operationBuffer;

	operationBuffer<<"#LeftContigPath	LengthInKmers	DNAStrand	PositionOnStrand";
	operationBuffer<<"	RightContigPath	LengthInKmers	DNAStrand	PositionOnStrand";
	operationBuffer<<"	DistanceInKmers	QualityControlStatus"<<endl;

	for(int i=0;i<(int)m_finalList.size();i++){
		PathHandle contig1=m_finalList[i].getContig1();
		PathHandle contig2=m_finalList[i].getContig2();
		int length1=m_contigLengths->operator[](contig1);
		int length2=m_contigLengths->operator[](contig2);
		Strand strand1=m_finalList[i].getStrand1();
		Strand strand2=m_finalList[i].getStrand2();
		int progression1=m_finalList[i].getProgression1();
		int progression2=m_finalList[i].getProgression2();

		int depth=m_finalList[i].getDepth();

		bool valid=true;

		int windows=(0x00000001 << 0x00000002);

		int width1=length1/windows;
		int width2=length2/windows;

		/*The pair is considered valid unless the similarity is after the first 1/4 and before the last 3/4*/

		if((progression1>width1 && progression1<(length1-width1-1)) || (progression2>width2 && progression2<(length2-width2-1))){
			valid=false;
		}


		operationBuffer<<"contig-"<<contig1<<"	"<<length1<<"	"<<strand1<<"	"<<progression1<<"";
		operationBuffer<<"	contig-"<<contig2<<"	"<<length2<<"	"<<strand2<<"	"<<progression2<<"";
		operationBuffer<<"	"<<depth<<"	";

		if(valid){
			operationBuffer<<"PASS";
		}else{
			operationBuffer<<"FAIL";
		}
		operationBuffer<<endl;

		flushFileOperationBuffer(false,&operationBuffer,&f,CONFIG_FILE_IO_BUFFER_SIZE);
	}

	flushFileOperationBuffer(true,&operationBuffer,&f,CONFIG_FILE_IO_BUFFER_SIZE);

	f.close();
}

void GenomeNeighbourhood::call_RAY_MASTER_MODE_NEIGHBOURHOOD(){

	if(!m_started){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getMessagesHandler()->getRank());

		m_started=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		cout<<"[GenomeNeighbourhood] in final list: "<<m_finalList.size()<<endl;

		processFinalList();

		m_finalList.clear();
		m_contigNames->clear();
		m_contigLengths->clear();
		m_contigs->clear();

		m_timePrinter->printElapsedTime("Computing neighbourhoods");

		m_core->getSwitchMan()->closeMasterMode();
	}
}

void GenomeNeighbourhood::selectHits(){
	/** we have all the neighbours, unfiltered... **/
	/* process neighbours */

	map<PathHandle,int> leftMinimums;
	map<PathHandle,int> leftMaximums;
	map<PathHandle,int> rightMinimums;
	map<PathHandle,int> rightMaximums;

	for(int i=0;i<(int)m_leftNeighbours.size();i++){
		PathHandle contig=m_leftNeighbours[i].getContig();
		int progression=m_leftNeighbours[i].getProgression();

		if(leftMinimums.count(contig)==0 || progression < leftMinimums[contig]){
			leftMinimums[contig]=progression;
		}

		if(leftMaximums.count(contig)==0 || progression > leftMaximums[contig]){
			leftMaximums[contig]=progression;
		}

	}

	for(int i=0;i<(int)m_rightNeighbours.size();i++){
		PathHandle contig=m_rightNeighbours[i].getContig();
		int progression=m_rightNeighbours[i].getProgression();

		if(rightMinimums.count(contig)==0 || progression < rightMinimums[contig]){
			rightMinimums[contig]=progression;
		}

		if(rightMaximums.count(contig)==0 || progression > rightMaximums[contig]){
			rightMaximums[contig]=progression;
		}
	}

	vector<Neighbour> leftNeighbours;

	for(int i=0;i<(int)m_leftNeighbours.size();i++){
		PathHandle contig=m_leftNeighbours[i].getContig();
		int progression=m_leftNeighbours[i].getProgression();

		if(progression!=leftMinimums[contig] && progression != leftMaximums[contig]){
			continue;
		}

		leftNeighbours.push_back(m_leftNeighbours[i]);

		#ifdef DEBUG_NEIGHBOUR_LISTING
		cout<<"[GenomeNeighbourhood] ITEM LEFT ";

		cout<<"contig-"<<m_leftNeighbours[i].getContig()<<" "<<m_leftNeighbours[i].getStrand();
		cout<<" "<<m_leftNeighbours[i].getProgression()<<"(TODO) ";

		cout<<" contig-"<<contigName<<" "<<contigStrand<<" 0 ";

		cout<<m_leftNeighbours[i].getDepth()<<endl;
		
		#endif
	}

	m_leftNeighbours=leftNeighbours;

	vector<Neighbour> rightNeighbours;

	for(int i=0;i<(int)m_rightNeighbours.size();i++){

		PathHandle contig=m_rightNeighbours[i].getContig();
		int progression=m_rightNeighbours[i].getProgression();

		if(progression!=rightMinimums[contig] && progression != rightMaximums[contig]){
			continue;
		}

		rightNeighbours.push_back(m_rightNeighbours[i]);

		#ifdef DEBUG_NEIGHBOUR_LISTING
		cout<<"[GenomeNeighbourhood] ITEM RIGHT ";

		cout<<"contig-"<<contigName<<" "<<contigStrand<<" "<<contigLength-1;

		cout<<" contig-"<<m_rightNeighbours[i].getContig()<<" "<<m_rightNeighbours[i].getStrand();
		cout<<" "<<m_rightNeighbours[i].getProgression()<<"(TODO) ";

		cout<<m_rightNeighbours[i].getDepth()<<endl;
		#endif
	}

	m_rightNeighbours=rightNeighbours;

}

/**
 * for each contig owned by the current compute core,
 * search on its left and on its right in the distributed de
 * Bruijn graph.
 *
 * send items to master.
 *
 * each item is (leftContig	strand	rightContig	strand	verticesInGap)
 *
 * to do so, do a depth first search with a maximum depth
 *
 *
 * message used and what is needed:
 *
 *    - get the edges of a vertex
 *    RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT
 *      period: registered via RayPlatform, fetch it for there
 *      input: a k-mer
 *      output: edges (1 element), coverage (1 element)
 *      multiplexing: supported
 *
 *
 * used tags for paths: 
 *
 *	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE
 *	RAY_MPI_TAG_ASK_VERTEX_PATH
 *	RAY_MPI_TAG_GET_PATH_LENGTH
 *
 *    - get the path length for a path
 *    RAY_MPI_TAG_GET_PATH_LENGTH
 *      period: 1
 *      input: path unique identifier (usually the the contig name)
 *      output: the length of the path, measured in k-mers
 *
 *
 * prototype 1: don't use Message Multiplexing, because the thing may be fast without it
 *              like scaffolding.
 */
void GenomeNeighbourhood::call_RAY_SLAVE_MODE_NEIGHBOURHOOD(){

	if(!m_pluginIsEnabled){

		cout<<"Rank "<<m_rank<<": the CorePlugin GenomeNeighbourhood is disabled..."<<endl;

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getMessagesHandler()->getRank());

		return; /* . */
	}

	/* force flush everything ! */
	m_virtualCommunicator->forceFlush();
	m_virtualCommunicator->processInbox(&m_activeWorkers);
	m_activeWorkers.clear();


	if(!m_slaveStarted){

		m_contigIndex=0;

		m_doneLeftSide=false;
		m_startedLeft=false;
		m_doneRightSide=false;
		m_startedRight=false;
	
		m_slaveStarted=true;

		m_virtualCommunicator->resetCounters();

	}else if(m_contigIndex<(int)m_contigs->size()){ /* there is still work to do */

/*
		PathHandle contigName=m_contigNames->at(m_contigIndex);
		Strand contigStrand='F';
		int contigLength=m_contigs->at(m_contigIndex).size();
*/

		// left side
		if(!m_doneLeftSide){
	
			if(!m_startedLeft){

				cout<<"Rank "<<m_rank<<" is fetching contig path neighbours ["<<m_contigIndex<<"/"<<m_contigs->size()<<"]"<<endl;

				m_startedLeft=true;
				m_startedSide=false;
				m_doneSide=false;


				m_leftNeighbours.clear();
				m_rightNeighbours.clear();

			}else if(!m_doneSide){
				processSide(FETCH_PARENTS);
			}else{
				m_doneLeftSide=true;
			}

		// right side
		}else if(!m_doneRightSide){
		
			if(!m_startedRight){
				m_startedRight=true;
				m_startedSide=false;
				m_doneSide=false;
			}else if(!m_doneSide){

				processSide(FETCH_CHILDREN);
			}else{
				m_doneRightSide=true;
				m_selectedHits=false;
			}


		}else if(!m_selectedHits){

			selectHits();

			m_selectedHits=true;
			m_sentLeftNeighbours=false;
			m_neighbourIndex=0;
			m_sentEntry=false;
			m_receivedReply=false;

		}else if(!m_sentLeftNeighbours){

			sendLeftNeighbours();

		}else if(!m_sentRightNeighbours){

			sendRightNeighbours();

		}else{

			/* continue the work */
			m_contigIndex++;

			m_doneLeftSide=false;
			m_startedLeft=false;
			m_doneRightSide=false;
			m_startedRight=false;
		}
	}else{

		cout<<"Rank "<<m_rank<<" is fetching contig path neighbours ["<<m_contigIndex<<"/"<<m_contigs->size()<<"]"<<endl;

		#ifdef ASSERT
		assert(m_contigIndex == (int)m_contigs->size());
		#endif

		m_virtualCommunicator->printStatistics();

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getMessagesHandler()->getRank());
	}
}

void GenomeNeighbourhood::sendRightNeighbours(){

	if(m_neighbourIndex < (int)m_rightNeighbours.size()){

		if(!m_sentEntry){
			m_sentEntry=true;

			// send a message to request the links of the current vertex
			MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(Kmer));

			Rank destination=0x0;
			int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_NEIGHBOURHOOD_DATA);

			#ifdef ASSERT
			assert(period > 0);
			#endif

			int outputPosition=0;

			buffer[outputPosition++]=m_contigNames->at(m_contigIndex);
			buffer[outputPosition++]='F';
			buffer[outputPosition++]=m_contigs->at(m_contigIndex).size()-1;

			buffer[outputPosition++]=m_rightNeighbours[m_neighbourIndex].getContig();
			buffer[outputPosition++]=m_rightNeighbours[m_neighbourIndex].getStrand();
			buffer[outputPosition++]=m_rightNeighbours[m_neighbourIndex].getProgression();

			buffer[outputPosition++]=m_rightNeighbours[m_neighbourIndex].getDepth();

			Message aMessage(buffer,period,
				destination,RAY_MPI_TAG_NEIGHBOURHOOD_DATA,m_rank);

			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);

			m_receivedReply=false;

		}else if(!m_receivedReply && m_virtualCommunicator->isMessageProcessed(m_workerId)){

			vector<MessageUnit> elements;
			m_virtualCommunicator->getMessageResponseElements(m_workerId,&elements);


			m_receivedReply=true;
			m_sentEntry=false;

			m_neighbourIndex++;
		}


	}else{

		#ifdef DEBUG_LEFT_PATHS
		cout<<"[DEBUG_LEFT_PATHS] processed left paths: "<<m_leftNeighbours.size()<<endl;
		#endif


		m_sentRightNeighbours=true;

	}
}

void GenomeNeighbourhood::sendLeftNeighbours(){

	if(m_neighbourIndex< (int)m_leftNeighbours.size()){
		if(!m_sentEntry){
			m_sentEntry=true;

			// send a message to request the links of the current vertex
			MessageUnit*buffer=(MessageUnit*)m_outboxAllocator->allocate(1*sizeof(Kmer));

			Rank destination=0x0;
			int period=m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_NEIGHBOURHOOD_DATA);

			#ifdef ASSERT
			assert(period > 0);
			#endif

			int outputPosition=0;

			buffer[outputPosition++]=m_leftNeighbours[m_neighbourIndex].getContig();
			buffer[outputPosition++]=m_leftNeighbours[m_neighbourIndex].getStrand();
			buffer[outputPosition++]=m_leftNeighbours[m_neighbourIndex].getProgression();

			buffer[outputPosition++]=m_contigNames->at(m_contigIndex);
			buffer[outputPosition++]='F';
			buffer[outputPosition++]=0;

			buffer[outputPosition++]=m_leftNeighbours[m_neighbourIndex].getDepth();

			Message aMessage(buffer,period,
				destination,RAY_MPI_TAG_NEIGHBOURHOOD_DATA,m_rank);

			m_virtualCommunicator->pushMessage(m_workerId,&aMessage);

			m_receivedReply=false;

		}else if(!m_receivedReply && m_virtualCommunicator->isMessageProcessed(m_workerId)){

			vector<MessageUnit> elements;
			m_virtualCommunicator->getMessageResponseElements(m_workerId,&elements);


			m_receivedReply=true;
			m_sentEntry=false;

			m_neighbourIndex++;
		}
	}else{

		#ifdef DEBUG_LEFT_PATHS
		cout<<"[DEBUG_LEFT_PATHS] processed left paths: "<<m_leftNeighbours.size()<<endl;
		#endif

		m_sentLeftNeighbours=true;
		m_sentRightNeighbours=false;
		m_sentEntry=false;
		m_receivedReply=false;

		m_neighbourIndex=0;
	}

}

/**
 * register the plugin
 * */
void GenomeNeighbourhood::registerPlugin(ComputeCore*core){

	m_plugin=core->allocatePluginHandle();

	core->setPluginName(m_plugin,"GenomeNeighbourhood");
	core->setPluginDescription(m_plugin,"Get a sophisticated bird's-eye view of a sample's DNA");
	core->setPluginAuthors(m_plugin,"Sébastien Boisvert");
	core->setPluginLicense(m_plugin,"GNU General Public License version 3 (GPLv3)");

	// register handles
	
	RAY_MASTER_MODE_NEIGHBOURHOOD=core->allocateMasterModeHandle(m_plugin);
	core->setMasterModeSymbol(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,"RAY_MASTER_MODE_NEIGHBOURHOOD");
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,__GetAdapter(GenomeNeighbourhood,RAY_MASTER_MODE_NEIGHBOURHOOD));

	RAY_SLAVE_MODE_NEIGHBOURHOOD=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_NEIGHBOURHOOD,"RAY_SLAVE_MODE_NEIGHBOURHOOD");
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_NEIGHBOURHOOD,__GetAdapter(GenomeNeighbourhood,RAY_SLAVE_MODE_NEIGHBOURHOOD));

	RAY_MPI_TAG_NEIGHBOURHOOD_DATA=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD_DATA,"RAY_MPI_TAG_NEIGHBOURHOOD_DATA");
	core->setMessageTagObjectHandler(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD_DATA,__GetAdapter(GenomeNeighbourhood,RAY_MPI_TAG_NEIGHBOURHOOD_DATA));

	RAY_MPI_TAG_NEIGHBOURHOOD=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD,"RAY_MPI_TAG_NEIGHBOURHOOD");

	RAY_MPI_TAG_NEIGHBOURHOOD_DATA_REPLY=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD_DATA_REPLY,"RAY_MPI_TAG_NEIGHBOURHOOD_DATA_REPLY");

	core->setMessageTagReplyMessageTag(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD_DATA,RAY_MPI_TAG_NEIGHBOURHOOD_DATA_REPLY);

	/* 
 * 	contig1, strand1, position1
 * 	contig2, strand2, position2
 *      gap size
 */
	core->setMessageTagSize(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD_DATA,7);
}

/**
 * resolve symbols
 */
void GenomeNeighbourhood::resolveSymbols(ComputeCore*core){

	RAY_MASTER_MODE_KILL_RANKS=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_KILL_RANKS");
	RAY_MASTER_MODE_NEIGHBOURHOOD=core->getMasterModeFromSymbol(m_plugin,"RAY_MASTER_MODE_NEIGHBOURHOOD");
	RAY_SLAVE_MODE_NEIGHBOURHOOD=core->getSlaveModeFromSymbol(m_plugin,"RAY_SLAVE_MODE_NEIGHBOURHOOD");

	RAY_MPI_TAG_NEIGHBOURHOOD=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_NEIGHBOURHOOD");
	RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT");
	RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATHS_SIZE");
	RAY_MPI_TAG_ASK_VERTEX_PATH=core->getMessageTagFromSymbol(m_plugin,"RAY_MPI_TAG_ASK_VERTEX_PATH");

/* configure workflow */

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,RAY_MPI_TAG_NEIGHBOURHOOD);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD,RAY_SLAVE_MODE_NEIGHBOURHOOD);

	/* this is done here because we need symbols */

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,RAY_MASTER_MODE_KILL_RANKS);

	// fetch parallel shared objects
	m_timePrinter=(TimePrinter*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Timer.ray");
	m_contigs=(vector<vector<Kmer> >*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/ContigPaths.ray");
	m_contigNames=(vector<PathHandle>*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/ContigNames.ray");
	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");
	m_contigLengths=(map<PathHandle,int>*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/ContigLengths.ray");


	m_virtualCommunicator=core->getVirtualCommunicator();

	m_core=core;
	m_started=false;

	m_rank=core->getMessagesHandler()->getRank();
	m_outboxAllocator=core->getOutboxAllocator();
	m_workerId=0;

	m_slaveStarted=false;

	/* this plugin is disabled because it is not ready yet */
	m_pluginIsEnabled=false;

	if(m_parameters->hasOption("-find-neighbourhoods"))
		m_pluginIsEnabled=true;

	if(m_parameters->hasOption("-enable-neighbourhoods"))
		m_pluginIsEnabled=true;

	__BindPlugin(GenomeNeighbourhood);
}

