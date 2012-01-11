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

//#define CONFIG_SEARCH_DIR_VERBOSE

#include <search-engine/SearchDirectory.h>

#include <assert.h>
#include <string.h>
#include <dirent.h> /* for opendir, readdir and closedir */
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;


void SearchDirectory::constructor(string path){
	m_path=path;

	// list entries
	readDirectory();

	// initialise enpty counts
	for(int i=0;i<(int)m_files.size();i++){
		m_counts.push_back(0);
	}
}

string*SearchDirectory::getFileName(int j){
	return &m_files[j];
}

// \see http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
void SearchDirectory::readDirectory(){
	struct dirent *ent;
	DIR*dir = opendir (m_path.c_str());
	if(dir != NULL){

		while((ent = readdir (dir)) != NULL){
			string fileName=ent->d_name;

			if(fileName.find(".fasta")==string::npos)
				continue;

			if(fileName==".fasta")
				continue;

			m_files.push_back(fileName);
		}
		closedir (dir);

	} else {
		/* could not open directory */
		cout<<"Warning, directory "<<m_path<<" is invalid."<<endl;
	}
}

int SearchDirectory::getCount(int i){
	#ifdef ASSERT
	assert(i<(int)m_counts.size());
	#endif
	return m_counts[i];
}

void SearchDirectory::countEntriesInFile(int fileNumber){
	#ifdef ASSERT
	assert(fileNumber<(int)m_files.size());
	#endif

	int count=0;
	ostringstream file;
	file<<m_path<<"/"<<m_files[fileNumber];

	ifstream f(file.str().c_str());

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"Opening "<<file.str()<<endl;
	#endif

	char line[10000];
	
	if(!f){
		cout<<"Error, cannot open "<<file.str()<<endl;
		f.close();
		return;
	}

	while(!f.eof()){
		f.getline(line,10000);
		if(strlen(line)>0 && line[0]=='>'){
			count++;
		}
	}

	m_counts[fileNumber]=count;

	f.close();
}

string* SearchDirectory::getDirectoryName(){
	return &m_path;
}

int SearchDirectory::getSize(){
	return m_files.size();
}

void SearchDirectory::setCount(int file,int count){
	m_counts[file]=count;
}

void SearchDirectory::createSequenceReader(int file,int sequence){
}

bool SearchDirectory::hasNextKmer(){
	return false;
}

void SearchDirectory::iterateToNextKmer(){
}

void SearchDirectory::getNextKmer(Kmer*kmer){
}

string SearchDirectory::getCurrentSequenceName(){
	return "Unknown";
}
