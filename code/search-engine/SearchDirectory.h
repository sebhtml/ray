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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _SearchDirectory_h
#define _SearchDirectory_h

#include <string>
#include <vector>
using namespace std;

class SearchDirectory{

	string m_path;

	vector<string> m_files;
	vector<int> m_counts;

	void readDirectory();

public:
	void constructor(string path);
	int getCount(int i);
	string*getFileName(int i);
	string*getDirectoryName();
	void countEntriesInFile(int j);
	int getSize();

	void setCount(int file,int count);
};

#endif

