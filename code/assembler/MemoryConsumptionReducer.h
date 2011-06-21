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

#ifndef _MemoryConsumptionReducer
#define _MemoryConsumptionReducer

#include <structures/MyForestIterator.h>
#include <structures/MyForest.h>
#include <graph/GridTable.h>
#include <graph/GridTableIterator.h>
#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <communication/BufferedData.h>
#include <assembler/DepthFirstSearchData.h>
#include <assembler/SeedingData.h>
#include <structures/StaticVector.h>
#include <set>
#include <vector>
using namespace std;

/*
 * This class is currently not used.
 * It implements strategies to minimise the
 * memory usage of the Ray assembler.
 */
class MemoryConsumptionReducer{
	int m_pendingMessages;
	set<int>*m_processedTasks;
	BufferedData m_bufferedData;
	Parameters*m_parameters;
	uint64_t m_counter;

	GridTableIterator m_iterator;
	bool m_initiated;

	bool m_currentVertexIsDone;
	bool m_hasSetVertex;
	bool m_doneWithOutgoingEdges;
	
	DepthFirstSearchData*m_dfsDataOutgoing;
	Kmer m_firstKey;
	Vertex*m_firstVertex;

	vector<Kmer>*m_toRemove;
	map<Kmer,vector<Kmer> >*m_ingoingEdges;
	map<Kmer,vector<Kmer> >*m_outgoingEdges;

	vector<vector<Kmer> >*m_confettiToCheck;
	vector<int>*m_confettiMaxCoverage;

	int m_maximumDepth;

	bool isCandidate(Kmer key,Vertex*node,int wordSize);
	
	void printCounter(Parameters*parameters,GridTable*a);

	bool isJunction(Kmer vertex,map<Kmer ,vector<Kmer> >*edges,int wordSize);
	vector<Kmer> computePath(map<Kmer,vector<Kmer> >*edges,Kmer start,Kmer end,set<Kmer >*visited);

	void getPermutations(Kmer kmer,int length,vector<Kmer>*output,int wordSize);

public:
	MemoryConsumptionReducer();
	/*
 	* returns true if done
 	*/
	bool reduce(GridTable*a,Parameters*parameters,
bool*edgesRequested,bool*vertexCoverageRequested,bool*vertexCoverageReceived,
	RingAllocator*outboxAllocator,int size,int theRank,StaticVector*outbox,
 int*receivedVertexCoverage,SeedingData*seedingData,
		int minimumCoverage,bool*edgesReceived
);

	int getNumberOfRemovedVertices();
	vector<Kmer>*getVerticesToRemove();

	void constructor(int size,Parameters*a);

	void processConfetti(uint64_t*a,int b);

	map<Kmer,vector<Kmer> >*getIngoingEdges();
	map<Kmer,vector<Kmer> >*getOutgoingEdges();

	void constructor();
	void destructor();
};

#endif
