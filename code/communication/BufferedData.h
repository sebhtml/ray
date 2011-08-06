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

#ifndef _BufferedData
#define _BufferedData

#include <stdint.h>
#include <core/common_functions.h>
#include <memory/MyAllocator.h>
#include <structures/StaticVector.h>
#include <memory/RingAllocator.h>

/*
 *  This class accumulates messages and flush them when the threshold is reached.
 *
 *  Messages are added in a periodic manner, and 4096 (-) something is the _real_ threshold.
 *  Currently, MAXIMUM_MESSAGE_SIZE_IN_BYTES is 4000. Therefore, it is assumed that 96 bytes 
 *  is an upper bound for the required space for a message's enveloppe.
 *
 *  for instance, if messages are of size 3 and the MAX_SIZE is 10,
 *  then you want the threshold to be 9, that is 10/3*3 because else
 *  at 9 it won't get flushed, and the next thing you know is that you are at 12,
 *  this is above the threshold and the RingAllocator does not allow that.
 */
class BufferedData{
	int m_count;
	int m_period;
	uint64_t m_flushedMessages;
	uint64_t m_pushedMessages;
	int m_type;
	int*m_sizes;
	int m_ranks;
	bool m_show;

	// the capacity is measured in uint64_t
	int m_capacity;
	uint64_t *m_data;
public:
	/**
 *	the is numberOfRanks MPI ranks, and messages have a capacity of capacity.
 *
	 the capacity is measured in uint64_t
 */
	void constructor(int numberOfRanks,int capacity,int type,bool show,int period);
/*
 * Get the number of appended elements for MPI rank i
 */
	int size(int i)const;
/**
 * Get the element j to send to MPI rank i
 */
	uint64_t getAt(int i,int j);
/**
 * add an element to send to MPI rank i.
 */
	void addAt(int i,uint64_t k);
/**
 * Clear output buffer for MPI rank i
 */
	void reset(int i);

	/**
 * return true if flushed something 
 *  The result is mainly utilized to wait for a reply to regulate the communication in order 
 *  to not exaust resources such as the RingAllocator.
 * */
	bool flush(int destination,int period,int tag,RingAllocator*outboxAllocator,StaticVector*outbox,int rank,bool force);

	/**
 *		returns the number of flushed devices.
 */
	int flushAll(int tag,RingAllocator*outboxAllocator,StaticVector*outbox,int rank);
	bool isEmpty()const;
	void clear();

/*
 * Returns true if any buffer is full.
 */
	bool needsFlushing(int destination,int period);
	void showStatistics(int rank);
};

#endif

