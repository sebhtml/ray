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

\file GenomeNeighbourhood.h
\author Sébastien Boisvert
*/

//#define DEBUG_NEIGHBOURHOOD_COMMUNICATION

#include <plugin_GenomeNeighbourhood/GenomeNeighbourhood.h>

#ifdef ASSERT
#include <assert.h>
#endif

#define FETCH_PARENTS 0
#define FETCH_CHILDREN 1

void GenomeNeighbourhood::processLinks(int mode){

	#ifdef ASSERT
	assert(!m_stackOfVertices.empty());
	assert(m_stackOfVertices.size() == m_stackOfDepths.size());
	#endif

	Kmer currentKmer=m_stackOfVertices.top();
	int depth=m_stackOfDepths.top();

	if(!m_linksRequested){

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Sending message RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT to "<<endl;
		#endif


		// send a message to request the links of the current vertex
		uint64_t*buffer=(uint64_t*)m_outboxAllocator->allocate(1*sizeof(Kmer));
		int bufferPosition=0;
		currentKmer.pack(buffer,&bufferPosition);
	
		Rank destination=m_parameters->_vertexRank(&currentKmer);

		Message aMessage(buffer,m_virtualCommunicator->getElementsPerQuery(RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT),
			destination,RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT,m_rank);

		m_virtualCommunicator->pushMessage(m_workerId,&aMessage);

		m_linksRequested=true;
		m_linksReceived=false;
		m_visited.insert(currentKmer);

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Message sent, RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT, will wait for a reply "<<endl;
		#endif


	}else if(!m_linksReceived && m_virtualCommunicator->isMessageProcessed(m_workerId)){

		#ifdef DEBUG_NEIGHBOURHOOD_COMMUNICATION
		cout<<"Message received, RAY_MPI_TAG_GET_VERTEX_EDGES_COMPACT"<<endl;
		#endif


		vector<uint64_t> elements;
		m_virtualCommunicator->getMessageResponseElements(m_workerId,&elements);

		#ifdef ASSERT
		assert((int)elements.size()>=2);
		#endif

		uint8_t edges=elements[0];
		int coverage=elements[1];

		#ifdef ASSERT
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

		for(int i=0;i<(int)links->size();i++){

			#ifdef ASSERT
			assert(i<(int) links->size());
			#endif

			Kmer newKmer=links->at(i);

			if(nextDepth<= m_maximumDepth && m_visited.count(newKmer)==0){
		
				m_stackOfVertices.push(newKmer);
				m_stackOfDepths.push(nextDepth);
			}
		}

		m_linksReceived=true;

	}else if(m_linksRequested && m_linksReceived){

		// keep up the good work for now
		m_linksRequested=false;
		m_linksReceived=false;
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

		m_linksRequested=false;
		m_visited.clear();
		m_maximumDepth=1024;

		m_startedSide=true;

	}else if(!m_stackOfVertices.empty()){
		
		processLinks(mode);

	}else{
		m_doneSide=true;
	}

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

void GenomeNeighbourhood::call_RAY_MASTER_MODE_NEIGHBOURHOOD(){

	if(!m_started){
		m_core->getSwitchMan()->openMasterMode(m_core->getOutbox(),m_core->getMessagesHandler()->getRank());

		m_started=true;

	}else if(m_core->getSwitchMan()->allRanksAreReady()){

		m_timePrinter->printElapsedTime("Computing neighbourhoods");

		m_core->getSwitchMan()->closeMasterMode();
	}
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

	}else if(m_contigIndex<(int)m_contigs->size()){ /* there is still work to do */

		// left side
		if(!m_doneLeftSide){
	
			if(!m_startedLeft){
				m_startedLeft=true;
				m_startedSide=false;
				m_doneSide=false;
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
			}


		}else{
			m_contigIndex++;

			m_doneLeftSide=false;
			m_startedLeft=false;
			m_doneRightSide=false;
			m_startedRight=false;
		}
	}else{

		#ifdef ASSERT
		assert(m_contigIndex == (int)m_contigs->size());
		#endif

		m_core->getSwitchMan()->closeSlaveModeLocally(m_core->getOutbox(),m_core->getMessagesHandler()->getRank());
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
	m_adapter_RAY_MASTER_MODE_NEIGHBOURHOOD.setObject(this);
	core->setMasterModeObjectHandler(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,&m_adapter_RAY_MASTER_MODE_NEIGHBOURHOOD);

	RAY_SLAVE_MODE_NEIGHBOURHOOD=core->allocateSlaveModeHandle(m_plugin);
	core->setSlaveModeSymbol(m_plugin,RAY_SLAVE_MODE_NEIGHBOURHOOD,"RAY_SLAVE_MODE_NEIGHBOURHOOD");
	m_adapter_RAY_SLAVE_MODE_NEIGHBOURHOOD.setObject(this);
	core->setSlaveModeObjectHandler(m_plugin,RAY_SLAVE_MODE_NEIGHBOURHOOD,&m_adapter_RAY_SLAVE_MODE_NEIGHBOURHOOD);

	RAY_MPI_TAG_NEIGHBOURHOOD=core->allocateMessageTagHandle(m_plugin);
	core->setMessageTagSymbol(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD,"RAY_MPI_TAG_NEIGHBOURHOOD");
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

	core->setMasterModeToMessageTagSwitch(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,RAY_MPI_TAG_NEIGHBOURHOOD);
	core->setMessageTagToSlaveModeSwitch(m_plugin,RAY_MPI_TAG_NEIGHBOURHOOD,RAY_SLAVE_MODE_NEIGHBOURHOOD);

	/* this is done here because we need symbols */

	core->setMasterModeNextMasterMode(m_plugin,RAY_MASTER_MODE_NEIGHBOURHOOD,RAY_MASTER_MODE_KILL_RANKS);

	// fetch parallel shared objects
	m_timePrinter=(TimePrinter*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Timer.ray");
	m_contigs=(vector<vector<Kmer> >*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/ContigPaths.ray");
	m_contigNames=(vector<uint64_t>*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/ContigNames.ray");
	m_parameters=(Parameters*)core->getObjectFromSymbol(m_plugin,"/RayAssembler/ObjectStore/Parameters.ray");

	m_virtualCommunicator=core->getVirtualCommunicator();

	m_core=core;
	m_started=false;

	m_rank=core->getMessagesHandler()->getRank();
	m_outboxAllocator=core->getOutboxAllocator();
	m_workerId=0;

	m_slaveStarted=false;
}

