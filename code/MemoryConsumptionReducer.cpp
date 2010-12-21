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
	if(!m_initiated){
		m_iterator.constructor(a);
		m_initiated=true;
		m_removedVertices=0;
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
			m_hasSetVertex=true;
			m_doneWithOutgoingEdges=false;
			m_dfsDataOutgoing.m_doChoice_tips_dfs_done=false;
			m_dfsDataOutgoing.m_doChoice_tips_dfs_initiated=false;
		}else if(!m_doneWithOutgoingEdges){
			uint64_t key=m_firstVertex->getKey();
			int wordSize=parameters->getWordSize();
			vector<uint64_t> parents=m_firstVertex->getValue()->getIngoingEdges(key,wordSize);
			vector<uint64_t> children=m_firstVertex->getValue()->getOutgoingEdges(key,wordSize);
			int maximumDepth=200;

			uint64_t next=0;
			if(children.size()==1){
				next=children[0];
			}else if(parents.size()==1){
				next=parents[0];
			}


			//m_doneWithOutgoingEdges=true;
			//return false;

			if((!(parents.size()==0
			&& children.size()==1))
			 &&
			(!(parents.size()==1
			&& children.size()==0))){
				// skip this one
				m_doneWithOutgoingEdges=true;
			}else if(!m_dfsDataOutgoing.m_doChoice_tips_dfs_done){
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
				if(parameters->getRank()==MASTER_RANK
				&& m_dfsDataOutgoing.m_maxDepthReached
				&& m_firstVertex->getValue()->getCoverage()==1){
					cout<<"root="<<idToWord(key,wordSize)<<endl;
					cout<<"Parents: "<<parents.size()<<endl;
					cout<<"Children: "<<children.size()<<endl;
					cout<<m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size()/2<<" edges."<<endl;
			
					if(children.size()==1){
						cout<<idToWord(key,wordSize)<<" ("<<m_firstVertex->getValue()->getCoverage()<<") -> "<<idToWord(next,wordSize)<<" ("<<m_dfsDataOutgoing.m_coverages[next]<<")"<<endl;
					}else{

						cout<<" -> "<<idToWord(next,wordSize)<<" ("<<m_dfsDataOutgoing.m_coverages[next]<<")"<<idToWord(key,wordSize)<<" ("<<m_firstVertex->getValue()->getCoverage()<<")"<<endl;
					}
					for(int j=0;j<(int)m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector.size();j+=2){
						uint64_t prefix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+0];
						uint64_t suffix=m_dfsDataOutgoing.m_depthFirstSearchVisitedVertices_vector[j+1];
						cout<<idToWord(prefix,wordSize)<<" ("<<m_dfsDataOutgoing.m_coverages[prefix]<<") -> "<<idToWord(suffix,wordSize)<<" ("<<m_dfsDataOutgoing.m_coverages[suffix]<<")"<<endl;
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
	return m_removedVertices;
}
