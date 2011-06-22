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

#include <assembler/MemoryConsumptionReducer.h>
#include <graph/CoverageDistribution.h>
#include <stack>
#include <string.h>
#include <set>
#include <memory/malloc_types.h>
using namespace std;

void MemoryConsumptionReducer::getPermutations(Kmer kmer,int length,vector<Kmer>*output,int wordSize){
	#ifdef ASSERT
	assert(output->size()==0);
	assert(length<=wordSize);
	#endif
	string stringVersion=idToWord(&kmer,wordSize,m_parameters->getColorSpaceMode());
	char buffer[1000];
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
			Kmer a=wordId(buffer);
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

vector<Kmer> MemoryConsumptionReducer::computePath(map<Kmer,vector<Kmer> >*edges,Kmer start,Kmer end,set<Kmer>*visited){
	vector<Kmer> path;
	if(visited->count(start)>0){
		return path;
	}
	visited->insert(start);
	if(start.isEqual(&end)){
		path.push_back(start);
		return path;
	}
	if(edges->count(start)==0){
		return path;
	}
	vector<Kmer> nextVertices=(*edges)[start];
	for(int j=0;j<(int)nextVertices.size();j++){
		Kmer vertex=nextVertices[j];
		vector<Kmer > aPath=computePath(edges,vertex,end,visited);
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

bool MemoryConsumptionReducer::isJunction(Kmer vertex,map<Kmer ,vector<Kmer> >*edges,int wordSize){
	if(edges->count(vertex)==0){
		return false;
	}
	if((*edges)[vertex].size()<2){
		return false;
	}
	// compute the depth at each depth
	// one leads to the root, and another must be longer than wordSize (not a tip)
	
	vector<Kmer> nextVertices=(*edges)[vertex];
	for(int i=0;i<(int)nextVertices.size();i++){
		Kmer current=nextVertices[i];
		stack<Kmer> vertices;
		stack<int> depths;
		set<Kmer> visited;
		vertices.push(current);
		depths.push(1);

		while(!vertices.empty()){
			Kmer topVertex=vertices.top();
			vertices.pop();
			int topDepth=depths.top();
			depths.pop();
			if(visited.count(topVertex)){
				continue;
			}
			visited.insert(topVertex);

			if(topDepth>wordSize){
				return true;
			}
			if(edges->count(topVertex)==0){
				continue;
			}
			vector<Kmer> nextBits=(*edges)[topVertex];
			int newDepth=topDepth+1;
			for(int j=0;j<(int)nextBits.size();j++){
				vertices.push(nextBits[j]);
				depths.push(newDepth);
			}
		}
	}
	return false;
}

bool MemoryConsumptionReducer::reduce(GridTable*a,Parameters*parameters,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,SeedingData*seedingData,
		int minimumCoverage,bool*edgesReceived
){
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	assert(a->frozen());
	#endif

	if(m_pendingMessages>0){
		return false;
	}
	int wordSize=parameters->getWordSize();
	if(!m_initiated){
		m_counter=0;
		m_initiated=true;

		constructor();

		m_currentVertexIsDone=false;
		m_hasSetVertex=false;

		// wordSize for the hanging tip
		// wordSize+1 for the correct path
		// 1 for the junction
		// total: 2k+2
		m_maximumDepth=2*wordSize+2;
		
		m_iterator.constructor(a,wordSize,parameters);
	}else if(!m_currentVertexIsDone){
		if(!m_hasSetVertex){
			if(!m_iterator.hasNext()){
				if(!m_bufferedData.isEmpty()){
					m_pendingMessages+=m_bufferedData.flushAll(RAY_MPI_TAG_CHECK_VERTEX,outboxAllocator,outbox,theRank);
					return false;
				}
				m_initiated=false;
				printCounter(parameters,a);
				
				return true;
			}
			printCounter(parameters,a);
			m_firstVertex=m_iterator.next();
			m_firstKey=*(m_iterator.getKey());
			m_counter++;

			while(!isCandidate(m_firstKey,m_firstVertex,wordSize)){
				if(!m_iterator.hasNext()){
					return false;
				}
				printCounter(parameters,a);
				m_firstVertex=m_iterator.next();
				m_firstKey=*(m_iterator.getKey());
				m_counter++;
			}

			m_hasSetVertex=true;
			m_doneWithOutgoingEdges=false;
			m_dfsDataOutgoing->m_doChoice_tips_dfs_done=false;
			m_dfsDataOutgoing->m_doChoice_tips_dfs_initiated=false;
		}else if(!isCandidate(m_firstKey,m_firstVertex,wordSize)){
			m_hasSetVertex=false;
		}else if(!m_doneWithOutgoingEdges){
			Kmer key=m_firstVertex->m_lowerKey;
			vector<Kmer > parents=m_firstVertex->getIngoingEdges(&key,wordSize);
			vector<Kmer > children=m_firstVertex->getOutgoingEdges(&key,wordSize);

			if(!m_dfsDataOutgoing->m_doChoice_tips_dfs_done){
				m_dfsDataOutgoing->depthFirstSearchBidirectional(key,m_maximumDepth,
edgesRequested,
vertexCoverageRequested,
vertexCoverageReceived,
outboxAllocator,
size,
theRank,
outbox,
receivedVertexCoverage,
seedingData,
minimumCoverage,
edgesReceived,parameters
);
			}else{
				// find the first probably-good vertex 
				map<Kmer,vector<Kmer> > theParents;
				map<Kmer,vector<Kmer> > theChildren;

				for(int j=0;j<(int)m_dfsDataOutgoing->m_depthFirstSearchVisitedVertices_vector.size();j+=2){
					Kmer prefix=m_dfsDataOutgoing->m_depthFirstSearchVisitedVertices_vector[j+0];
					Kmer suffix=m_dfsDataOutgoing->m_depthFirstSearchVisitedVertices_vector[j+1];
					theChildren[prefix].push_back(suffix);
					theParents[suffix].push_back(prefix);
				}

				vector<Kmer > path;
				bool foundDestination=false;
				bool foundJunction=false;
				set<Kmer > visited;
				int maximumDepth=-99;
				int coverageAtMaxDepth=0;
				Kmer vertexAtMaxDepth;
				Kmer junction;
				int maximumCoverageInPath=0;
				int minimumCoverageInOtherPath=999;
				bool isLow=false;
				set<Kmer > bestVertices;
				bool aloneBits=false;
				int maxReadLength=2*wordSize-1;
				int maxPathSize=maxReadLength-wordSize; // k-1

				if((parents.size()==0||children.size()==0)&&!(parents.size()==0&&children.size()==0)){
					map<Kmer ,vector<Kmer > >*theEdges=&theParents;
					map<Kmer ,vector<Kmer > >*otherEdges=&theChildren;
					if(parents.size()==0){
						theEdges=&theChildren;
						otherEdges=&theParents;
					}

					Kmer current=key;
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
							int theCoverageOfCurrent=m_dfsDataOutgoing->m_coverages[current];
							if(theCoverageOfCurrent>maximumCoverageInPath){
								maximumCoverageInPath=theCoverageOfCurrent;
							}	
							current=(*theEdges)[current][0];
						}else{
							break;
						}
					}

					if(foundJunction){
						stack<Kmer > nodes;
						stack<int> depths;

						depths.push(0);
						nodes.push(junction);
						while(!nodes.empty()){
							Kmer node=nodes.top();

							int theCoverageOfCurrent=m_dfsDataOutgoing->m_coverages[node];

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
								Kmer nextVertex=(*otherEdges)[node][k];
								if(visited.count(nextVertex)>0){
									continue;
								}
								nodes.push(nextVertex);
								depths.push(newDepth);
							}
						}
			
						set<Kmer > visited;
						vector<Kmer > bestPath=computePath(otherEdges,junction,vertexAtMaxDepth,&visited);
						for(int u=0;u<(int)bestPath.size();u++){
							Kmer node=bestPath[u];
							int theCoverageOfCurrent=m_dfsDataOutgoing->m_coverages[node];
							if(theCoverageOfCurrent<minimumCoverageInOtherPath){
								minimumCoverageInOtherPath=theCoverageOfCurrent;
							}
							bestVertices.insert(node);
						}

						if(maximumDepth>wordSize&&(int)path.size()<=wordSize){
							foundDestination=true;
						}
						if(maximumCoverageInPath>=minimumCoverageInOtherPath
							|| maximumCoverageInPath>3){
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
							if(m_dfsDataOutgoing->m_coverages[path[o]]!=1){
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
							int uniqueId=m_confettiToCheck->size();
							m_confettiToCheck->push_back(path);
							m_confettiMaxCoverage->push_back(maximumCoverageInPath);

							Kmer root=path[0];
							vector<Kmer > kMersToCheck;
							getPermutations(root,positionsToCheck,&kMersToCheck,wordSize);
							
							for(int u=0;u<(int)path.size();u++){	
								Kmer kmer=path[u];
								(*m_ingoingEdges)[kmer]=(*(m_dfsDataOutgoing->getIngoingEdges()))[kmer];
								(*m_outgoingEdges)[kmer]=(*(m_dfsDataOutgoing->getOutgoingEdges()))[kmer];
							}

							// push queries in a buffer
							for(int i=0;i<(int)kMersToCheck.size();i++){
								Kmer kmer=kMersToCheck[i];
								int destination=m_parameters->_vertexRank(&kmer);
								m_bufferedData.addAt(destination,uniqueId);
								for(int k=0;k<kmer.getNumberOfU64();k++){
									m_bufferedData.addAt(destination,kmer.getU64(k));
								}
								if(m_bufferedData.flush(destination,1+KMER_U64_ARRAY_SIZE,RAY_MPI_TAG_CHECK_VERTEX,outboxAllocator,outbox,theRank,false)){
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
						m_toRemove->push_back(path[u]);
						Kmer vertex=path[u];
			
						(*m_ingoingEdges)[vertex]=(*(m_dfsDataOutgoing->getIngoingEdges()))[vertex];

						(*m_outgoingEdges)[vertex]=(*(m_dfsDataOutgoing->getOutgoingEdges()))[vertex];

					}
				}
				#ifdef _VERBOSE
				#define PRINT_GRAPHVIZ
				#endif
				#ifdef PRINT_GRAPHVIZ
				if(
				forcePrint){
					set<Kmer > removed;
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
					cout<<"root="<<idToWord(key,wordSize,m_parameters->getColorSpaceMode())<<endl;
					cout<<"Parents: "<<parents.size()<<endl;
					cout<<"Children: "<<children.size()<<endl;
					cout<<m_dfsDataOutgoing->m_depthFirstSearchVisitedVertices_vector.size()/2<<" edges."<<endl;
					cout<<"MaximumCoverageInPath="<<maximumCoverageInPath<<endl;
					cout<<"MinimumCoverageInOtherPath="<<minimumCoverageInOtherPath<<endl;
			
					cout<<"digraph{"<<endl;
					cout<<"node [color=lightblue2 style=filled]"<<endl;
					for(map<Kmer ,int>::iterator p=m_dfsDataOutgoing->m_coverages.begin();
						p!=m_dfsDataOutgoing->m_coverages.end();p++){
						cout<<idToWord(p->first,wordSize,m_parameters->getColorSpaceMode())<<" [label=\""<<idToWord(p->first,wordSize,m_parameters->getColorSpaceMode())<<" "<<p->second;
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

					for(int j=0;j<(int)m_dfsDataOutgoing->m_depthFirstSearchVisitedVertices_vector.size();j+=2){
						Kmer prefix=m_dfsDataOutgoing->m_depthFirstSearchVisitedVertices_vector[j+0];
						Kmer suffix=m_dfsDataOutgoing->m_depthFirstSearchVisitedVertices_vector[j+1];
						cout<<idToWord(prefix,wordSize,m_parameters->getColorSpaceMode())<<" -> "<<idToWord(suffix,wordSize,m_parameters->getColorSpaceMode())<<endl;
						#ifdef ASSERT
						assert(m_dfsDataOutgoing->m_coverages.count(prefix)>0);
						assert(m_dfsDataOutgoing->m_coverages.count(suffix)>0);
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
	return m_toRemove->size();
}

bool MemoryConsumptionReducer::isCandidate(Kmer key,Vertex*m_firstVertex,int wordSize){
	vector<Kmer > parents=m_firstVertex->getIngoingEdges(&key,wordSize);
	vector<Kmer > children=m_firstVertex->getOutgoingEdges(&key,wordSize);
	int coverage=m_firstVertex->getCoverage(&key);
	return ((parents.size()==1&&children.size()==0)||(parents.size()==0&&children.size()==1))&&coverage==1;
}

void MemoryConsumptionReducer::printCounter(Parameters*parameters,GridTable*forest){
	if(m_counter==forest->size()){
		printf("Rank %i is reducing memory usage [%lu/%lu] (completed)\n",parameters->getRank(),m_counter,forest->size());
	}else if(m_counter%20000==0){
		printf("Rank %i is reducing memory usage [%lu/%lu]\n",parameters->getRank(),m_counter+1,forest->size());
	}
}

vector<Kmer >*MemoryConsumptionReducer::getVerticesToRemove(){
	return m_toRemove;
}

void MemoryConsumptionReducer::constructor(int size,Parameters*parameters){
	m_parameters=parameters;
	m_bufferedData.constructor(size,MAXIMUM_MESSAGE_SIZE_IN_BYTES/sizeof(uint64_t),
		RAY_MALLOC_TYPE_MEMORY_REDUCER_BUFFERS,false);
	m_pendingMessages=0;
}

void MemoryConsumptionReducer::processConfetti(uint64_t*a,int b){
	for(int i=0;i<b;i+=2){
		int task=a[i+0];
		int coverage=a[i+1];
		if(m_processedTasks->count(task)>0){
			continue;
		}
		m_processedTasks->insert(task);

		#ifdef ASSERT
		assert(task<(int)m_confettiToCheck->size());
		assert(task<(int)m_confettiMaxCoverage->size());
		#endif

		vector<Kmer>*vertices=&(*m_confettiToCheck)[task];
		int maxCoverage=(*m_confettiMaxCoverage)[task];
		if(coverage<maxCoverage){
			continue;
		}
		#ifdef ASSERT
		assert(vertices->size()>0);
		#endif
		for(int j=0;j<(int)vertices->size();j++){
			Kmer vertex=vertices->at(j);
			m_toRemove->push_back(vertex);
		}
	}
	m_pendingMessages--;
	#ifdef ASSERT
	assert(m_pendingMessages>=0);
	#endif
}

map<Kmer ,vector<Kmer > >*MemoryConsumptionReducer::getIngoingEdges(){
	return m_ingoingEdges;
}

map<Kmer ,vector<Kmer > >*MemoryConsumptionReducer::getOutgoingEdges(){
	return m_outgoingEdges;
}

void MemoryConsumptionReducer::destructor(){
	delete m_dfsDataOutgoing;
	delete m_toRemove;
	delete m_ingoingEdges;
	delete m_outgoingEdges;
	delete m_processedTasks;
	delete m_confettiToCheck;
	delete m_confettiMaxCoverage;
}

void MemoryConsumptionReducer::constructor(){
	m_dfsDataOutgoing=new DepthFirstSearchData;
	m_toRemove=new vector<Kmer >;
	m_ingoingEdges=new map<Kmer ,vector<Kmer > >;
	m_outgoingEdges=new map<Kmer ,vector<Kmer > >;
	m_processedTasks=new set<int>;
	m_confettiToCheck=new vector<vector<Kmer> >;
	m_confettiMaxCoverage=new vector<int>;
}
