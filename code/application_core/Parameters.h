/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _Parameters
#define _Parameters

#include<map>
#include<application_core/common_functions.h>
#include<set>
#include<string>
#include<vector>
using namespace std;

/**
 * the default number of buckets in the
 * distributed hash table, for a single agent
 */
#define __DEFAULT_BUCKETS 268435456 // old value: 1048576

/**
 * The threshold for triggering incremental resizing
 * of the distributed hash table, valid for a single
 * agent
 */
#define __DEFAULT_LOAD_FACTOR_THRESHOLD 0.75 /* Like Java HashMap */

/**
 * The number of buckets per group in the distributed
 * hash table. The table uses a sparse model.
 * This value can not exceed 64 because bits are stored
 * with a uint64_t 
 */
#define __DEFAULT_BUCKETS_PER_GROUP 64

// 8 bytes
/*
 * The default number of bits in the Bloom filter.
 */
#define __BLOOM_DEFAULT_BITS 268435456 //64000000

/**
 * This class is the implementation of an interpreter for the RayInputFile.
 * It allows the following commands:
 * 	LoadSingleEndReads <FileA>
 * 	LoadPairedEndReads <FileLeft> <FileRight> <FragmentLength> <StandardDeviation>
 *
 * \author Sébastien Boisvert
 */
class Parameters{
	

	string m_configurationContent;

	void writeConfigurationFile();

	CoverageDepth m_maximumSeedCoverage;

	string m_genomeToTaxonFile;
	string m_treeFile;
	string m_taxonNameFile;

	vector<string> m_searchDirectories;

	int m_degree;

	string m_connectionType;

	bool m_showReadPlacement;
	bool m_showCommunicationEvents;
	set<string> m_options;
	bool m_providedPeakCoverage;
	bool m_providedMinimumCoverage;
	bool m_providedRepeatCoverage;
	int*m_slaveMode;
	int*m_masterMode;
	bool m_showMemoryUsage;
	bool m_showEndingContext;
	bool m_debugSeeds;
	bool m_profiler;
	bool m_debugBubbles;
	bool m_showMemoryAllocations;
	bool m_writeKmers;
	int m_reducerPeriod;
	int m_maximumDistance;

	string m_memoryFilePrefix;

	bool m_reducerIsActivated;
	int m_size;
	CoverageDepth m_repeatCoverage;
	Rank m_rank;
	CoverageDepth m_peakCoverage;

	map<int,vector<int> > m_libraryAverageLength;
	map<int,vector<int> > m_libraryDeviation;
	int m_numberOfLibraries;
	CoverageDepth m_minimumCoverage;
	bool m_error;
	string m_prefix;
	bool m_amos;
	bool m_initiated;
	vector<string> m_singleEndReadsFile;
	map<int,LargeCount> m_numberOfSequencesInFile;
	LargeCount m_totalNumberOfSequences;
	map<int,int> m_fileLibrary;
	vector<vector<int> > m_libraryFiles;
	set<int> m_automaticLibraries;

	string m_directory;
	string m_outputFile;
	int m_wordSize;
	int m_minimumContigLength;
	set<int> m_leftFiles;
	set<int> m_rightFiles;
	set<int> m_interleavedFiles;
	CoverageDepth m_seedCoverage;
	bool m_colorSpaceMode;
	string m_input;
	vector<string> m_commands;
	vector<string> m_originalCommands;
	map<int,map<int,int> >  m_observedDistances;
	vector<int> m_observedAverageDistances;
	vector<int> m_observedStandardDeviations;
	void loadCommandsFromArguments(int argc,char**argv);
	void loadCommandsFromFile(char*file);
	void parseCommands();

	string getLibraryFile(int library);
	void showOption(string a,string b);
	void showOptionDescription(string a);
	void fileNameHook(string f);

	bool m_showExtensionChoice;

	void getIndexes(int count,vector<int>*out);

	string getBaseName(string directory);
	bool isDirectorySeparator(char a);

	string m_checkpointDirectory;
	bool m_hasCheckpointDirectory;

	void __shuffleOperationCodes();

