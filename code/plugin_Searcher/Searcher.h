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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>
*/

#ifndef _Searcher_h
#define _Searcher_h

#include <stdint.h>
#include <fstream>
#include <stdint.h> /* for uint64_t */
#include <stdio.h> /*for FILE */
#include <vector>
#include <string>
#include <map>
#include <plugin_VerticesExtractor/GridTable.h>
using namespace std;

#include <application_core/Parameters.h>
#include <scheduling/SwitchMan.h>
#include <communication/VirtualCommunicator.h>
#include <communication/BufferedData.h>
#include <memory/RingAllocator.h>
#include <plugin_Searcher/SearchDirectory.h>
#include <plugin_Searcher/ContigSearchEntry.h>
#include <structures/StaticVector.h>
#include <profiling/TimePrinter.h>
#include <profiling/Derivative.h>
#include <handlers/SlaveModeHandler.h>
#include <handlers/MasterModeHandler.h>
#include <handlers/MessageTagHandler.h>
#include <plugin_Searcher/ContigHit.h>
#include <plugin_Searcher/ColorSet.h>
#include <plugins/CorePlugin.h>
#include <plugin_Searcher/QualityCaller.h>
#include <plugin_Searcher/Searcher_adapters.h>
#include <plugin_Searcher/DistributionWriter.h>

/**
 * This class searches for sequences in the de Bruijn graph
 * It outputs biological abundance readouts
 * \author Sébastien Boisvert
 **/
class Searcher :  public CorePlugin {

	QualityCaller m_caller;

	MessageTag RAY_MPI_TAG_ADD_COLORS;
	MessageTag RAY_MPI_TAG_ADD_KMER_COLOR_REPLY;
	MessageTag RAY_MPI_TAG_CONTIG_BIOLOGICAL_ABUNDANCES;
	MessageTag RAY_MPI_TAG_SEQUENCE_BIOLOGICAL_ABUNDANCES;
	MessageTag RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS;

	MessageTag RAY_MPI_TAG_ADD_KMER_COLOR;
	MessageTag RAY_MPI_TAG_CONTIG_ABUNDANCE;
	MessageTag RAY_MPI_TAG_CONTIG_ABUNDANCE_REPLY;
	MessageTag RAY_MPI_TAG_CONTIG_IDENTIFICATION;
	MessageTag RAY_MPI_TAG_CONTIG_IDENTIFICATION_REPLY;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_PATHS;
	MessageTag RAY_MPI_TAG_GET_COVERAGE_AND_PATHS_REPLY;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE;
	MessageTag RAY_MPI_TAG_REQUEST_VERTEX_COVERAGE_REPLY;
	MessageTag RAY_MPI_TAG_SEARCH_COUNTING_DONE;
	MessageTag RAY_MPI_TAG_SEARCH_ELEMENTS;
	MessageTag RAY_MPI_TAG_SEARCH_ELEMENTS_REPLY;
	MessageTag RAY_MPI_TAG_SEARCH_MASTER_COUNT;
	MessageTag RAY_MPI_TAG_SEARCH_MASTER_COUNT_REPLY;
	MessageTag RAY_MPI_TAG_SEARCH_MASTER_SHARING_DONE;
	MessageTag RAY_MPI_TAG_SEARCH_SHARE_COUNTS;
	MessageTag RAY_MPI_TAG_SEARCH_SHARING_COMPLETED;
	MessageTag RAY_MPI_TAG_SEQUENCE_ABUNDANCE_FINISHED;
	MessageTag RAY_MPI_TAG_SEQUENCE_ABUNDANCE_YOU_CAN_GO_HOME;
	MessageTag RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_REPLY;
	MessageTag RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCES;
	MessageTag RAY_MPI_TAG_SEARCHER_CLOSE;
	MessageTag RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY;
	MessageTag RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY_REPLY;

	MasterMode RAY_MASTER_MODE_KILL_RANKS;
	MasterMode RAY_MASTER_MODE_ADD_COLORS;
	MasterMode RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS;
	MasterMode RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES;
	MasterMode RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES;
	MasterMode RAY_MASTER_MODE_SEARCHER_CLOSE;
	MasterMode RAY_MASTER_MODE_PHYLOGENY_MAIN;

