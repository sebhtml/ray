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

SequenceFileDetector::SequenceFileDetector() {

	m_directorySeparator = '/';

#ifdef _WIN32
	m_directorySeparator = '\\';
#endif

}

// TODO: this code does not detect loops...
void SequenceFileDetector::gatherAllFiles(string & root, vector<string> & rawFiles) {

#ifdef DEBUG_SMART_LOADER
	cout << "DEBUG SequenceFileDetector::gatherAllFiles ";
	cout << root << endl;
#endif

	// explore the directory
	if(isDirectory(root)) {

		vector<string>  entries;
		getDirectoryFiles(root, entries);

		for(vector<string>::iterator i = entries.begin() ;
				i != entries.end() ; ++i) {

			string & entry = *i;

			// skip self and parent
			if(entry == "." || entry == "..")
				continue;

			ostringstream path;
			path << root;

			if(root[root.length()-1] != m_directorySeparator)
				path << m_directorySeparator;

			path << entry;

			string aPath = path.str();

			gatherAllFiles(aPath, rawFiles);
		}

	// add the file directly...
	} else {

		rawFiles.push_back(root);
	}
}

string SequenceFileDetector::replaceString(const string & templateString, const string & oldString,
		const string & newString) {

	int position = templateString.find(oldString);

	if(position < 0)
		return templateString;

	string mutableString = templateString;

	string result = mutableString.replace(position, oldString.length(), newString);

	return result;
}

bool SequenceFileDetector::match(map<string, int> & fileIndex,
	vector<string> & files,
	const char * sequence1, const char * sequence2,
	bool enableSmartMatchingMode,
	set<int> & consumedFiles, int fileNumber) {

	int i = fileNumber;

	string & file1 = files[i];
	string newFile = replaceString(file1, "_R1_", "_R2_");

	if(enableSmartMatchingMode && fileIndex.count(newFile) > 0
			&& newFile != file1) {

		int index2 = fileIndex[newFile];

#ifdef CONFIG_ASSERT
		assert(i != index2);
#endif /// CONFIG_ASSERT

		if(consumedFiles.count(index2) == 0) {
			m_leftFiles.push_back(file1);
			m_rightFiles.push_back(newFile);
			consumedFiles.insert(i);
			consumedFiles.insert(index2);

			return true;
		}
	}

	return false;
}