	bool isValidInteger(const char*textMessage);
public:
	Parameters();
	string getReceivedMessagesFile();
	void constructor(int argc,char**argv,Rank rank,int size);
	bool isInitiated();
	vector<string> getAllFiles();
	string getDirectory();
	int getMinimumContigLength();
	string getOutputFile();
	int getWordSize();
	int getLibraryAverageLength(int i,int peak);
	int getLibraryStandardDeviation(int i,int peak);
	bool isLeftFile(int i);
	bool isRightFile(int i);
	bool getColorSpaceMode();
	bool useAmos();
	string getInputFile();
	string getAmosFile();
	string getParametersFile();
	string getCoverageDistributionFile();
	vector<string> getCommands();
	bool getError();
	void addDistance(int library,int distance,int count);
	void computeAverageDistances();
	LargeCount getNumberOfSequences(int n);
	void setNumberOfSequences(int file,LargeCount n);
	int getNumberOfFiles();
	bool isAutomatic(int library);
	int getLibrary(int file);
	void printFinalMessage();
	bool isInterleavedFile(int i);
	void setPeakCoverage(CoverageDepth a);
	CoverageDepth getPeakCoverage();
	void addLibraryData(int library,int average,int deviation);
	int getNumberOfLibraries();
	CoverageDepth  getRepeatCoverage();
	string getPrefix();
	int getRank();
	int getSize();
	void setSize(int a);
	bool runReducer();
	void showUsage();
	int getReducerValue();

	Rank getRankFromGlobalId(uint64_t a);
	int getIdFromGlobalId(uint64_t a);
	int getMaximumDistance();
	uint64_t getGlobalIdFromRankAndLocalId(Rank rank,int id);
	CoverageDepth getMaximumAllowedCoverage();
	CoverageDepth getMinimumCoverage();
	void setMinimumCoverage(CoverageDepth a);
	Kmer _complementVertex(Kmer*a);
	bool hasPairedReads();
	int _vertexRank(Kmer*a);
	string getMemoryPrefix();
	/**
	* run the profiler
	*/
	bool runProfiler();
	/**
	* debug the code for bubble detection
	*/
	bool debugBubbles();
	/** 
	* debug the code that computes seeds
	*/
	bool debugSeeds();
	bool showMemoryUsage();
	bool showEndingContext();

	string getScaffoldFile();
	int getColumns();
	int getLargeContigThreshold();
	void setRepeatCoverage(CoverageDepth a);
	bool showMemoryAllocations();
	bool writeKmers();
	CoverageDepth getMinimumCoverageToStore();

	int getLibraryMaxAverageLength(int i);
	int getLibraryMaxStandardDeviation(int i);
	int getLibraryPeaks(int library);
	string getFile(int file);
	bool showExtensionChoice();
	bool hasOption(string a);
	bool hasCheckpoint(const char*checkpointName);

	/** get the checkpoint file */
	string getCheckpointFile(const char*a);

	/** true if file exists */
	bool hasFile(const char*file);
	bool writeCheckpoints();
	bool readCheckpoints();

	void writeCommandFile();
	bool showCommunicationEvents();
	bool showReadPlacement();

	int getCoresPerNode();
	string getConnectionType();

	int getRoutingDegree();

	vector<string>*getSearchDirectories();

	string getGenomeToTaxonFile();
	string getTreeFile();
	string getTaxonNameFile();
	string getSampleName();

/** gets the maximum coverage depth allowed for seeds. **/
	CoverageDepth getMaximumSeedCoverage();

	uint64_t getNumberOfBuckets();
	int getNumberOfBucketsPerGroup();
	double getLoadFactorThreshold();


	uint64_t getConfigurationInteger(const char*string,int offset);
	double getConfigurationDouble(const char*string,int offset);


/**
 ** returns true if there is a command named <string> with exactly 
 ** <count> operands
 **
 ** Examples:
 **   -foo
 **           1 operator, 0 operand
 **
 **   -add 1 2
 **           1 operator, 2 operands
 **
 **   -malloc 2000
 **           1 operator, 1 operand
 **
 **/
	bool hasConfigurationOption(const char*string,int count);

	const char*getConfigurationString(const char*string,int offset);
};

#endif

