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
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _EdgesExtractor
#define _EdgesExtractor

#include<DistributionData.h>
#include<Message.h>
#include<MyAllocator.h>
#include<vector>
#include<Read.h>
using namespace std;

class EdgesExtractor{
public:
	vector<Read*>*m_myReads;
	bool m_reverseComplementEdge;
	DistributionData*m_disData;
	int getRank;
	int getSize;
	MyAllocator*m_outboxAllocator;
	vector<Message>*m_outbox;
	int m_mode_send_edge_sequence_id;
	int m_mode_send_edge_sequence_id_position;
	bool*m_mode_send_outgoing_edges;
	bool*m_mode_send_ingoing_edges;
	int m_wordSize;
	bool m_colorSpaceMode;
	int*m_mode;

	void processOutgoingEdges();
	void processIngoingEdges();
	void flushOutgoingEdges(int threshold);
	void flushIngoingEdges(int threshold);
	EdgesExtractor();
};

#endif
