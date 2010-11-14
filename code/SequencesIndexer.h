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
#include<vector>
#include<MyAllocator.h>
#include<Read.h>
#include<DistributionData.h>
#include<Parameters.h>

class SequencesIndexer{
public:
	void attachReads(std::vector<Message>*m_outbox,int*m_distribution_file_id,int*m_distribution_sequence_id,
	int*m_wordSize,vector<Read*>*m_distribution_reads,int size,MyAllocator*m_distributionAllocator,
	int*m_distribution_currentSequenceId,int rank,DistributionData*m_disData,bool*m_mode_AttachSequences,
	Parameters*m_parameters,bool*m_colorSpaceMode,MyAllocator*m_outboxAllocator,time_t*m_lastTime
);
	void flushAttachedSequences(int threshold,std::vector<Message>*m_outbox,int rank,int size,DistributionData*m_disData,
	MyAllocator*m_outboxAllocator);
};

#endif