	SlaveMode RAY_SLAVE_MODE_ADD_COLORS;
	SlaveMode RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES;
	SlaveMode RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES;
	SlaveMode RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS;
	SlaveMode RAY_SLAVE_MODE_SEARCHER_CLOSE;

	Adapter_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS m_adapter_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS;
	Adapter_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES m_adapter_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES;
	Adapter_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES m_adapter_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES;
	Adapter_RAY_SLAVE_MODE_ADD_COLORS m_adapter_RAY_SLAVE_MODE_ADD_COLORS;
	Adapter_RAY_SLAVE_MODE_SEARCHER_CLOSE m_adapter_RAY_SLAVE_MODE_SEARCHER_CLOSE;

	Adapter_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS m_adapter_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS;
	Adapter_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES m_adapter_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES;
	Adapter_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES m_adapter_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES;
	Adapter_RAY_MASTER_MODE_ADD_COLORS m_adapter_RAY_MASTER_MODE_ADD_COLORS;
	Adapter_RAY_MASTER_MODE_SEARCHER_CLOSE m_adapter_RAY_MASTER_MODE_SEARCHER_CLOSE;

	Adapter_RAY_MPI_TAG_ADD_KMER_COLOR m_adapter_RAY_MPI_TAG_ADD_KMER_COLOR;
	Adapter_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS m_adapter_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS;
	Adapter_RAY_MPI_TAG_CONTIG_IDENTIFICATION m_adapter_RAY_MPI_TAG_CONTIG_IDENTIFICATION;
	Adapter_RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY m_adapter_RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY;

/** translator for virtual colors **/
	ColorSet m_colorSet;

/** an object that writes files */
	DistributionWriter m_writer;

	uint64_t m_processedFiles;
	uint64_t m_processedSequences;

	uint64_t m_sequencesToProcessInFile;
	uint64_t m_sequencesToProcess;
	uint64_t m_filesToProcess;

	uint64_t m_totalNumberOfKmerObservations;
	
	PhysicalKmerColor m_color;
	PhysicalKmerColor m_identifier;

	bool m_closedFiles;

	/** indicates if colors must be added for this sequence. **/
	bool m_mustAddColors;

	bool m_colorSequenceKmersSlaveStarted;

	/** the part of the de Bruijn graph that the current ranks owns **/
	GridTable*m_subgraph;

	bool m_startedColors;

	/** number of pending messages */
	int m_pendingMessages;
	
	int m_numberOfRanksThatFinishedSequenceAbundances;

	/** total number of kmers processed */
	uint64_t m_kmersProcessed;

	/** manually buffered data */
	BufferedData m_bufferedData;

	/** point-wise derivative for speed readouts */
	Derivative m_derivative;

	/** the Ray parameters */
	Parameters*m_parameters;

	/** the switchman */
	SwitchMan*m_switchMan;
	VirtualCommunicator*m_virtualCommunicator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	RingAllocator*m_outboxAllocator;
	TimePrinter*m_timePrinter;

	/** counts for each contig for the current sequence
 * being processed
 *
 * map<char, map<contig, set<position > > > */
	map<char, map<uint64_t,set<int> > > m_contigCounts;

/** the set of observed paths for the current position */
	map<int,set<uint64_t > > m_observedPaths;

	// state of the machine
	bool m_checkedHits;

	map<uint64_t,int> m_contigLengths;

	// iterators for hits
	vector<ContigHit> m_sortedHits;

	vector<ContigHit>::iterator m_sortedHitsIterator;

	vector<ContigSearchEntry> m_listOfContigEntries;

	/** the identifier to give to the virtual communicator */
	uint64_t m_workerId;

	// for counting entries in category files
	bool m_countElementsMasterStarted;
	bool m_countElementsSlaveStarted;

	// for counting stuff in contigs
	bool m_countContigKmersSlaveStarted;
	bool m_countContigKmersMasterStarted;

	// for counting sequences
	bool m_listedDirectories;
	bool m_countedDirectories;
	bool m_shareCounts;
	bool m_sharedCounts;
	int m_directoryIterator;
	int m_fileIterator;
	bool m_waiting;

	/** option that indicates if detailed reports are needed */
	bool m_writeDetailedFiles;

	/** contig paths */
	vector<vector<Kmer> >*m_contigs;
	vector<uint64_t>*m_contigNames;

