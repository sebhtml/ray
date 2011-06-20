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

#ifndef _Scaffolder
#define _Scaffolder

#include <core/Parameters.h>
#include <structures/StaticVector.h>
#include <vector>
#include <memory/RingAllocator.h>
#include <core/constants.h>
#include <communication/VirtualCommunicator.h>
#include <assembler/ReadFetcher.h>
#include <sstream>
using namespace std;

/**
 * Scaffolder class, it uses MPI through the virtual communicator.
 *
 * \author Sébastien Boisvert
 */
class Scaffolder{
	int m_rankIdForContig;
	bool m_hasContigSequence_Initialised;
	FILE*m_fp;
	bool m_hasContigSequence;
	string m_contigSequence;
	map<uint64_t,int> m_contigLengths;
	int m_position;
	int m_theLength;
	vector<Kmer> m_contigPath;
	bool m_requestedContigChunk;
	int m_numberOfScaffolds;
	int m_numberOfScaffoldsWithThreshold;
	int m_numberOfContigs;
	int m_numberOfContigsWithAtLeastThreshold;
	uint64_t m_totalContigLength;
	uint64_t m_totalContigLengthWithThreshold;
	uint64_t m_totalScaffoldLength;
	uint64_t m_totalScaffoldLengthWithThreshold;

	int m_scaffoldId;
	int m_positionOnScaffold;
	bool m_writeContigRequested;

	vector<vector<uint64_t> > m_scaffoldContigs;
	vector<vector<char> >m_scaffoldStrands;
	vector<vector<int> >m_scaffoldGaps;
	bool m_sentContigInfo;
	bool m_sentContigMeta;
	vector<uint64_t> m_masterContigs;
	vector<int> m_masterLengths;
	vector<vector<uint64_t> >m_masterLinks;
	int m_summaryIterator;
	bool m_summarySent;
	vector<vector<uint64_t> >m_summary;
	bool m_summaryPerformed;
	bool m_entrySent;
	map<uint64_t,map<char,map<uint64_t,map<char,vector<int> > > > > m_scaffoldingSummary;
	bool m_reverseDirectionsReceived;
	bool m_reverseDirectionLengthReceived;
	uint64_t m_pairedReverseDirectionName;
	int m_pairedReverseDirectionPosition;
	int m_pairedReverseMarkerCoverage;
	bool m_pairedReverseHasDirection;
	bool m_reverseDirectionLengthRequested;
	int m_pairedReverseDirectionLength;
	// these are mostly flags for sends/receives
	bool m_forwardDirectionLengthRequested;
	bool m_forwardDirectionLengthReceived;
	int m_pairedForwardDirectionLength;
	bool m_forwardDirectionsRequested;
	bool m_forwardDirectionsReceived;
	int m_pairedForwardMarkerCoverage;
	bool m_pairedForwardHasDirection;
	uint64_t m_pairedForwardDirectionName;
	int m_pairedForwardDirectionPosition;
	bool m_reverseDirectionsRequested;
	int m_readLength;
	bool m_markersRequested;
	bool m_markersReceived;
	int m_pairedForwardOffset;
	int m_pairedReverseOffset;
	Kmer m_pairedForwardMarker;
	Kmer m_pairedReverseMarker;
	bool m_pairReceived;
	int m_pairedReadRank;
	int m_pairedReadIndex;
	int m_pairedReadLibrary;
	int m_pairedReadLength;
	bool m_pairRequested;
	bool m_hasPairReceived;
	bool m_hasPairRequested;
	bool m_hasPair;
	bool m_initialisedFetcher;
	int m_readAnnotationId;
	ReadFetcher m_readFetcher;
	vector<uint64_t> m_activeWorkers;
	uint64_t m_workerId;
	VirtualCommunicator*m_virtualCommunicator;
	bool m_coverageRequested;
	bool m_coverageReceived;
	int m_receivedCoverage;

	bool m_reverseDone;
	bool m_forwardDone;
	int m_contigId;
	int m_positionOnContig;
	vector<vector<Kmer> > m_contigs;
	vector<uint64_t> m_contigNames;

	Parameters*m_parameters;
	StaticVector*m_inbox;
	StaticVector*m_outbox;
	RingAllocator*m_outboxAllocator;
	int*m_slave_mode;
	bool m_ready;

	/**
 *	gets a contig sequence by receiving several MPI messages
 */
	void getContigSequence(uint64_t id);
	void processContig();
	void processContigPosition();
	void processVertex(Kmer vertex);
	void processAnnotations();
	void processAnnotation();
	void performSummary();
	void sendSummary();
	void sendContigInfo();
	void extractScaffolds(char state,map<uint64_t,int>*colors,uint64_t vertex,
		map<uint64_t,map<char,vector<vector<uint64_t> > > >*parents,
		map<uint64_t,map<char,vector<vector<uint64_t> > > >*children,set<int>*completedColours);

public:

	bool m_initialised;
	/**
 *	Number of ranks that have finished scaffolding
 */
	int m_numberOfRanksFinished;

	/**
 *	Constructor of the scaffolder
 */
	void constructor(StaticVector*outbox,StaticVector*inbox,RingAllocator*outboxAllocator,Parameters*parameters,
		int*slaveMode,VirtualCommunicator*vc);
	void run();
	void addContig(uint64_t name,vector<Kmer>*vertices);
	void addMasterLink(vector<uint64_t>*link);
	void solve();
	void addMasterContig(uint64_t name,int length);
	void writeScaffolds();
	void printFinalMessage();
};

#endif

