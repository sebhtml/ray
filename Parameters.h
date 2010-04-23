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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _Parameters

#define _Parameters


#define _AUTOMATIC_DETECTION 65535

#include<map>
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
	bool m_error;
	string m_contigsFile;
	string m_amosFile;
	bool m_amos;
	bool m_initiated;
	int m_numberOfSequences;
	vector<string> m_singleEndReadsFile;
	vector<int> m_numberOfSequencesInFile;
	map<int,int> m_automaticRightFiles;// mapping from rightFileId to libraryId
	string m_directory;
	string m_outputFile;
	int m_wordSize;
	int m_minimumContigLength;
	set<int> m_leftFiles;
	set<int> m_rightFiles;
	map<int,int> m_averageFragmentLengths;
	map<int,int> m_standardDeviations;
	bool m_colorSpaceMode;
	string m_input;
	vector<string> m_commands;
	vector<map<int,int> > m_observedDistances;
	vector<int> m_observedAverageDistances;
	vector<int> m_observedStandardDeviations;
	void loadCommandsFromArguments(int argc,char**argv);
	void loadCommandsFromFile(char*file);
	void parseCommands();
public:
	Parameters();
	void load(int argc,char**argv);
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
	string getContigsFile();
	string getCoverageDistributionFile();
	string getEngineName();
	string getVersion();
	vector<string> getCommands();
	bool getError();
	void addDistance(int library,int distance,int count);
	void computeAverageDistances();
	int getObservedAverageDistance(int library);
	int getObservedStandardDeviation(int library);
	int getNumberOfSequences(int n);
	void setNumberOfSequences(int n);
	int getNumberOfFiles();
	bool isAutomatic(int file);
	int getLibrary(int file);
};

#endif

