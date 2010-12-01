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

#ifndef _VerticesExtractor
#define _VerticesExtractor

#include<StaticVector.h>
#include<BufferedData.h>
#include<vector>
#include<common_functions.h>
#include<ArrayOfReads.h>
#include<Message.h>
#include<RingAllocator.h>
#include<Read.h>
using namespace std;

class VerticesExtractor{
	bool m_ready;
	BufferedData m_bufferedData;
public:
	void constructor(int size);
	void process(int*m_mode_send_vertices_sequence_id,
				ArrayOfReads*m_myReads,
				bool*m_reverseComplementVertex,
				int*m_mode_send_vertices_sequence_id_position,
				int rank,
				StaticVector*m_outbox,
				bool*m_mode_send_vertices,
				int m_wordSize,
				int size,
				RingAllocator*m_outboxAllocator,
				bool m_colorSpaceMode,int*m_mode
			);
	void setReadiness();
};

#endif

