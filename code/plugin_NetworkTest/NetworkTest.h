/*
 	Ray
    Copyright (C) 2011, 2012  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#ifndef _NetworkTest_H
#define _NetworkTest_H

#include <structures/StaticVector.h>
#include <application_core/Parameters.h>
#include <profiling/TimePrinter.h>
#include <memory/RingAllocator.h>
#include <scheduling/SwitchMan.h>
#include <handlers/MasterModeHandler.h>
#include <handlers/SlaveModeHandler.h>
#include <core/ComputeCore.h>

#include <string>
#include <map>
using namespace std;



/**
 * This class tests the network
 * Tested elements:
 *
 *     - latency to get a response for a message
 *
 *     Dependencies from the Ray software stack (mostly):
 *
 *          - message inbox
 *          - message outbox
 *          - outbox memory ring allocator
 *          - slave and master modes.
 * \author Sébastien Boisvert
 */
class NetworkTest :  public CorePlugin {

	MessageTag RAY_MPI_TAG_TEST_NETWORK;
	MessageTag RAY_MPI_TAG_TEST_NETWORK_MESSAGE;
	MessageTag RAY_MPI_TAG_TEST_NETWORK_MESSAGE_REPLY;
	MessageTag RAY_MPI_TAG_TEST_NETWORK_REPLY;
	MessageTag RAY_MPI_TAG_TEST_NETWORK_REPLY_REPLY;
	MessageTag RAY_MPI_TAG_TEST_NETWORK_WRITE_DATA;

	MasterMode RAY_MASTER_MODE_KILL_ALL_MPI_RANKS;
	MasterMode  RAY_MASTER_MODE_TEST_NETWORK;
	MasterMode RAY_MASTER_MODE_COUNT_FILE_ENTRIES;

	SlaveMode RAY_SLAVE_MODE_TEST_NETWORK;


	SwitchMan*m_switchMan;
	
/** indicates if a rank got a response from master **/
	bool m_gotResponse;

	bool m_askedToWriteFiles;
	int m_ranksFinished;

/**
 * do we gather raw data
 */
	bool m_writeRawData;

	int m_messagesPerRank;

	vector<Rank> m_destinations;
	vector<uint64_t> m_sentMicroseconds;
	vector<uint64_t> m_receivedMicroseconds;

	/* number of words to use for network test */
	/* a word is 8 bytes */
	/* MAXIMUM_MESSAGE_SIZE_IN_BYTES is 4000 per default so 
		numberOfWords must be <= 500 */
	/* this is only for the network test */
	/* default is 500 */
	int m_numberOfWords;

	TimePrinter*m_timePrinter;
	/** the message inbox */
	StaticVector*m_inbox;
	/** the message outbox */
	StaticVector*m_outbox;
	/** parameter object */
	Parameters*m_parameters;
	/** the slave mode */
	int *m_slaveMode;
	/** the master mode, always RAY_SLAVE_MODE_DO_NOTHING for rank >0 */
	int *m_masterMode;

	bool m_sentData;

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

	bool m_started;

	/** latencies */
	map<int,int> m_modeLatencies;
	map<int,int> m_averageLatencies;

	map<int,string> m_names;

	/* processor name */
	string*m_name;

	int getModeLatency();
	int getAverageLatency();
public:
	/** initialize the NetworkTest */
	void constructor(Rank rank,int size,StaticVector*inbox,StaticVector*outbox,Parameters*parameters,RingAllocator*outboxAllocator,
		string*name,TimePrinter*timePrinter);
	/** work method for the master mode */
	void call_RAY_MASTER_MODE_TEST_NETWORK ();

	/** work method for the slave mode */
	void call_RAY_SLAVE_MODE_TEST_NETWORK();

	void writeData();

	void setSwitchMan(SwitchMan*a);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);
};

#endif

