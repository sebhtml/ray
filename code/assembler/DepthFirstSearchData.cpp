/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include <core/ray_windows.h>
#include <core/constants.h>
#include <assembler/DepthFirstSearchData.h>

/*
 * do a depth first search with max depth of maxDepth;
 */
void DepthFirstSearchData::depthFirstSearch(Kmer root,Kmer a,int maxDepth,
	bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,vector<Kmer>*receivedOutgoingEdges,
		int minimumCoverage,bool*edgesReceived,int wordSize){
	if(!m_doChoice_tips_dfs_initiated){
		m_depthFirstSearchVisitedVertices.clear();
		m_depthFirstSearchVisitedVertices_vector.clear();

		// add an arc
		m_depthFirstSearchVisitedVertices_vector.push_back(root);
		m_depthFirstSearchVisitedVertices_vector.push_back(a);

		m_depthFirstSearchVisitedVertices_depths.clear();
		while(m_depthFirstSearchVerticesToVisit.size()>0){
			m_depthFirstSearchVerticesToVisit.pop();
		}
		while(m_depthFirstSearchDepths.size()>0){
			m_depthFirstSearchDepths.pop();
		}
		m_maxDepthReached=false;
		m_depthFirstSearchVerticesToVisit.push(a);
		m_depthFirstSearchVisitedVertices.insert(a);
		m_depthFirstSearchDepths.push(0);
		m_depthFirstSearch_maxDepth=0;
		m_doChoice_tips_dfs_initiated=true;
		m_doChoice_tips_dfs_done=false;
		m_coverages.clear();
		(*edgesRequested)=false;
		(*vertexCoverageRequested)=false;
		#ifdef SHOW_MINI_GRAPH
		cout<<"<MiniGraph>"<<endl;
		cout<<idToWord(root,wordSize)<<" -> "<<idToWord(a,wordSize)<<endl;
		#endif
	}
	if(m_depthFirstSearchVerticesToVisit.size()>0){
		Kmer vertexToVisit=m_depthFirstSearchVerticesToVisit.top();
		if(!(*vertexCoverageRequested)){
			(*vertexCoverageRequested)=true;
			(*vertexCoverageReceived)=false;
			
			uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(KMER_U64_ARRAY_SIZE*sizeof(uint64_t));
			int j=0;
			vertexToVisit.pack(message,&j);
			int dest=vertexRank(&vertexToVisit,size,wordSize);
			
			Message aMessage(message,j,MPI_UNSIGNED_LONG_LONG,dest,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,theRank);
			(*outbox).push_back(aMessage);
		}else if((*vertexCoverageReceived)){
			if(!(*edgesRequested)){
				m_coverages[vertexToVisit]=(*receivedVertexCoverage);
				m_depthFirstSearchVisitedVertices.insert(vertexToVisit);
				int theDepth=m_depthFirstSearchDepths.top();

				if(theDepth> m_depthFirstSearch_maxDepth){
					m_depthFirstSearch_maxDepth=theDepth;
				}
			
				// visit the vertex, and ask next edges.
				uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(1*sizeof(uint64_t));
				int bufferPosition=0;
				vertexToVisit.pack(message,&bufferPosition);
				int destination=vertexRank(&vertexToVisit,size,wordSize);
				Message aMessage(message,bufferPosition,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_REQUEST_VERTEX_OUTGOING_EDGES,theRank);
				(*outbox).push_back(aMessage);
				(*edgesRequested)=true;
				(*edgesReceived)=false;
			}else if((*edgesReceived)){
				Kmer vertexToVisit=m_depthFirstSearchVerticesToVisit.top();
				int theDepth=m_depthFirstSearchDepths.top();
				#ifdef ASSERT
				assert(theDepth>=0);
				assert(theDepth<=maxDepth);
				#endif
				int newDepth=theDepth+1;

				m_depthFirstSearchVerticesToVisit.pop();
				m_depthFirstSearchDepths.pop();

				for(int i=0;i<(int)(*receivedOutgoingEdges).size();i++){
					Kmer nextVertex=(*receivedOutgoingEdges)[i];
					if(m_depthFirstSearchVisitedVertices.count(nextVertex)>0){
						continue;
					}
					if(newDepth>maxDepth){
						m_maxDepthReached=true;
						continue;
					}

					if(m_depthFirstSearchVisitedVertices.size()<MAX_VERTICES_TO_VISIT){
						// add an arc
						m_depthFirstSearchVisitedVertices_vector.push_back(vertexToVisit);
						m_depthFirstSearchVisitedVertices_vector.push_back(nextVertex);

						// add the depth for the vertex
						m_depthFirstSearchVisitedVertices_depths.push_back(newDepth);

						// stacks
						m_depthFirstSearchVerticesToVisit.push(nextVertex);
						m_depthFirstSearchDepths.push(newDepth);
					}


					#ifdef SHOW_MINI_GRAPH
					cout<<idToWord(vertexToVisit,wordSize)<<" -> "<<idToWord(nextVertex,wordSize)<<endl;
					#endif
				}
				(*edgesRequested)=false;
				(*vertexCoverageRequested)=false;
			}
		}
	}else{
		m_doChoice_tips_dfs_done=true;
		#ifdef SHOW_MINI_GRAPH
		cout<<"</MiniGraph>"<<endl;
		#endif
	}
}

