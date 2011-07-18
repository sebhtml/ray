/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#ifndef _Partitioner_H
#define _Partitioner_H

#include <structures/StaticVector.h>
#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <assembler/Loader.h>
#include <map>
using namespace std;

/**
 * This class counts the number of entries in each input file in parallel 
 */
class Partitioner{
	/** the loader */
	Loader m_loader;
	/** did we initialize the master peer */
	bool m_initiatedMaster;
	/** did we initialize the slave peer */
	bool m_initiatedSlave;
	/** the number of peers that have finished sending  counts */
	int m_ranksDoneSending;
	/** the number of peers that have finished counting files */
	int m_ranksDoneCounting;
	/** did the count was sent ?*/
	bool m_sentCount;
	/** the current file to send */
	int m_currentFileToSend;
	/** the current file to count */
	int m_currentFileToCount;
	/** are we currently sending counts ?*/
	bool m_currentlySendingCounts;
	/** counts for a peer slave */
	map<int,uint64_t> m_slaveCounts;
	/** counts for the master node */
	map<int,uint64_t> m_masterCounts;

	/** --- Below are the only elements necessary to bind Partitioner to the Ray software stack */
	/** master mode */
	int*m_masterMode;
	/** slave mode */
	int*m_slaveMode;
	/** allocator for outgoing messages */
	RingAllocator*m_outboxAllocator;
	/** message inbox */
	StaticVector*m_inbox;
	/** message outbox */
	StaticVector*m_outbox;
	/** parameters */
	Parameters*m_parameters;

	/** --- */

public:
	void constructor(RingAllocator*outboxAllocator,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,
	int*slaveMode,int*masterMode);
	void masterMethod();
	void slaveMethod();
};

#endif
