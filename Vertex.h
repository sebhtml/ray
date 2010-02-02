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
#include"Edge.h"
#include"MyAllocator.h"
#include<vector>
using namespace std;

class Vertex{
	char m_coverage;
	Edge*m_ingoingEdges;
	Edge*m_outgoingEdges;
	bool m_assembled;

	bool hasEdge(Edge*e,int rank,void*ptr);

	Edge*m_readsStartingHere;
public:
	void constructor();
	void setCoverage(int coverage);
	int getCoverage();
	void addOutgoingEdge(int rank,void*ptr,MyAllocator*allocator);
	void addIngoingEdge(int rank,void*ptr,MyAllocator*allocator);
	void addRead(int rank,void*ptr,MyAllocator*allocator);
	bool isAssembled();
	void assemble();
	Edge*getFirstIngoingEdge();
	Edge*getFirstOutgoingEdge();
};

#endif
