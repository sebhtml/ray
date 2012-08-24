/*
 	Ray
    Copyright (C) 2012  Sébastien Boisvert

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

#ifndef _SearchDirectory_h
#define _SearchDirectory_h

#include <plugin_KmerAcademyBuilder/Kmer.h>
#include <plugin_GeneOntology/KeyEncoder.h>
#include <plugin_Searcher/VirtualKmerColor.h>

#include <set>
#include <string>
#include <fstream>
#include <vector>
using namespace std;

#define DUMMY_IDENTIFIER 999999999999ULL
#define CONFIG_COLORED_LINE_MAX_LENGTH 10000

/**
 * This class represents a directory
 * with fasta files
 * to be used for coloring
 */
class SearchDirectory{

	KeyEncoder m_encoder;

	bool m_hasBufferedLine;

/** this is to avoid using tellg() and seekg() **/
	char m_bufferedLine[CONFIG_COLORED_LINE_MAX_LENGTH];

	bool m_hasN;

	string m_path;

	vector<string> m_files;
	vector<int> m_counts;

	set<int> m_createdDirectories;

	/** sequence lazy loader */
	bool m_hasFile;
	int m_currentSequencePosition;
	char m_currentSequenceHeader[CONFIG_COLORED_LINE_MAX_LENGTH];
	char m_currentSequenceBuffer[CONFIG_COLORED_LINE_MAX_LENGTH];
	int m_currentSequenceNumberOfAvailableKmers;

	int m_currentFile;
	int m_currentSequence;

	bool m_noMoreSequence;
	FILE* m_currentFileStream;

	void readDirectory();

	/** lazy load some sequences */
	void loadSomeSequence();


	void readLineFromFile(char*line,int length);

	bool lineIsSequenceHeader(char*line);

	string filterName(string a);
public:
	void constructor(string path);

/** get the number of sequences in the file */
	int getCount(int i);

	string*getFileName(int i);
	string*getDirectoryName();
	void countEntriesInFile(int j);

/** get the number of files in the directory */
	int getSize();

	void setCount(int file,int count);

	// sequence reader
	void createSequenceReader(int file,int sequence,int kmerLength);

	int getCurrentSequenceLengthInKmers();
	bool hasNextKmer(int kmerLength);
	void iterateToNextKmer();
	void getNextKmer(int kmerLength,Kmer*kmer);

	bool kmerContainsN();

	string getCurrentSequenceName();

	bool hasCurrentSequenceIdentifier();
	LargeIndex getCurrentSequenceIdentifier();

	bool hasIdentifier_EMBL_CDS();
	PhysicalKmerColor getIdentifier_EMBL_CDS();

	bool hasDirectory(int file);
	void setCreatedDirectory(int file);
};

#endif

