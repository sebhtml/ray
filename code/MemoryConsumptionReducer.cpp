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
#include<CoverageDistribution.h>
#include<stack>
#include<set>
using namespace std;

//#define _VERBOSE

void MemoryConsumptionReducer::getPermutations(uint64_t kmer,int length,vector<uint64_t>*output,int wordSize){
	#ifdef ASSERT
	assert(output->size()==0);
	assert(length<=wordSize);
	#endif
	string stringVersion=idToWord(kmer,wordSize);
	char buffer[50];
	strcpy(buffer,stringVersion.c_str());
	vector<char> changes;
	changes.push_back('A');
	changes.push_back('T');
	changes.push_back('C');
	changes.push_back('G');

	for(int i=0;i<length;i++){
		for(int j=0;j<(int)changes.size();j++){
			char oldNucleotide=buffer[i];
			char newNucleotide=changes[j];
			if(oldNucleotide==newNucleotide){
				continue;
			}
			buffer[i]=newNucleotide;
			uint64_t a=wordId(buffer);
			buffer[i]=oldNucleotide;
			output->push_back(a);
		}
	}

	#ifdef ASSERT
	assert((int)output->size()==length*3);
	#endif
}

/*
 * algorithm:
 *
 * junction
 *
 * root
 *
 * from root to junction (junction is excluded), store the maximum coverage encountered A
 *
 * from junction to the vertex at maximum depth in the other child, store the minimum coverage encountered B
 *
 * to remove a path, the maximum A must be lower than the minimum B
 *
 */
MemoryConsumptionReducer::MemoryConsumptionReducer(){
	m_initiated=false;
}

vector<uint64_t> MemoryConsumptionReducer::computePath(map<uint64_t,vector<uint64_t> >*edges,uint64_t start,uint64_t end,set<uint64_t>*visited){
	vector<uint64_t> path;
	if(visited->count(start)>0){
		return path;
	}
	visited->insert(start);
	if(start==end){
		path.push_back(start);
		return path;
	}
	if(edges->count(start)==0){
		return path;
	}
	vector<uint64_t> nextVertices=(*edges)[start];
	for(int j=0;j<(int)nextVertices.size();j++){
		uint64_t vertex=nextVertices[j];
		vector<uint64_t> aPath=computePath(edges,vertex,end,visited);
		if(aPath.size()>0){
			path.push_back(start);
			for(int l=0;l<(int)aPath.size();l++){
				path.push_back(aPath[l]);
			}
			return path;
		}
	}
	return path;
}

bool MemoryConsumptionReducer::isJunction(uint64_t vertex,map<uint64_t,vector<uint64_t> >*edges,int wordSize){
	if(edges->count(vertex)==0){
		return false;
	}
	if((*edges)[vertex].size()<2){
		return false;
	}
	// compute the depth at each depth
	// one leads to the root, and another must be longer than wordSize (not a tip)
	
	vector<uint64_t> nextVertices=(*edges)[vertex];
	for(int i=0;i<(int)nextVertices.size();i++){
		uint64_t current=nextVertices[i];
		stack<uint64_t> vertices;
		stack<int> depths;
		set<uint64_t> visited;
		vertices.push(current);
		depths.push(1);

		while(!vertices.empty()){
			uint64_t topVertex=vertices.top();
			vertices.pop();
			int topDepth=depths.top();
			depths.pop();
			if(visited.count(topVertex)){
				continue;
			}
			visited.insert(topVertex);

			if(topDepth>wordSize){
				//cout<<"is Junction"<<endl;
				return true;
			}
			if(edges->count(topVertex)==0){
				continue;
			}
			vector<uint64_t> nextBits=(*edges)[topVertex];
			int newDepth=topDepth+1;
			for(int j=0;j<(int)nextBits.size();j++){
				vertices.push(nextBits[j]);
				depths.push(newDepth);
			}
		}
	}
	return false;
}