void SequenceFileDetector::detectSequenceFiles(string & directory) {

	// read files
	vector<string> rawFiles;
	gatherAllFiles(directory, rawFiles);

	vector<string> files;

	// keep supported files
	LoaderFactory factory;

	for(int i = 0 ; i < (int) rawFiles.size() ; ++ i) {

		string & file = rawFiles[i];
		if(factory.makeLoader(file) != NULL)
			files.push_back(file);
	}

	// build an index

	map<string, int> fileIndex;

	for(int i = 0 ; i < (int) files.size() ; ++i) {

		string & fileName = files[i];

#ifdef DEBUG_SMART_LOADER
		cout << "DEBUG fileIndex " << i << " " << fileName << endl;
#endif

		fileIndex[fileName] = i;
	}

	set<int> consumedFiles;

	/**
	 * Options for the matching algorithm
	 */
	bool enableSmartMatchingMode = true;

#ifdef BRUTE_FORCE_MATCHING
	bool enableBestHitMatchingMode = true;
	int configurationMaximumHits = 1;
	int configurationMinimumScore = 0;
	int configurationMaximumScore = 2;
#endif

	// detect pairs
	for(int i = 0 ; i < (int) files.size() ; ++i){

		if(consumedFiles.count(i) > 0)
			continue;

		// First, try to match the files using pairing rules.
		//
		// _R1_ + _R2_
		// _R2_ + _R1_
		// _1.fa + _2.fa
		// _2.fa + _1.fa


/*
		if(match(fileIndex, files, "_R1_", "_R2_", enableSmartMatchingMode, consumedFiles, i))
			continue;
*/

		// first, replace the _R1_ with _R2_
		string & file1 = files[i];
		string newFile = replaceString(file1, "_R1_", "_R2_");

		if(enableSmartMatchingMode && fileIndex.count(newFile) > 0
				&& newFile != file1) {

			int index2 = fileIndex[newFile];

#ifdef CONFIG_ASSERT
			assert(i != index2);
#endif // CONFIG_ASSERT

			if(consumedFiles.count(index2) == 0) {
				m_leftFiles.push_back(file1);
				m_rightFiles.push_back(newFile);
				consumedFiles.insert(i);
				consumedFiles.insert(index2);

				continue;
			}
		}

		// try to replace the _R2. with _R1.
		file1 = files[i];
		newFile = replaceString(file1, "_R2.", "_R1.");

		if(enableSmartMatchingMode && fileIndex.count(newFile) > 0
				&& newFile != file1) {

			int index2 = fileIndex[newFile];

#ifdef CONFIG_ASSERT
			assert(i != index2);
#endif // CONFIG_ASSERT

			if(consumedFiles.count(index2) == 0) {
				m_leftFiles.push_back(newFile);
				m_rightFiles.push_back(file1);
				consumedFiles.insert(i);
				consumedFiles.insert(index2);

				continue;
			}
		}



		// try to replace the _R2_ with _R1_
		file1 = files[i];
		newFile = replaceString(file1, "_R2_", "_R1_");

		if(enableSmartMatchingMode && fileIndex.count(newFile) > 0
				&& newFile != file1) {

			int index2 = fileIndex[newFile];

#ifdef CONFIG_ASSERT
			assert(i != index2);
#endif // CONFIG_ASSERT

			if(consumedFiles.count(index2) == 0) {
				m_leftFiles.push_back(newFile);
				m_rightFiles.push_back(file1);
				consumedFiles.insert(i);
				consumedFiles.insert(index2);

				continue;
			}
		}

		// replace the _1.fa with _2.fa
		file1 = files[i];
		newFile = replaceString(file1, "_1.f", "_2.f");

		if(enableSmartMatchingMode && fileIndex.count(newFile) > 0
				&& newFile != file1) {

			int index2 = fileIndex[newFile];

#ifdef CONFIG_ASSERT
			assert(i != index2);
#endif // CONFIG_ASSERT

			if(consumedFiles.count(index2) == 0) {
				m_leftFiles.push_back(file1);
				m_rightFiles.push_back(newFile);
				consumedFiles.insert(i);
				consumedFiles.insert(index2);

				continue;
			}
		}

		// try to replace the _2.fa with _1.fa
		file1 = files[i];
		newFile = replaceString(file1, "_2.f", "_1.f");

		if(enableSmartMatchingMode && fileIndex.count(newFile) > 0
				&& newFile != file1) {

			int index2 = fileIndex[newFile];

#ifdef CONFIG_ASSERT
			assert(i != index2);
#endif // CONFIG_ASSERT

			if(consumedFiles.count(index2) == 0) {

				m_leftFiles.push_back(newFile);
				m_rightFiles.push_back(file1);
				consumedFiles.insert(i);
				consumedFiles.insert(index2);

				continue;
			}
		}

		// try to do it by brute force !

#ifdef BRUTE_FORCE_MATCHING // not very good

		int bestMatch = -1;
		int bestScore = 999;
		int withBestMatch = 0;

		for(int j = 0 ; j < (int) files.size() ; ++j) {

			if(!enableBestHitMatchingMode)
				break;

			if(consumedFiles.count(j) > 0)
				continue;

			if(i == j)
				continue;

			string & file2 = files[j];

			int position1 = 0;
			int position2 = 0;

			// find the last m_directorySeparator

			int positionForFile1 = 0;
			while(positionForFile1 < (int)file1.length()) {
				if(file1[positionForFile1] == m_directorySeparator)
					position1 = positionForFile1;

				positionForFile1 ++;
			}

			int positionForFile2 = 0;
			while(positionForFile2 < (int)file2.length()) {
				if(file2[positionForFile2] == m_directorySeparator)
					position2 = positionForFile2;

				positionForFile2 ++;
			}

			// the files must be in the same directory
			if(position1 != position2)
				continue;

			// compute mismatches before the last separator

			int mismatches = 0;

			int positionBefore1 = 0;
			int positionBefore2 = 0;

			while(positionBefore1 < position1 && positionBefore2 < position2) {

				if(file1[positionBefore1] != file2[positionBefore2])
					mismatches++;

				positionBefore1 ++;
				positionBefore2 ++;
			}

			// skip because they are not in the same directory
			if(mismatches != 0)
				continue;
#if 0
			cout << "DEBUG position1 " << position1 << " position2 " << position2 << endl;
#endif
			mismatches = 0;

			while(position1 < (int)file1.length() && position2 < (int)file2.length()) {

				if(file1[position1++] != file2[position2++])
					mismatches++;
			}

			if(bestMatch < 0) {
				bestMatch = j;
				bestScore = mismatches;

				withBestMatch = 1;
			}

			if(mismatches < bestScore) {
				bestMatch = j;
				bestScore = mismatches;
				withBestMatch = 1;
			} else if(mismatches == bestScore) {

				withBestMatch ++;
			}
		}

		if(bestScore > configurationMinimumScore
				&& bestScore <= configurationMaximumScore
				&& withBestMatch <= configurationMaximumHits) {

			string & file1 = files[i];
			string & file2 = files[bestMatch];

#if 0
			cout << "DEBUG matching files " << file1 << " and " << file2 << endl;
#endif

			m_leftFiles.push_back(file1);
			m_rightFiles.push_back(file2);

			consumedFiles.insert(i);
			consumedFiles.insert(bestMatch);
		}

#endif


	}

	// detect single files
	for(int i = 0 ; i < (int) files.size() ; ++i){

		if(consumedFiles.count(i) > 0)
		continue;

		string & file = files[i];

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

