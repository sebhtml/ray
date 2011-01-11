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

#ifndef _Parameters

#define _Parameters



#include<map>
#include<common_functions.h>
#include<set>
#include<string>
#include<vector>
using namespace std;

/*
 * This class is the implementation of an interpreter for the RayInputFile.
 * It allows the following commands:
 * 	LoadSingleEndReads <FileA>
 * 	LoadPairedEndReads <FileLeft> <FileRight> <FragmentLength> <StandardDeviation>
 *
 */
class Parameters{
	int m_reducerPeriod;

	bool m_reducerIsActivated;
	int m_size;
	COVERAGE_TYPE m_maxCoverage;
	int m_rank;
	int m_peakCoverage;

	int m_libraryAverageLength[MAXIMUM_NUMBER_OF_LIBRARIES];
	int m_libraryDeviation[MAXIMUM_NUMBER_OF_LIBRARIES];
	int m_numberOfLibraries;

	bool m_error;
	string m_prefix;
	bool m_amos;
	bool m_initiated;
	int m_numberOfSequences;
	vector<string> m_singleEndReadsFile;
	vector<uint64_t> m_numberOfSequencesInFile;
	uint64_t m_totalNumberOfSequences;
	map<int,int> m_fileLibrary;
	set<int> m_automaticLibraries;

	string m_directory;
	string m_outputFile;
	int m_wordSize;
	int m_minimumContigLength;
	set<int> m_leftFiles;
	set<int> m_rightFiles;
	set<int> m_interleavedFiles;

	bool m_colorSpaceMode;
	string m_input;
	vector<string> m_commands;
	map<int,map<int,int> >  m_observedDistances;
	vector<int> m_observedAverageDistances;
	vector<int> m_observedStandardDeviations;
	void loadCommandsFromArguments(int argc,char**argv);
	void loadCommandsFromFile(char*file);
	void parseCommands();

	string getLibraryFile(int library);

public:
	Parameters();
	string getReceivedMessagesFile();
	void constructor(int argc,char**argv,int rank);
	bool isInitiated();
	vector<string> getAllFiles();
	string getDirectory();
	int getMinimumContigLength();
	string getOutputFile();
	int getWordSize();
	int getFragmentLength(int i);
	int getStandardDeviation(int i);
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
	int getObservedAverageDistance(int library);
	int getObservedStandardDeviation(int library);
	uint64_t getNumberOfSequences(int n);
	void setNumberOfSequences(uint64_t n);
	int getNumberOfFiles();
	bool isAutomatic(int library);
	int getLibrary(int file);
	void printFinalMessage();
	bool isInterleavedFile(int i);
	void setPeakCoverage(int a);
	int getPeakCoverage();
	void addLibraryData(int library,int average,int deviation);
	int getNumberOfLibraries();

	int getMaxCoverage();
	string getPrefix();
	int getRank();
	int getSize();
	void setSize(int a);
	bool runReducer();
	void showUsage();
	int getReducerValue();

	int getRankFromGlobalId(uint64_t a);
	int getIdFromGlobalId(uint64_t a);
};

#endif