bool MemoryConsumptionReducer::reduce(MyForest*a,Parameters*parameters,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,vector<uint64_t>*receivedOutgoingEdges,
		int minimumCoverage,bool*edgesReceived
){
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif

	if(m_pendingMessages>0){
		return false;
	}
	int wordSize=parameters->getWordSize();
	if(!m_initiated){
		m_counter=0;
		m_initiated=true;
		m_toRemove.clear();

/*
		if((int)m_confettiToCheck.size()>0){
			int remaining=m_confettiToCheck.size()-m_processedTasks.size();
			cout<<remaining<<" confetti were held back."<<endl;
		}
*/

		m_processedTasks.clear();
		m_confettiToCheck.clear();
		m_confettiMaxCoverage.clear();

		m_currentVertexIsDone=false;
		m_hasSetVertex=false;
		a->freeze();

		// wordSize for the hanging tip
		// wordSize+1 for the correct path
		// 1 for the junction
		// total: 2k+2
		m_maximumDepth=2*wordSize+2;

/*
		m_iterator.constructor(a);

		map<int,uint64_t> distribution;
		while(m_iterator.hasNext()){
			distribution[m_iterator.next()->getValue()->getCoverage()]++;
		}
		CoverageDistribution dis(&distribution,NULL);
		int peak=dis.getPeakCoverage();
		//printf("Rank %i: peak coverage is %i\n",parameters->getRank(),dis.getPeakCoverage());
		if(parameters->getRank()==MASTER_RANK){
			for(map<int,uint64_t>::iterator i=distribution.begin();i!=distribution.end();i++){
				cout<<i->first<<" -> "<<i->second<<endl;
			}
			cout<<"Peak is "<<peak<<endl;
		}
*/
		
		m_iterator.constructor(a);
	}else if(!m_currentVertexIsDone){
		if(!m_hasSetVertex){
			printCounter(parameters,a);
			if(!m_iterator.hasNext()){
				m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_CHECK_VERTEX,outboxAllocator,outbox,theRank);
				if(m_pendingMessages>0){
					return false;
				}
				m_initiated=false;
				a->unfreeze();
				printCounter(parameters,a);
				
				return true;
			}
			m_firstVertex=m_iterator.next();
			m_counter++;

			while(!isCandidate(m_firstVertex,wordSize)){
				if(!m_iterator.hasNext()){
					return false;
				}
				printCounter(parameters,a);
				m_firstVertex=m_iterator.next();
				m_counter++;
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
				bool foundJunction=false;
				set<uint64_t> visited;
				int maximumDepth=-99;
				int coverageAtMaxDepth=0;
				uint64_t vertexAtMaxDepth=0;
				uint64_t junction=0;
				int maximumCoverageInPath=0;
				int minimumCoverageInOtherPath=999;
				bool isLow=false;
				set<uint64_t> bestVertices;
				bool aloneBits=false;
				int maxReadLength=2*wordSize-1;
				int maxPathSize=maxReadLength-wordSize; // k-1

				if((parents.size()==0||children.size()==0)&&!(parents.size()==0&&children.size()==0)){
					map<uint64_t,vector<uint64_t> >*theEdges=&theParents;
					map<uint64_t,vector<uint64_t> >*otherEdges=&theChildren;
					if(parents.size()==0){
						theEdges=&theChildren;
						otherEdges=&theParents;
					}

					uint64_t current=key;
					while(1){
						if(visited.count(current)>0){
							break;
						}else if(isJunction(current,otherEdges,wordSize)){
							junction=current;
							foundJunction=true;
							break;
						}
						visited.insert(current);

						if(theEdges->count(current)>0&&(*theEdges)[current].size()==1){
							path.push_back(current);
							int theCoverageOfCurrent=m_dfsDataOutgoing.m_coverages[current];
							if(theCoverageOfCurrent>maximumCoverageInPath){
								maximumCoverageInPath=theCoverageOfCurrent;
							}	
							current=(*theEdges)[current][0];
						}else{
							break;
						}
					}

					if(foundJunction){
						stack<uint64_t> nodes;
						stack<int> depths;

						depths.push(0);
						nodes.push(junction);
						while(!nodes.empty()){
							uint64_t node=nodes.top();

							int theCoverageOfCurrent=m_dfsDataOutgoing.m_coverages[node];

							visited.insert(node);
							nodes.pop();
							int nodeDepth=depths.top();
							depths.pop();
							
							if(nodeDepth>maximumDepth
							||(nodeDepth==maximumDepth &&theCoverageOfCurrent>coverageAtMaxDepth)){
								maximumDepth=nodeDepth;
								coverageAtMaxDepth=theCoverageOfCurrent;
								vertexAtMaxDepth=node;
							}

							int newDepth=nodeDepth+1;

							for(int k=0;k<(int)(*otherEdges)[node].size();k++){
								uint64_t nextVertex=(*otherEdges)[node][k];
								if(visited.count(nextVertex)>0){
									continue;
								}
								nodes.push(nextVertex);
								depths.push(newDepth);
							}
						}
			
						set<uint64_t> visited;
						vector<uint64_t> bestPath=computePath(otherEdges,junction,vertexAtMaxDepth,&visited);
						for(int u=0;u<(int)bestPath.size();u++){
							uint64_t node=bestPath[u];
							int theCoverageOfCurrent=m_dfsDataOutgoing.m_coverages[node];
							if(theCoverageOfCurrent<minimumCoverageInOtherPath){
								minimumCoverageInOtherPath=theCoverageOfCurrent;
							}
							bestVertices.insert(node);
						}

						//cout<<"Depth reached: "<<maximumDepth<<" vs "<<path.size()<<endl;
						if(maximumDepth>wordSize&&(int)path.size()<=wordSize){
							foundDestination=true;
						}
						if(maximumCoverageInPath>=minimumCoverageInOtherPath){
							isLow=true;
							foundDestination=false;
						}

					// confetti in the graph
					}else if(children.size()==0){
						aloneBits=true;
						if(!((int)path.size()<=maxPathSize)){
							aloneBits=false;
						}

						for(int o=0;o<(int)path.size();o++){
							if(!aloneBits){
								break;
							}
							if(m_dfsDataOutgoing.m_coverages[path[o]]!=1){
								#ifdef _VERBOSE
								cout<<path.size()<<" vertices, no junction, and a vertex has a coverage greater than 1"<<endl;
								#endif
								aloneBits=false;
								break;
							}
						}

						if(aloneBits&&(int)path.size()<=maxPathSize){
							//foundDestination=true;
							//
							// example:
							//
							// read length: 36
							// k: 21
							// number of vertices in the path: 36-21=15
							//
							// length of the zone to check: 21-15=6
							//
							// so in general: pathSize-kMerSize is the number of position to change at the beginning of the root,
							// with the root being the last k-mer
							//
							// add the path in the storage section
							// request the validity of the <path.size()
							
							int positionsToCheck=wordSize-path.size();
							int uniqueId=m_confettiToCheck.size();
							m_confettiToCheck.push_back(path);
							m_confettiMaxCoverage.push_back(maximumCoverageInPath);
							uint64_t root=path[0];
							vector<uint64_t> kMersToCheck;
							getPermutations(root,positionsToCheck,&kMersToCheck,wordSize);
							
							// push queries in a buffer
							for(int i=0;i<(int)kMersToCheck.size();i++){
								uint64_t kmer=kMersToCheck[i];
								int destination=vertexRank(kmer,size);
								m_bufferedData.addAt(destination,uniqueId);
								m_bufferedData.addAt(destination,kmer);
								if(m_bufferedData.flush(destination,2,RAY_MPI_TAG_CHECK_VERTEX,outboxAllocator,outbox,theRank,false)){
									m_pendingMessages++;
								}
							}
						}
					}
				}

				bool processed=false;
				if(foundDestination){
					if(processed&&parameters->getRank()==MASTER_RANK){
						cout<<"removed "<<path.size()<<endl;
					}
					for(int u=0;u<(int)path.size();u++){
						m_toRemove.push_back(path[u]);
					}
				}
				#ifdef _VERBOSE
				#define PRINT_GRAPHVIZ
				#endif
				#ifdef PRINT_GRAPHVIZ
				if(parameters->getRank()==MASTER_RANK 
				&&!foundJunction&&!aloneBits&&children.size()==0&&(int)path.size()<=maxPathSize){
					set<uint64_t> removed;
					for(int p=0;p<(int)path.size();p++){
						if(foundDestination){
							removed.insert(path[p]);
						}
					}
					processed=true;
					cout<<"BEGIN"<<endl;
					cout<<"foundJunction="<<foundJunction<<endl;
					cout<<"foundDestination="<<foundDestination<<endl;
					cout<<"aloneBits="<<aloneBits<<endl;
					cout<<"Depth reached: "<<maximumDepth<<" vs "<<path.size()<<" MAX="<<m_maximumDepth<<endl;
					cout<<"root="<<idToWord(key,wordSize)<<endl;
					cout<<"Parents: "<<parents.size()<<endl;
					cout<<"Children: "<<children.size()<<endl;
					cout<<m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size()/2<<" edges."<<endl;
					cout<<"MaximumCoverageInPath="<<maximumCoverageInPath<<endl;
					cout<<"MinimumCoverageInOtherPath="<<minimumCoverageInOtherPath<<endl;
			
					cout<<"digraph{"<<endl;
					cout<<"node [color=lightblue2 style=filled]"<<endl;
					for(map<uint64_t,int>::iterator p=m_dfsDataOutgoing.m_coverages.begin();
						p!=m_dfsDataOutgoing.m_coverages.end();p++){
						cout<<idToWord(p->first,wordSize)<<" [label=\""<<idToWord(p->first,wordSize)<<" "<<p->second;
						if(key==p->first){
							cout<<" (root) ";
						}
						if(junction==p->first){
							cout<<" (junction) ";
						}

						if(vertexAtMaxDepth==p->first){
							cout<<" (deepest vertex) ";
						}

						cout<<"\" ";
						if(removed.count(p->first)>0){
							cout<<" color=salmon2";
						}

						if(bestVertices.count(p->first)>0){
							cout<<" color=greenyellow";
						}
						cout<<" ] "<<endl;
					}

					for(int j=0;j<(int)m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size();j+=2){
						uint64_t prefix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+0];
						uint64_t suffix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+1];
						cout<<idToWord(prefix,wordSize)<<" -> "<<idToWord(suffix,wordSize)<<endl;
						#ifdef ASSERT
						assert(m_dfsDataOutgoing.m_coverages.count(prefix)>0);
						assert(m_dfsDataOutgoing.m_coverages.count(suffix)>0);
						#endif
					}
					cout<<"}"<<endl;


					cout<<"END"<<endl;
				}
				#endif

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

void MemoryConsumptionReducer::printCounter(Parameters*parameters,MyForest*forest){
	if(m_counter==forest->size()){
		printf("Rank %i is reducing memory usage [%lu/%lu] (completed)\n",parameters->getRank(),m_counter,forest->size());
	}else if(m_counter%40000==0){
		printf("Rank %i is reducing memory usage [%lu/%lu]\n",parameters->getRank(),m_counter+1,forest->size());
	}
}

vector<uint64_t>*MemoryConsumptionReducer::getVerticesToRemove(){
	return &m_toRemove;
}

void MemoryConsumptionReducer::constructor(int size){
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES);
	m_pendingMessages=0;
}

void MemoryConsumptionReducer::processConfetti(uint64_t*a,int b){
	for(int i=0;i<b;i+=2){
		int task=a[i+0];
		int coverage=a[i+1];
		if(m_processedTasks.count(task)>0){
			continue;
		}
		m_processedTasks.insert(task);
		vector<uint64_t>*vertices=&m_confettiToCheck[task];
		int maxCoverage=m_confettiMaxCoverage[task];
		if(coverage<maxCoverage){
			continue;
		}
		for(int j=0;j<(int)vertices->size();j++){
			m_toRemove.push_back(vertices->at(j));
		}
	}
	m_pendingMessages--;
}
