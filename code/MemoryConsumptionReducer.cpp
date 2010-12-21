/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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

#include<MemoryConsumptionReducer.h>

MemoryConsumptionReducer::MemoryConsumptionReducer(){
	m_initiated=false;
}

bool MemoryConsumptionReducer::reduce(MyForest*a,Parameters*parameters,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,vector<uint64_t>*receivedOutgoingEdges,
		int minimumCoverage,bool*edgesReceived
){
	int wordSize=parameters->getWordSize();
	if(!m_initiated){
		m_iterator.constructor(a);
		m_initiated=true;
		m_toRemove.clear();
		m_currentVertexIsDone=false;
		m_hasSetVertex=false;
		a->freeze();
	}else if(!m_currentVertexIsDone){
		if(!m_hasSetVertex){
			if(!m_iterator.hasNext()){
				m_initiated=false;
				a->unfreeze();
				return true;
			}
			m_firstVertex=m_iterator.next();

			while(!isCandidate(m_firstVertex,wordSize)){
				if(!m_iterator.hasNext()){
					return false;
				}
				m_firstVertex=m_iterator.next();
			}

			m_hasSetVertex=true;
			m_doneWithOutgoingEdges=false;
			m_dfsDataOutgoing.m_doChoice_tips_dfs_done=false;
			m_dfsDataOutgoing.m_doChoice_tips_dfs_initiated=false;
		}else if(!isCandidate(m_firstVertex,wordSize)){
			m_hasSetVertex=false;
		}else if(!m_doneWithOutgoingEdges){
			uint64_t key=m_firstVertex->getKey();
			vector<uint64_t> parents=m_firstVertex->getValue()->getIngoingEdges(key,wordSize);
			vector<uint64_t> children=m_firstVertex->getValue()->getOutgoingEdges(key,wordSize);
			int maximumDepth=wordSize+1;

			uint64_t next=0;
			if(children.size()==1){
				next=children[0];
			}else if(parents.size()==1){
				next=parents[0];
			}

			if(!m_dfsDataOutgoing.m_doChoice_tips_dfs_done){
				//cout<<"visit. "<<endl;
				m_dfsDataOutgoing.depthFirstSearchBidirectional(key,next,maximumDepth,
edgesRequested,
vertexCoverageRequested,
vertexCoverageReceived,
outboxAllocator,
size,
theRank,
outbox,
receivedVertexCoverage,
receivedOutgoingEdges,
minimumCoverage,
edgesReceived
);
			}else{
				#define _SHOW_GRAPH
				#ifdef _SHOW_GRAPH
				if(parameters->getRank()==MASTER_RANK
				&& m_dfsDataOutgoing.m_maxDepthReached
				&& m_firstVertex->getValue()->getCoverage()==1){
					cout<<"root="<<idToWord(key,wordSize)<<endl;
					cout<<"Parents: "<<parents.size()<<endl;
					cout<<"Children: "<<children.size()<<endl;
					cout<<m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size()/2<<" edges."<<endl;
			
					cout<<"digraph{"<<endl;
					for(map<uint64_t,int>::iterator p=m_dfsDataOutgoing.m_coverages.begin();
						p!=m_dfsDataOutgoing.m_coverages.end();p++){
						cout<<idToWord(p->first,wordSize)<<" [label=\""<<idToWord(p->first,wordSize)<<" "<<p->second<<"\"]"<<endl;
					}

					for(int j=0;j<(int)m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size();j+=2){
						uint64_t prefix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+0];
						uint64_t suffix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+1];
						cout<<idToWord(prefix,wordSize)<<" -> "<<idToWord(suffix,wordSize)<<endl;
					}
					cout<<"}"<<endl;
				}
				#endif

				// 1 and 10
				// 2 and 15
				// 3 and 20

				int originRedundancy=m_firstVertex->getValue()->getCoverage();
				int destinationRedundancy=6;
				if(originRedundancy==1){
					destinationRedundancy=10;
				}else if(originRedundancy==2){
					destinationRedundancy=10;
				}else if(originRedundancy==3){
					destinationRedundancy=15;
				}

				// find the first probably-good vertex 
				map<uint64_t,uint64_t> nextVertex;
				for(int j=0;j<(int)m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size();j+=2){
					uint64_t prefix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+0];
					uint64_t suffix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+1];
					nextVertex[prefix]=suffix;
				}
				
				uint64_t current=key;
				vector<uint64_t> path;
				bool foundDestination=false;

				while(1){
					int theCoverage=m_dfsDataOutgoing.m_coverages[current];
					if(theCoverage>=destinationRedundancy){
						foundDestination=true;
						break;
					}else{
						path.push_back(current);
						if(nextVertex.count(current)>0){
							current=nextVertex[current];
						}else{
							break;
						}
					}
				}
				if(foundDestination){
					if(parameters->getRank()==MASTER_RANK){
						cout<<"removed "<<path.size()<<endl;
					}
					for(int u=0;u<(int)path.size();u++){
						m_toRemove.push_back(path[u]);
					}
				}

				m_doneWithOutgoingEdges=true;
			}
		}else{
			// done with it., next one.
			m_hasSetVertex=false;
		}
	}
	return false;
}

int MemoryConsumptionReducer::getNumberOfRemovedVertices(){
	return m_toRemove.size();
}

bool MemoryConsumptionReducer::isCandidate(SplayNode<uint64_t,Vertex>*m_firstVertex,int wordSize){
	uint64_t key=m_firstVertex->getKey();
	vector<uint64_t> parents=m_firstVertex->getValue()->getIngoingEdges(key,wordSize);
	vector<uint64_t> children=m_firstVertex->getValue()->getOutgoingEdges(key,wordSize);
	int coverage=m_firstVertex->getValue()->getCoverage();
	return ((parents.size()==1&&children.size()==0)||(parents.size()==0&&children.size()==1))&&coverage<=3;
}
