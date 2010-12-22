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
#include<stack>
using namespace std;

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
		m_maximumDepth=2*wordSize+1;
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

			if(!m_dfsDataOutgoing.m_doChoice_tips_dfs_done){
				//cout<<"visit. "<<endl;
				m_dfsDataOutgoing.depthFirstSearchBidirectional(key,m_maximumDepth,
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
				// find the first probably-good vertex 
				map<uint64_t,vector<uint64_t> > theParents;
				map<uint64_t,vector<uint64_t> > theChildren;

				for(int j=0;j<(int)m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size();j+=2){
					uint64_t prefix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+0];
					uint64_t suffix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+1];
					theChildren[prefix].push_back(suffix);
					theParents[suffix].push_back(prefix);
				}

				vector<uint64_t> path;
				bool foundDestination=false;

				set<uint64_t> visited;

				if(parents.size()==0){
					bool foundJunction=false;
					uint64_t current=key;
					while(1){
						if(visited.count(current)>0){
							break;
						}else if(theParents[current].size()>1||theChildren[current].size()>1){
							foundJunction=true;
							break;
						}else if(theChildren.count(current)>0&&theChildren[current].size()==1){
							visited.insert(current);
							path.push_back(current);
							current=theChildren[current][0];
						}else{
							break;
						}
					}
					if(foundJunction){
						stack<uint64_t> nodes;
						stack<int> depths;
						depths.push(0);
						nodes.push(current);
						int maximumDepth=0;
						while(!nodes.empty()){
							uint64_t node=nodes.top();
							visited.insert(node);
							nodes.pop();
							int nodeDepth=depths.top();
							depths.pop();
							if(nodeDepth>maximumDepth){
								maximumDepth=nodeDepth;
							}
							int newDepth=nodeDepth+1;
							for(int k=0;k<(int)theParents[node].size();k++){
								uint64_t nextVertex=theParents[node][k];
								if(visited.count(nextVertex)>0){
									continue;
								}
								nodes.push(nextVertex);
								depths.push(newDepth);
							}
							for(int k=0;k<(int)theChildren[node].size();k++){
								uint64_t nextVertex=theChildren[node][k];
								if(visited.count(nextVertex)>0){
									continue;
								}
								nodes.push(nextVertex);
								depths.push(newDepth);
							}
						}
						//cout<<"Depth reached: "<<maximumDepth<<" vs "<<path.size()<<endl;
						if(maximumDepth+(int)path.size()==m_maximumDepth&&(int)path.size()<=wordSize){
							foundDestination=true;
						}
					}
				}else if(children.size()==0){
					bool foundJunction=false;
					uint64_t current=key;
					while(1){
						if(visited.count(current)>0){
							break;
						}else if(theParents[current].size()>1||theChildren[current].size()>1){
							foundJunction=true;
							break;
						}else if(theParents.count(current)>0&&theParents[current].size()==1){
							visited.insert(current);
							path.push_back(current);
							current=theParents[current][0];
						}else{
							break;
						}
					}
					if(foundJunction){
						stack<uint64_t> nodes;
						stack<int> depths;
						depths.push(0);
						nodes.push(current);
						int maximumDepth=0;
						while(!nodes.empty()){
							uint64_t node=nodes.top();
							visited.insert(node);
							nodes.pop();
							int nodeDepth=depths.top();
							depths.pop();
							if(nodeDepth>maximumDepth){
								maximumDepth=nodeDepth;
							}
							int newDepth=nodeDepth+1;
							for(int k=0;k<(int)theParents[node].size();k++){
								uint64_t nextVertex=theParents[node][k];
								if(visited.count(nextVertex)>0){
									continue;
								}
								nodes.push(nextVertex);
								depths.push(newDepth);
							}

							for(int k=0;k<(int)theChildren[node].size();k++){
								uint64_t nextVertex=theChildren[node][k];
								if(visited.count(nextVertex)>0){
									continue;
								}
								nodes.push(nextVertex);
								depths.push(newDepth);
							}
						}
						//cout<<"Depth reached: "<<maximumDepth<<" vs "<<path.size()<<" MAX="<<m_maximumDepth<<endl;

						if(maximumDepth+(int)path.size()==m_maximumDepth&&(int)path.size()<=wordSize){
							//cout<<"deleting "<<path.size()<<endl;
							foundDestination=true;
						}
					}
				}

				bool processed=false;
				if(foundDestination){
					if(processed&&parameters->getRank()==MASTER_RANK){
						//cout<<"removed "<<path.size()<<endl;
					}
					for(int u=0;u<(int)path.size();u++){
						m_toRemove.push_back(path[u]);
					}
				}else{
					processed=true;
					if(processed&&parameters->getRank()==MASTER_RANK){
				#define _SHOW_GRAPH
				#ifdef _SHOW_GRAPH
				if(parameters->getRank()==MASTER_RANK
				&& m_dfsDataOutgoing.m_maxDepthReached){
					processed=true;
					cout<<"BEGIN"<<endl;
					cout<<"root="<<idToWord(key,wordSize)<<endl;
					cout<<"Parents: "<<parents.size()<<endl;
					cout<<"Children: "<<children.size()<<endl;
					cout<<m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size()/2<<" edges."<<endl;
			
					cout<<"digraph{"<<endl;
					for(map<uint64_t,int>::iterator p=m_dfsDataOutgoing.m_coverages.begin();
						p!=m_dfsDataOutgoing.m_coverages.end();p++){
						if(key==p->first){
							cout<<idToWord(p->first,wordSize)<<" [label=\""<<idToWord(p->first,wordSize)<<" "<<p->second<<" root\"]"<<endl;
						}else{
							cout<<idToWord(p->first,wordSize)<<" [label=\""<<idToWord(p->first,wordSize)<<" "<<p->second<<"\"]"<<endl;
						}
					}

					for(int j=0;j<(int)m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size();j+=2){
						uint64_t prefix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+0];
						uint64_t suffix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+1];
						cout<<idToWord(prefix,wordSize)<<" -> "<<idToWord(suffix,wordSize)<<endl;
					}
					cout<<"}"<<endl;


					cout<<"no destination found"<<endl;
					cout<<"END"<<endl;
				}
				#endif

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
