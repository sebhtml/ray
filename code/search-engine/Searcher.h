/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#ifndef _Searcher_h
#define _Searcher_h

#include <core/Parameters.h>
#include <scheduling/SwitchMan.h>
#include <communication/VirtualCommunicator.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <assembler/TimePrinter.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <map>
using namespace std;

/**
 * This class searches for sequences in the de Bruijn graph
 * It outputs biological abundance readouts
 **/
class Searcher{
	Parameters*m_parameters;
	SwitchMan*m_switchMan;
	VirtualCommunicator*m_virtualCommunicator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	RingAllocator*m_outboxAllocator;
	TimePrinter*m_timePrinter;

	/** the identifier to give to the virtual communicator */
	uint64_t m_workerId;

	vector<string> categoryFiles;

	// for counting entries in category files
	bool m_countElementsMasterStarted;
	bool m_countElementsSlaveStarted;

	// for counting stuff in contigs
	bool m_countContigKmersSlaveStarted;
	bool m_countContigKmersMasterStarted;

	vector<vector<Kmer> >*m_contigs;
	vector<uint64_t>*m_contigNames;

	int m_contig;
	int m_contigPosition;

	// for the virtual communicator
	
	vector<uint64_t> m_activeWorkers;

	ofstream m_currentCoverageFile;
	bool m_waitingForAbundanceReply;

	map<int,int> m_coverageDistribution;

	/** for the master rank */
	ofstream m_contigSummaryFile;
	ofstream m_identificationFile;

	/** flag */
	bool m_requestedCoverage;

	void showContigAbundanceProgress();
public:

	void countElements_masterMethod();
	void countElements_slaveMethod();

	void countContigKmers_masterHandler();
	void countContigKmers_slaveHandler();

	void constructor(Parameters*parameters,StaticVector*outbox,TimePrinter*timePrinter,SwitchMan*switchMan,
	VirtualCommunicator*m_vc,StaticVector*inbox,RingAllocator*outboxAllocator);

	void setContigs(vector<vector<Kmer> >*paths,vector<uint64_t>*names);
};

#endif

