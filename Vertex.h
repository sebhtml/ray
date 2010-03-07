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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/






#ifndef _Vertex

#define _Vertex
#include<ReadAnnotation.h>
#include<MyAllocator.h>
#include<Direction.h>
#include<stdint.h>
#include<vector>
using namespace std;

#define COVERAGE_TYPE unsigned char

class Vertex{
	COVERAGE_TYPE m_coverage;
	// 
	// outgoing  ingoing
	//
	// G C T A G C T A
	//
	// 7 6 5 4 3 2 1 0
	char m_edges;

	ReadAnnotation*m_readsStartingHere;
	Direction*m_direction;
public:
	void constructor();
	void setCoverage(int coverage);
	int getCoverage();
	void addOutgoingEdge(uint64_t a,int k);
	void addIngoingEdge(uint64_t a,int k);
	void addRead(int rank,int i,char c,MyAllocator*allocator);
	bool isAssembled();
	void assemble();
	vector<uint64_t> getIngoingEdges(uint64_t a,int k);
	vector<uint64_t> getOutgoingEdges(uint64_t a,int k);
	ReadAnnotation*getReads();
	void addDirection(int wave,int progression,MyAllocator*a);
	vector<Direction> getDirections();
	void clearDirections();
};

#endif
