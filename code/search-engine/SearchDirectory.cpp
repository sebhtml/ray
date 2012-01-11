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
#include <core/common_functions.h> /* for wordId */
using namespace std;


void SearchDirectory::constructor(string path){
	m_path=path;

	// list entries
	readDirectory();

	// initialise enpty counts
	for(int i=0;i<(int)m_files.size();i++){
		m_counts.push_back(0);
	}

	m_hasFile=false;
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

	#ifdef ASSERT
	assert(file<(int)m_files.size());
	assert(sequence<m_counts[file]);
	#endif

	// close the file and open the good one
	if(m_hasFile && m_currentFile!=file){

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"Closing current file"<<endl;
		#endif

		m_currentFileStream.close();
		m_hasFile=false;
	}

	// open the file
	if(!m_hasFile){
		ostringstream fileName;
		fileName<<m_path<<"/"<<m_files[file];

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"Opening current file "<<fileName.str()<<endl;
		cout<<"File has "<<getCount(file)<<" sequences"<<endl;
		#endif

		m_currentFileStream.open(fileName.str().c_str());
		m_currentFile=file;
	
		// we set it to -1 to be able to pick up 0
		m_currentSequence=-1;// start at the beginning

		m_hasFile=true;
	}

	#ifdef ASSERT
	assert(m_hasFile);
	assert(m_currentFile==file);
	
	if(m_currentSequence>sequence){
		cout<<"m_currentSequence: "<<m_currentSequence<<" sequence: "<<sequence<<endl;
	}
	
	// be need to be at least before the one we want to 
	// pick it up
	assert(m_currentSequence < sequence);
	#endif

	// here we want to advance to the sequence 
	
	while(m_currentSequence<sequence && !m_currentFileStream.eof()){
		char line[10000];
		m_currentFileStream.getline(line,10000);
		if(strlen(line)>0 && line[0]=='>'){
			m_currentSequence++;
			// we have the header
			m_currentSequenceHeader=line;
		}
	}

	#ifdef ASSERT
	if(m_currentSequence!=sequence){
		cout<<"Expected: "<<sequence<<" Actual: "<<m_currentSequence<<endl;
	}
	assert(m_currentSequence==sequence);
	#endif

	m_currentSequenceBuffer.clear();
	m_currentSequencePosition=0;
	m_noMoreSequence=false;

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"Ready to process sequence "<<sequence<<endl;
	#endif
}

bool SearchDirectory::hasNextKmer(int kmerLength){
	// attempt to load some data
	if(((int)m_currentSequenceBuffer.length() - m_currentSequencePosition) < kmerLength
		&& !m_noMoreSequence){

		// load some more
		loadSomeSequence();
	}

	if(((int)m_currentSequenceBuffer.length() - m_currentSequencePosition) <kmerLength){
		return false;
	}

	return true;
}

void SearchDirectory::iterateToNextKmer(){
	m_currentSequencePosition++;
}

void SearchDirectory::getNextKmer(int kmerLength,Kmer*kmer){
	char sequenceBuffer[400];
	memcpy(sequenceBuffer,m_currentSequenceBuffer.c_str()+m_currentSequencePosition,kmerLength);
	sequenceBuffer[kmerLength]='\0';
	*kmer=wordId(sequenceBuffer);
}

string SearchDirectory::getCurrentSequenceName(){
	int maximumLength=40;
	// if '|' are there, this is the NCBI format
	// skip 4 '|' and return the rest
	size_t position=m_currentSequenceHeader.find_last_of('|');

	if(position!=string::npos){
		if(position+2<m_currentSequenceHeader.length()){
			position+=2;
		}

		#ifdef ASSERT
		if(position>=m_currentSequenceHeader.length()){
			cout<<"Header= "<<m_currentSequenceHeader<<endl;
		}
		assert(position<m_currentSequenceHeader.length());
		#endif

		//skip the '|' and the ' '
		return m_currentSequenceHeader.substr(position,maximumLength);
	}

	// otherwise
	// remove the '>' and keep only 100 characters.
	//cout<<"code 108"<<endl;

	#ifdef ASSERT
	if(m_currentSequenceHeader.length()==0){
		cout<<"m_currentSequenceHeader is empty, fatal"<<endl;
	}
	assert(m_currentSequenceHeader.length()>0);
	#endif

	return m_currentSequenceHeader.substr(1,maximumLength);
}

// load in chunks
void SearchDirectory::loadSomeSequence(){
	if(m_noMoreSequence)
		return;

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"Loading some more bits"<<endl;
	#endif

	ostringstream newContent;
	
	// copy old content
	// discard already processed content -- the bytes 
	// before m_currentSequencePosition that is
	//cout<<"code 203"<<endl;
	newContent<<m_currentSequenceBuffer.substr(m_currentSequencePosition,m_currentSequenceBuffer.length());

	// load some lines
	int lines=100;
	int loaded=0;
	for(int i=0;i<lines;i++){
		char line[10000];
		int position=m_currentFileStream.tellg();
		m_currentFileStream.getline(line,10000);

		// we reached the next sequence
		// rollback to where we were before
		if(strlen(line)>0 && line[0]=='>'){
			m_currentFileStream.seekg(position);// return to the old place
			m_noMoreSequence=true;
			break; // we don't add this line and we stop here
		}

		newContent<<line;
		loaded++;
	}

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"Loaded "<<loaded<<" lines in working buffer"<<endl;
	#endif

	// set the new content.
	m_currentSequencePosition=0;
	m_currentSequenceBuffer=newContent.str();
}
