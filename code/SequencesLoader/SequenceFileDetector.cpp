/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
 *  Copyright (C) 2013 Sébastien Boisvert
 *
 *  http://DeNovoAssembler.SourceForge.Net/
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You have received a copy of the GNU General Public License
 *  along with this program (gpl-3.0.txt).
 *  see <http://www.gnu.org/licenses/>
 */

#include "SequenceFileDetector.h"
#include "LoaderFactory.h"

#include <RayPlatform/core/OperatingSystem.h>

#include <vector>
#include <string>
#include <set>
using namespace std;

void SequenceFileDetector::detectSequenceFiles(string & directory) {

	bool hasSeparator = false;
	char directorySeparator = '/';

#ifdef _WIN32
	directorySeparator = '\\';
#endif

	if(directory[directory.length()-1] == directorySeparator)
		hasSeparator = true;

	// read files
	vector<string> rawFiles;

	getDirectoryFiles(directory, rawFiles);

	vector<string> files;

	// keep supported files
	LoaderFactory factory;

	for(int i = 0 ; i < (int) rawFiles.size() ; ++ i) {

		string & file = rawFiles[i];
		if(factory.makeLoader(file) != NULL)
			files.push_back(file);
	}

	set<int> consumedFiles;

	// detect pairs
	for(int i = 0 ; i < (int) files.size() ; ++i){

		if(consumedFiles.count(i) > 0)
			continue;

		for(int j = 0 ; j < (int) files.size() ; ++j) {

			if(consumedFiles.count(j) > 0)
				continue;

			if(i == j)
				continue;

			string & file1 = files[i];
			string & file2 = files[j];

			int mismatches = 0;

			int position1 = 0;
			int position2 = 0;

			while(position1 < (int)file1.length() && position2 < (int)file2.length()) {

				if(file1[position1++] != file2[position2++])
					mismatches++;
			}

			if(mismatches <= 2) {

				ostringstream file1WithPath;
				file1WithPath << directory;
			       
				if(!hasSeparator)
					file1WithPath << directorySeparator;

				file1WithPath << file1;
				m_leftFiles.push_back(file1WithPath.str());

				ostringstream file2WithPath;
				file2WithPath << directory;
			       
				if(!hasSeparator)
					file2WithPath << directorySeparator;

				file2WithPath << file2;
				m_rightFiles.push_back(file2WithPath.str());

				consumedFiles.insert(i);
				consumedFiles.insert(j);
			}
		}

	}

	// detect single files
	for(int i = 0 ; i < (int) files.size() ; ++i){

		if(consumedFiles.count(i) > 0)
		continue;

		string & file = files[i];

		ostringstream fileWithPath;
		fileWithPath << directory;

		if(!hasSeparator) {
			fileWithPath << directorySeparator;
		}

		m_singleFiles.push_back(file);
		consumedFiles.insert(i);
	}
}

vector<string> & SequenceFileDetector::getLeftFiles() {
	return m_leftFiles;
}

vector<string> & SequenceFileDetector::getRightFiles() {

	return m_rightFiles;
}

vector<string> & SequenceFileDetector::getSingleFiles() {

	return m_singleFiles;
}

