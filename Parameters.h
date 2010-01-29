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
#include<string>
#include<vector>
using namespace std;

class Parameters{
	bool m_initiated;
	int m_numberOfSequences;
	vector<string> m_singleEndReadsFile;
	string m_directory;
	int m_wordSize;
public:
	Parameters();
	void load(string file);
	bool isInitiated();
	vector<string> getAllFiles();
	string getDirectory();
	int getWordSize();
};

#endif

