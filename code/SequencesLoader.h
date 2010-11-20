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

#ifndef _SequencesLoader
#define _SequencesLoader
#include<Parameters.h>
#include<DistributionData.h>
#include<RingAllocator.h>
#include<StaticVector.h>
#include<vector>
#include<Message.h>
#include<Read.h>
#include<BubbleData.h>
#include<time.h>
using namespace std;

class SequencesLoader{
	bool m_isInterleavedFile;
	bool m_ready;
	time_t m_last;
	int m_produced;

	char*m_buffers;
	int*m_entries;
	int m_size;


	void flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox);
	void flush(int rank,RingAllocator*m_outboxAllocator,StaticVector*m_outbox);
	void appendSequence(int rank,char*sequence);
	int getUsedSpace(int rank);
	int getSpaceLeft(int rank);

public:
	/**
 *	load sequences from disk, and distribute them over the network.
 */
	bool loadSequences(int rank,int size,vector<Read*>*m_distribution_reads,int*m_distribution_sequence_id,
	bool*m_LOADER_isLeftFile,StaticVector*m_outbox,int*m_distribution_file_id,
	MyAllocator*m_distributionAllocator,bool*m_LOADER_isRightFile,int*m_LOADER_averageFragmentLength,
	DistributionData*m_disData,int*m_LOADER_numberOfSequencesInLeftFile,RingAllocator*m_outboxAllocator,
	int*m_distribution_currentSequenceId,int*m_LOADER_deviation,bool*m_loadSequenceStep,
	BubbleData*m_bubbleData,
	time_t*m_lastTime,
	Parameters*m_parameters,int*m_master_mode
	
);

	SequencesLoader();
	void setReadiness();
	void constructor(int size);
};
#endif
