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

#ifndef _SequencesLoader
#define _SequencesLoader

#include<Parameters.h>
#include<RingAllocator.h>
#include<Loader.h>
#include<MyAllocator.h>
#include<BufferedData.h>
#include<StaticVector.h>
#include<vector>
#include<Message.h>
#include<Read.h>
#include<BubbleData.h>
#include<time.h>
using namespace std;

class SequencesLoader{
	BufferedData m_bufferedData;

	int m_lastPrintedId;
	int m_distribution_currentSequenceId;
	int m_distribution_file_id;
	int m_distribution_sequence_id;
	bool m_LOADER_isLeftFile;
	bool m_LOADER_isRightFile;
	int m_LOADER_numberOfSequencesInLeftFile;
	int m_LOADER_averageFragmentLength;
	int m_LOADER_deviation;

	Loader m_loader;


	vector<Read*>m_distribution_reads;

	bool m_isInterleavedFile;
	time_t m_last;
	int m_produced;
	int m_waitingNumber;

	char*m_buffers;
	int*m_entries;
	int*m_numberOfSequences;
	int*m_numberOfFlushedSequences;

	int m_size;

	bool m_send_sequences_done;

	void flushAll(RingAllocator*m_outboxAllocator,StaticVector*m_outbox);
	void flush(int rank,RingAllocator*m_outboxAllocator,StaticVector*m_outbox,bool forceNothing);
	void appendSequence(int rank,char*sequence);
	int getUsedSpace(int rank);
	int getSpaceLeft(int rank);

public:
	/**
 *	load sequences from disk, and distribute them over the network.
 */

	bool isReady();
	bool loadSequences(int rank,int size,StaticVector*m_outbox,
	RingAllocator*m_outboxAllocator,
	bool*m_loadSequenceStep,
	BubbleData*m_bubbleData,
	time_t*m_lastTime,
	Parameters*m_parameters,int*m_master_mode
	
);

	SequencesLoader();
	void setReadiness();
	void constructor(int size);
};
#endif
