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

#ifndef _SequencesIndexer
#define _SequencesIndexer

#include<common_functions.h>
#include<Message.h>
#include<StaticVector.h>
#include<vector>
#include<MyAllocator.h>
#include<RingAllocator.h>
#include<Read.h>
#include<DistributionData.h>
#include<Parameters.h>

class SequencesIndexer{
	int m_theSequenceId;

public:
	void attachReads(
vector<Read*>*m_myReads,
				RingAllocator*m_outboxAllocator,
				StaticVector*m_outbox,
				int*m_mode,
				int m_wordSize,
				BufferedData*m_bufferedData,
				int m_size,
				int m_rank,
				bool m_colorSpaceMode
);

};

#endif
