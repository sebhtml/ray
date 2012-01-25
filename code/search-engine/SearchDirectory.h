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

#include <string>
#include <structures/Kmer.h>
#include <fstream>
#include <vector>
using namespace std;

#define DUMMY_IDENTIFIER 999999999999

class SearchDirectory{

	bool m_hasN;

	string m_path;

	vector<string> m_files;
	vector<int> m_counts;

	/** sequence lazy loader */
	bool m_hasFile;
	int m_currentSequencePosition;
	string m_currentSequenceHeader;
	string m_currentSequenceBuffer;
	int m_currentFile;
	int m_currentSequence;

	bool m_noMoreSequence;
	ifstream m_currentFileStream;

	void readDirectory();

	/** lazy load some sequences */
	void loadSomeSequence();

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
	void createSequenceReader(int file,int sequence);
	bool hasNextKmer(int kmerLength);
	void iterateToNextKmer();
	void getNextKmer(int kmerLength,Kmer*kmer);

	bool kmerContainsN();

	string getCurrentSequenceName();

	bool hasCurrentSequenceIdentifier();

	uint64_t getCurrentSequenceIdentifier();
};

#endif