void DepthFirstSearchData::depthFirstSearchBidirectional(Kmer a,int maxDepth,
	bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,SeedingData*seedingData,
		int minimumCoverage,bool*edgesReceived,Parameters*parameters){

	#ifdef ASSERT
	int wordSize=parameters->getWordSize();
	#endif

	if(!m_doChoice_tips_dfs_initiated){
		m_outgoingEdges.clear();
		m_ingoingEdges.clear();

		m_depthFirstSearchVisitedVertices.clear();
		m_depthFirstSearchVisitedVertices_vector.clear();
		m_depthFirstSearchVisitedVertices_depths.clear();
		while(m_depthFirstSearchVerticesToVisit.size()>0){
			m_depthFirstSearchVerticesToVisit.pop();
		}
		while(m_depthFirstSearchDepths.size()>0){
			m_depthFirstSearchDepths.pop();
		}
		m_maxDepthReached=false;
		m_depthFirstSearchVerticesToVisit.push(a);
		m_depthFirstSearchDepths.push(0);
		m_depthFirstSearch_maxDepth=0;
		m_doChoice_tips_dfs_initiated=true;
		m_doChoice_tips_dfs_done=false;
		m_coverages.clear();
		(*edgesRequested)=false;
		(*vertexCoverageRequested)=false;
	}
	if(m_depthFirstSearchVerticesToVisit.size()>0){
		Kmer vertexToVisit=m_depthFirstSearchVerticesToVisit.top();

		if(!(*vertexCoverageRequested)){

			if(m_depthFirstSearchVisitedVertices.count(vertexToVisit)>0){
				m_depthFirstSearchVerticesToVisit.pop();
				m_depthFirstSearchDepths.pop();
				return;
			}

			(*vertexCoverageRequested)=true;
			(*vertexCoverageReceived)=false;
			
			uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(KMER_U64_ARRAY_SIZE*sizeof(uint64_t));
			int bufferPosition=0;
			vertexToVisit.pack(message,&bufferPosition);
			int dest=vertexRank(&vertexToVisit,size,parameters->getWordSize());
			Message aMessage(message,bufferPosition,MPI_UNSIGNED_LONG_LONG,dest,RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE,theRank);
			(*outbox).push_back(aMessage);
		}else if((*vertexCoverageReceived)){
			if(!(*edgesRequested)){
				m_coverages[vertexToVisit]=(*receivedVertexCoverage);

				#ifdef ASSERT
				if(m_depthFirstSearchVisitedVertices.count(vertexToVisit)>0){
					cout<<"Already visited: "<<idToWord(&vertexToVisit,wordSize)<<" root is "<<idToWord(&a,wordSize)<<endl;
				}
				assert(m_depthFirstSearchVisitedVertices.count(vertexToVisit)==0);
				assert(*receivedVertexCoverage>0);
				#endif

				if((*receivedVertexCoverage)>0){
					m_depthFirstSearchVisitedVertices.insert(vertexToVisit);
				}else{
					#ifdef ASSERT
					assert(false);
					#endif
					// don't visit it.
					m_depthFirstSearchVerticesToVisit.pop();
					m_depthFirstSearchDepths.pop();
					(*edgesRequested)=false;
					(*vertexCoverageRequested)=false;
					return;
				}
				int theDepth=m_depthFirstSearchDepths.top();

				if(theDepth> m_depthFirstSearch_maxDepth){
					m_depthFirstSearch_maxDepth=theDepth;
				}
			
				// visit the vertex, and ask next edges.
				uint64_t*message=(uint64_t*)(*outboxAllocator).allocate(1*sizeof(uint64_t));
				int bufferPosition=0;
				vertexToVisit.pack(message,&bufferPosition);
				int destination=vertexRank(&vertexToVisit,size,parameters->getWordSize());
				Message aMessage(message,bufferPosition,MPI_UNSIGNED_LONG_LONG,destination,RAY_MPI_TAG_REQUEST_VERTEX_EDGES,theRank);

				(*outbox).push_back(aMessage);
				(*edgesRequested)=true;
				(*edgesReceived)=false;
			}else if((*edgesReceived)){
				Kmer vertexToVisit=m_depthFirstSearchVerticesToVisit.top();
				int theDepth=m_depthFirstSearchDepths.top();

				#ifdef ASSERT
	
				assert(theDepth>=0);
				assert(theDepth<=maxDepth);
				#endif

				int newDepth=theDepth+1;

				m_depthFirstSearchVerticesToVisit.pop();
				m_depthFirstSearchDepths.pop();

				// the first 4 elements are padding
				// the 5th is the number of outgoing edges
				// following are the outgoing edges
				// following is the number of ingoing edges
				// following are the ingoing edges.

				vector<Kmer > outgoingEdges=seedingData->m_SEEDING_receivedOutgoingEdges;

				for(int i=0;i<(int)outgoingEdges.size();i++){
					Kmer nextVertex=outgoingEdges[i];

					if(m_depthFirstSearchVisitedVertices.size()>=MAX_VERTICES_TO_VISIT){
						continue;
					}
					if(m_depthFirstSearchVisitedVertices.count(nextVertex)>0){
						continue;
					}
					if(newDepth>maxDepth){
						m_maxDepthReached=true;
						continue;
					}
					m_depthFirstSearchVerticesToVisit.push(nextVertex);
					m_depthFirstSearchDepths.push(newDepth);
		
					m_depthFirstSearchVisitedVertices_vector.push_back(vertexToVisit);
					m_depthFirstSearchVisitedVertices_vector.push_back(nextVertex);
					m_depthFirstSearchVisitedVertices_depths.push_back(newDepth);
				}

				#ifdef ASSERT
				if(m_outgoingEdges.count(vertexToVisit)>0){
					cout<<idToWord(&vertexToVisit,wordSize)<<" is already in the data structure "<<m_outgoingEdges[vertexToVisit].size()<<" v. "<<outgoingEdges.size()<<endl;
				}
				assert(m_outgoingEdges.count(vertexToVisit)==0);
				#endif

				m_outgoingEdges[vertexToVisit]=outgoingEdges;

				vector<Kmer> ingoingEdges=seedingData->m_SEEDING_receivedIngoingEdges;

				for(int i=0;i<(int)ingoingEdges.size();i++){
					Kmer nextVertex=ingoingEdges[i];

					if(m_depthFirstSearchVisitedVertices.size()>=MAX_VERTICES_TO_VISIT){
						continue;
					}
					if(m_depthFirstSearchVisitedVertices.count(nextVertex)>0){
						continue;
					}
					if(newDepth>maxDepth){
						m_maxDepthReached=true;
						continue;
					}
					m_depthFirstSearchVerticesToVisit.push(nextVertex);
					m_depthFirstSearchDepths.push(newDepth);

					// reverse the order.
					m_depthFirstSearchVisitedVertices_vector.push_back(nextVertex);
					m_depthFirstSearchVisitedVertices_vector.push_back(vertexToVisit);
					m_depthFirstSearchVisitedVertices_depths.push_back(newDepth);
				}

				(*edgesRequested)=false;
				(*vertexCoverageRequested)=false;

				#ifdef ASSERT
				assert(m_ingoingEdges.count(vertexToVisit)==0);
	
				#endif

				m_ingoingEdges[vertexToVisit]=ingoingEdges;
			}
		}
	}else{
		m_doChoice_tips_dfs_done=true;
		#ifdef SHOW_MINI_GRAPH
		cout<<"</MiniGraph>"<<endl;
		#endif
	}
}

map<Kmer,vector<Kmer> >*DepthFirstSearchData::getIngoingEdges(){
	return &m_ingoingEdges;
}

map<Kmer,vector<Kmer> >*DepthFirstSearchData::getOutgoingEdges(){
	return &m_outgoingEdges;
}

