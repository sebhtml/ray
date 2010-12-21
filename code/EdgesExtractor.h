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

#ifndef _EdgesExtractor
#define _EdgesExtractor

#include<Message.h>
#include<ArrayOfReads.h>
#include<BufferedData.h>
#include<StaticVector.h>
#include<RingAllocator.h>
#include<vector>
#include<Read.h>
using namespace std;

class EdgesExtractor{
	bool m_ready;

	BufferedData m_bufferedDataForVerifications;
	BufferedData m_bufferedDataForEdges;

	int m_last;

public:
	ArrayOfReads*m_myReads;
	bool m_reverseComplementEdge;
	int getRank;
	int getSize;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	int m_mode_send_edge_sequence_id;
	int m_mode_send_edge_sequence_id_position;
	bool*m_mode_send_outgoing_edges;
	bool*m_mode_send_ingoing_edges;
	int m_wordSize;
	bool m_colorSpaceMode;
	int*m_mode;


	void constructor(int size);
	void processOutgoingEdges();
	void processIngoingEdges();
	EdgesExtractor();
	void setReadiness();
	void receiveIngoingEdges(uint64_t*a,int b,bool force);
	void receiveOutgoingEdges(uint64_t*a,int b,bool force);
};

#endif
