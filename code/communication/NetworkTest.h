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

#ifndef _NetworkTest_H
#define _NetworkTest_H

#include <structures/StaticVector.h>
#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <string>
#include <map>
using namespace std;

#ifdef OS_POSIX
#include <sys/time.h>
#endif

/**
 * This class tests the network
 * Tested elements:
 *
 *     - latency to get a response for a message
 *
 *     if OS_POSIX is not defined, the network test is not done because the code uses POSIX gettimeofday()
 *
 *     Dependencies from the Ray software stack (mostly):
 *
 *          - message inbox
 *          - message outbox
 *          - outbox memory ring allocator
 *          - slave and master modes.
 */
class NetworkTest{
	/** the message inbox */
	StaticVector*m_inbox;
	/** the message outbox */
	StaticVector*m_outbox;
	/** parameter object */
	Parameters*m_parameters;
	/** the slave mode */
	int*m_slaveMode;
	/** the master mode, always RAY_SLAVE_MODE_DO_NOTHING for rank >0 */
	int*m_masterMode;
	/** message-passing interface rank */
	int m_rank;
	/** number of ranks */
	int m_size;
	/** number of ranks who finished the tests */
	int m_doneWithNetworkTest;
	/* initialised this ? */
	bool m_initialisedNetworkTest;
	/** outbox allocator */
	RingAllocator*m_outboxAllocator;
	/** the number of test message to send */
	int m_numberOfTestMessages;
	/** the current test message */
	int m_currentTestMessage;
	/** the current test message has been sent ? */
	bool m_sentCurrentTestMessage;

	#ifdef OS_POSIX
	/** the starting time */
	struct timeval m_startingTime;
	#endif

	uint64_t m_sumOfMicroSeconds;

	/** latencies */
	map<int,int> m_latencies;
	map<int,string> m_names;

	/* processor name */
	string*m_name;
public:
	/** initialize the NetworkTest */
	void constructor(int rank,int*masterMode,int*slaveMode,int size,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,RingAllocator*outboxAllocator,
		string*name);
	/** work method for the master mode */
	void masterWork();
	/** work method for the slave mode */
	void slaveWork();
};

#endif

