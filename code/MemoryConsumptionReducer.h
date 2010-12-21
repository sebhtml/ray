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

#ifndef _MemoryConsumptionReducer
#define _MemoryConsumptionReducer

#include<MyForestIterator.h>
#include<MyForest.h>
#include<Parameters.h>
#include<RingAllocator.h>
#include<vector>
#include<DepthFirstSearchData.h>
#include<StaticVector.h>
using namespace std;

class MemoryConsumptionReducer{
	MyForestIterator m_iterator;
	bool m_initiated;

	bool m_currentVertexIsDone;
	bool m_hasSetVertex;
	bool m_doneWithOutgoingEdges;
	
	DepthFirstSearchData m_dfsDataOutgoing;
	SplayNode<uint64_t,Vertex>*m_firstVertex;

	vector<uint64_t> m_toRemove;

	bool isCandidate(SplayNode<uint64_t,Vertex>*node,int wordSize);
public:
	MemoryConsumptionReducer();
	/*
 	* returns true if done
 	*/
	bool reduce(MyForest*a,Parameters*parameters,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,vector<uint64_t>*receivedOutgoingEdges,
		int minimumCoverage,bool*edgesReceived
);

	int getNumberOfRemovedVertices();
};

#endif