	// synchronization
	int m_ranksDoneCounting;
	int m_ranksDoneSharing;
	int m_ranksSynced;
	bool m_synchronizationIsDone;

	int m_masterDirectoryIterator;
	int m_masterFileIterator;
	bool m_sendCounts;

	/** contig iterator */
	int m_contig;

	/** contig position iterator */
	int m_contigPosition;

	/** only used by master, tells which rank to call next */
	int m_rankToCall;

	/** indicates if the rank has sent a control message already */
	bool m_finished;

	// for the virtual communicator
	vector<uint64_t> m_activeWorkers;

	/** file to write coverage distribution */
	ofstream m_currentCoverageFile;
	bool m_waitingForAbundanceReply;

	map<int,uint64_t> m_coverageDistribution;
	map<int,uint64_t> m_coloredCoverageDistribution;
	map<int,uint64_t> m_coloredAssembledCoverageDistribution;

	/** search directory objects */
	SearchDirectory*m_searchDirectories;
	int m_searchDirectories_size;

	/** for the master rank */
	map<int,FILE*> m_identificationFiles;

	// base names of directories
	vector<string> m_directoryNames;

/** for a sequence, indicates if its abundance was processed. */
	bool m_processedAbundance;

	// base names of files
	vector<vector<string> > m_fileNames;

	/** number of matches for a sequence */
	int m_matches;
	int m_coloredMatches;
	int m_coloredAssembledMatches;

	/** k-mer length */
	int m_kmerLength;

	/** flag */
	bool m_requestedCoverage;

	int m_numberOfKmers;

	/** files to write */
	map<int,FILE* > m_arrayOfFiles;

	/** track the descriptors */
	int m_activeFiles;

	/** partition */
	int m_firstSequence;
	int m_lastSequence;

	/** states for sequence abundances */
	bool m_countSequenceKmersSlaveStarted;
	bool m_countSequenceKmersMasterStarted;

	/** state */
	bool m_createdSequenceReader;

	/** iteraetor */
	int m_sequenceIterator;
	int m_globalSequenceIterator;
	int m_globalFileIterator;

	int m_currentLength;


//_-----------------_//


	void showContigAbundanceProgress();
	void createTrees();

	uint64_t m_lastPrinted;

	bool isFileOwner(int globalFile);

	void printDirectoryStart();

	string getBaseName(string a);

	void showProcessedKmers();

	int getDistributionMode(map<int,uint64_t>*distribution);

	void dumpDistributions();

	int getWriter(int directory);

	int getAbundanceWriter(int directory);

	void addColorToKmer(Vertex*node,PhysicalKmerColor color);

public:

	void call_RAY_MASTER_MODE_COUNT_SEARCH_ELEMENTS();
	void call_RAY_SLAVE_MODE_COUNT_SEARCH_ELEMENTS();

	void call_RAY_MASTER_MODE_CONTIG_BIOLOGICAL_ABUNDANCES();
	void call_RAY_SLAVE_MODE_CONTIG_BIOLOGICAL_ABUNDANCES();

	void call_RAY_MASTER_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES();
	void call_RAY_SLAVE_MODE_SEQUENCE_BIOLOGICAL_ABUNDANCES();
	
	void call_RAY_MASTER_MODE_ADD_COLORS();
	void call_RAY_SLAVE_MODE_ADD_COLORS();

	void call_RAY_MASTER_MODE_SEARCHER_CLOSE();
	void call_RAY_SLAVE_MODE_SEARCHER_CLOSE();

	void call_RAY_MPI_TAG_ADD_KMER_COLOR(Message*message);
	void call_RAY_MPI_TAG_CONTIG_IDENTIFICATION(Message*message);

	void constructor(Parameters*parameters,StaticVector*outbox,TimePrinter*timePrinter,SwitchMan*switchMan,
	VirtualCommunicator*m_vc,StaticVector*inbox,RingAllocator*outboxAllocator,
		GridTable*graph);

	void setContigs(vector<vector<Kmer> >*paths,vector<uint64_t>*names);

	void call_RAY_MPI_TAG_GET_COVERAGE_AND_PATHS(Message*message);
	void call_RAY_MPI_TAG_WRITE_SEQUENCE_ABUNDANCE_ENTRY(Message*message);

	void registerPlugin(ComputeCore*core);
	void resolveSymbols(ComputeCore*core);

}; /* Searcher */


#endif

