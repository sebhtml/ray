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

//#define CONFIG_SEARCH_DIR_VERBOSE

#include <plugin_Searcher/SearchDirectory.h>

#include <assert.h>
#include <string.h>

/* 
 * I am not sure that dirent.h is available on Microsoft(R) Windows(R).
 * Anyway, at this point, you are better off installing Cygwin in Windows(R)
 * to acquire a POSIX compatibility layer on this otherwise not-POSIX 
 * system
 */
#include <dirent.h> /* for opendir, readdir and closedir */

#include <iostream>
#include <fstream>
#include <sstream>
#include <application_core/common_functions.h> /* for wordId */
using namespace std;


void SearchDirectory::constructor(string path){
	m_hasBufferedLine=false;
	strcpy(m_bufferedLine,"");

	m_path=path;

	// list entries
	readDirectory();

	// initialise empty counts
	for(int i=0;i<(int)m_files.size();i++){
		m_counts.push_back(0);
	}

	m_hasFile=false;

	m_currentFileStream=NULL;

}

string*SearchDirectory::getFileName(int j){

	return &m_files[j];
}

/*
 * \see http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
 */
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
	if(i>=(int)m_counts.size()){
		cout<<"Error, file= "<<i<<" but size= "<<m_counts.size()<<endl;
		cout<<"Directory= "<<getDirectoryName()<<endl;

		cout.flush();
	}

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

		if(lineIsSequenceHeader(line)){
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
	
	#ifdef ASSERT
	int current=m_counts[file];
	if(current!=0){
		assert(current==count);
	}
	#endif

	m_counts[file]=count;
}

void SearchDirectory::createSequenceReader(int file,int sequence,int kmerLength){

	#ifdef ASSERT
	assert(file<(int)m_files.size());
	assert(sequence<m_counts[file]);
	#endif

	// close the file and open the good one
	if(m_hasFile && m_currentFile!=file){

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"Closing current file"<<endl;
		#endif

		#ifdef ASSERT
		//assert(m_currentFileStream.is_open());
		#endif

		fclose(m_currentFileStream);
		m_currentFileStream=NULL;
		m_hasFile=false;

		#ifdef ASSERT
		//assert(!m_currentFileStream.is_open());
		assert(m_currentFileStream==NULL);
		#endif

	}

	// open the file
	if(!m_hasFile){

		#ifdef ASSERT
		assert(m_currentFileStream==NULL);
		#endif

		ostringstream fileName;
		fileName<<m_path<<"/"<<m_files[file];

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"Opening current file "<<fileName.str()<<endl;
		cout<<"File has "<<getCount(file)<<" sequences"<<endl;
		#endif

		m_currentFileStream=fopen(fileName.str().c_str(),"r");
		m_currentFile=file;
	
		// we set it to -1 to be able to pick up 0
		m_currentSequence=-1;// start at the beginning

		m_hasFile=true;

		#ifdef ASSERT
		//assert(m_currentFileStream.is_open());
		assert(m_currentFileStream!=NULL);
		assert(m_hasFile);
		#endif
	}

	#ifdef ASSERT
	assert(m_hasFile);
	assert(m_currentFile==file);
	assert(m_currentFileStream!=NULL);
	if(m_currentSequence>=sequence){
		cout<<"m_currentSequence: "<<m_currentSequence<<" sequence: "<<sequence<<endl;
	}
	
	// be need to be at least before the one we want to 
	// pick it up
	assert(m_currentSequence < sequence);
	#endif

	// here we want to advance to the sequence 
	
	while(m_currentSequence<sequence && !feof(m_currentFileStream)){
		
		char line[CONFIG_COLORED_LINE_MAX_LENGTH];
		strcpy(line,"");

		readLineFromFile(line,CONFIG_COLORED_LINE_MAX_LENGTH);

		if(lineIsSequenceHeader(line)){

			m_currentSequence++;

			// we have the header
			strcpy(m_currentSequenceHeader,line);
		}
	}

	#ifdef ASSERT
	if(m_currentSequence!=sequence){
		cout<<"Expected: "<<sequence<<" Actual: "<<m_currentSequence<<endl;
	}
	assert(m_currentSequence==sequence);
	#endif

	strcpy(m_currentSequenceBuffer,"");
	m_currentSequencePosition=0;
	m_noMoreSequence=false;

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"Ready to process sequence "<<sequence<<endl;
	#endif


	/* at this point, we are ready to read. but first, we want to get the number of k-mers to
 * pump */

	m_currentSequenceNumberOfAvailableKmers=0;

	#ifdef ASSERT
	assert(strlen(m_currentSequenceHeader)>0);
	assert(m_currentSequenceHeader[0]=='>');
	assert(strlen(m_currentSequenceBuffer)==0);
	assert(!m_noMoreSequence);
	assert(m_currentFileStream!=NULL);
	#endif
}

void SearchDirectory::readLineFromFile(char*line,int length){
	// use the buffer
	if(m_hasBufferedLine){
		#ifdef ASSERT
		assert(m_hasBufferedLine);
		#endif

		strcpy(line,m_bufferedLine);
		strcpy(m_bufferedLine,"");
		m_hasBufferedLine=false;

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"Using buffered line!"<<endl;
		#endif

		#ifdef ASSERT
		assert(!m_hasBufferedLine);
		#endif

		return;
	}
	
	#ifdef ASSERT
	assert(strlen(m_bufferedLine)==0);
	assert(!m_hasBufferedLine);
	assert(m_currentFileStream!=NULL);
	#endif

	#ifdef ASSERT
	assert(line!=NULL);
	assert(length>0);
	assert(m_currentFileStream!=NULL);
	#endif

	fgets(line,length,m_currentFileStream);

	// remove the new line symbol, if any
	
	if(line[strlen(line)-1]=='\n'){
		line[strlen(line)-1]='\0';
	}
}

int SearchDirectory::getCurrentSequenceLengthInKmers(){

	return m_currentSequenceNumberOfAvailableKmers;
}

bool SearchDirectory::hasNextKmer(int kmerLength){

	#ifdef ASSERT
	assert(kmerLength>0);
	#endif

	// attempt to load some data
	if( !m_noMoreSequence  && ((int)strlen(m_currentSequenceBuffer) - m_currentSequencePosition) < kmerLength
		){

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"hasNextKmer calls loadSomeSequence()"<<endl;

		#endif

		// load some more
		loadSomeSequence();

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"hasNextKmer returns true kmer= "<<kmerLength<<" position= "<<m_currentSequencePosition;
		cout<<" buffer: "<<strlen(m_currentSequenceBuffer)<<endl;
		#endif

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"after"<<endl;
		#endif
	}

	// if we still don't have enough data
	// we are done
	if(((int)strlen(m_currentSequenceBuffer) - m_currentSequencePosition) <kmerLength){

		// close the file and reset the thing
		// if the sequence is the last one
		if(m_hasFile && m_currentSequence == getCount(m_currentFile)-1){

			#ifdef CONFIG_SEARCH_DIR_VERBOSE
			cout<<"Closing file in hasNextKmer"<<endl;
			#endif

			#ifdef ASSERT
			//assert(m_currentFileStream.is_open());
			assert(m_currentFileStream!=NULL);
			#endif

			// remove the current file.
			fclose(m_currentFileStream);
			m_hasFile=false;
			m_currentFileStream=NULL;

			#ifdef ASSERT
			//assert(!m_currentFileStream.is_open());
			#endif

			#ifdef ASSERT
			assert(m_hasFile==false);
			assert(m_currentFileStream==NULL);
			#endif
		}

		#ifdef CONFIG_SEARCH_DIR_VERBOSE
		cout<<"don't have more k-mers"<<endl;
		#endif

		return false;
	}

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"hasNextKmer returns true kmer= "<<kmerLength<<" position= "<<m_currentSequencePosition;
	cout<<" buffer: "<<strlen(m_currentSequenceBuffer)<<endl;
	#endif

	return true;
}

void SearchDirectory::iterateToNextKmer(){

	m_currentSequencePosition++;
}

void SearchDirectory::getNextKmer(int kmerLength,Kmer*kmer){

	#ifdef ASSERT
	assert(kmerLength>0);
	#endif

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"getNextKmer m_currentSequencePosition= "<<m_currentSequencePosition<<endl;
	#endif

	m_hasN=false;

	char sequenceBuffer[400];
	memcpy(sequenceBuffer,m_currentSequenceBuffer+m_currentSequencePosition,kmerLength);
	sequenceBuffer[kmerLength]='\0';

	#ifdef ASSERT
	assert((int)strlen(sequenceBuffer)==kmerLength);
	#endif

	// convert bases to upper case
	for(int i=0;i<kmerLength;i++){
		char nucleotide=sequenceBuffer[i];
		switch (nucleotide){
			case 'a':
				nucleotide='A';
				break;
			case 't':
				nucleotide='T';
				break;
			case 'c':
				nucleotide='C';
				break;
			case 'g':
				nucleotide='G';
				break;
		}

		if(nucleotide!='A' && nucleotide!='T' && nucleotide!='C'
			&& nucleotide!='G'){

			m_hasN=true;
		}

		sequenceBuffer[i]=nucleotide;
	}

	*kmer=wordId(sequenceBuffer);

}

bool SearchDirectory::kmerContainsN(){

	return m_hasN;
}

string SearchDirectory::getCurrentSequenceName(){

	int maximumLength=64;

	string currentSequenceHeader=m_currentSequenceHeader;

	// per default, just returns the header without any parsing...
	#ifdef CONFIG_USE_NCBI_HEADERS
	// if '|' are there, this is the NCBI format
	// skip 4 '|' and return the rest
	size_t position=currentSequenceHeader.find_last_of('|');

	if(position!=string::npos){
		if(position+2<currentSequenceHeader.length()){
			position+=2;
		}

		#ifdef ASSERT
		if(position>=currentSequenceHeader.length()){
			cout<<"Header= "<<currentSequenceHeader<<endl;
		}
		assert(position<currentSequenceHeader.length());
		#endif

		//skip the '|' and the ' '
		return filterName(currentSequenceHeader.substr(position,maximumLength));
	}

	#endif

	// otherwise
	// remove the '>' and keep only maximumLength characters.
	//cout<<"code 108"<<endl;

	#ifdef ASSERT
	if(currentSequenceHeader.length()==0){
		cout<<"currentSequenceHeader is empty, fatal"<<endl;
	}
	assert(currentSequenceHeader.length()>0);
	#endif

	return filterName(currentSequenceHeader.substr(1,maximumLength));
}

// load in chunks
void SearchDirectory::loadSomeSequence(){

	// nothing to load
	if(m_noMoreSequence)
		return;

	#ifdef ASSERT
	assert(m_currentFileStream!=NULL);
	assert(!m_noMoreSequence);
	#endif

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"Loading some more bits"<<endl;
	#endif

	char newContent[CONFIG_COLORED_LINE_MAX_LENGTH];
	int contentPosition=0;
	strcpy(newContent,"");
	
	// copy old content
	// discard already processed content -- the bytes 
	// before m_currentSequencePosition that is
	//cout<<"code 203"<<endl;
	
	int theLength=strlen(m_currentSequenceBuffer);

	while(m_currentSequencePosition<theLength){

		#ifdef ASSERT
		assert(theLength>0);
		#endif

		newContent[contentPosition++]=m_currentSequenceBuffer[m_currentSequencePosition++];
	}

	newContent[contentPosition]='\0';

	// load some lines
	int lines=100;
	int loaded=0;
	int i=0;

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"after copying remainings, newContent is "<<strlen(newContent)<<endl;
	#endif

	while(!feof(m_currentFileStream) && (i++ < lines)){

		char line[CONFIG_COLORED_LINE_MAX_LENGTH];
		strcpy(line,"");

		readLineFromFile(line,CONFIG_COLORED_LINE_MAX_LENGTH);

		// we reached the next sequence
		// rollback to where we were before
		if(lineIsSequenceHeader(line)){

			#ifdef ASSERT
			assert(strlen(m_bufferedLine)==0);
			assert(!m_hasBufferedLine);
			#endif

			strcpy(m_bufferedLine,line);

			m_noMoreSequence=true;

			m_hasBufferedLine=true;

			#ifdef CONFIG_SEARCH_DIR_VERBOSE
			cout<<"THe line is a header, buffering"<<endl;
			#endif

			break; // we don't add this line and we stop here
		}else{

			strcat(newContent,line);
			loaded++;
		}
	}

	/* there is no more sequence if the end was reached... */
	if(feof(m_currentFileStream)){
		m_noMoreSequence=true;
	}

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"Loaded "<<loaded<<" lines in working buffer"<<endl;
	#endif

	// set the new content.
	m_currentSequencePosition=0;
	strcpy(m_currentSequenceBuffer,newContent);

	#ifdef ASSERT
	assert(m_currentSequencePosition==0);
	#endif

	#ifdef CONFIG_SEARCH_DIR_VERBOSE
	cout<<"new m_currentSequenceBuffer has "<<strlen(m_currentSequenceBuffer)<<" nucleotides"<<endl;
	#endif
}

bool SearchDirectory::lineIsSequenceHeader(char*line){

	return strlen(line)>0 && line[0]=='>';
}

/**
 * \see http://www.ncbi.nlm.nih.gov/RefSeq/RSfaq.html
 */
bool SearchDirectory::hasCurrentSequenceIdentifier(){

	string currentSequenceHeader=m_currentSequenceHeader;

	//cout<<"Identifier= "<<m_currentSequenceHeader<<endl;

	// this means that the sequences are not genome sequences
	// but are genes or something like that.
	//if(m_currentSequenceHeader.find("|:") != string::npos){
		//cout<<"Contains '|:'"<<endl;

		//return false;
	//}

	if(currentSequenceHeader.find(">gi|") == string::npos)
		return false;

	if(currentSequenceHeader.find(">gi|")==0)
		return true;

	return false;
}

PhysicalKmerColor SearchDirectory::getCurrentSequenceIdentifier(){
	int count=0;
	int i=0;

	string currentSequenceHeader=m_currentSequenceHeader;

	while(i<(int)currentSequenceHeader.length() && count<2){
		if(currentSequenceHeader[i]=='|')
			count++;
		
		
		i++;
	}

	if(count!=2){
		return DUMMY_IDENTIFIER; // return a dummy identifier
	}

	// >gi|1234|
	//
	// 0123456789
	//
	// 9-4-1 = 4
	//
	string content=currentSequenceHeader.substr(4,i-4-1);

	istringstream aStream;
	aStream.str(content);

	PhysicalKmerColor identifier;

	aStream>>identifier;

	return identifier;
}

bool SearchDirectory::hasDirectory(int file){
	return m_createdDirectories.count(file)>0;
}

void SearchDirectory::setCreatedDirectory(int file){
	m_createdDirectories.insert(file);
}

string SearchDirectory::filterName(string a){
	for(int i=0;i<(int)a.length();i++){
		if(a[i]=='<' || a[i]=='>' || a[i]=='&'){
			a[i]='_';
		}
	}

	return a;
}

/* 
 * >EMBL_CDS:CBW26015 CBW26015.1 */
bool SearchDirectory::hasIdentifier_EMBL_CDS(){
	
	string currentSequenceHeader=m_currentSequenceHeader;

	if(currentSequenceHeader.find(">EMBL_CDS:") == 0){
		return true;
	}

	return false;
}

PhysicalKmerColor SearchDirectory::getIdentifier_EMBL_CDS(){

	#ifdef ASSERT
	assert(hasIdentifier_EMBL_CDS());
	#endif

	/* >EMBL_CDS:CBW26015 CBW26015.1 */

	string header=m_currentSequenceHeader;
	string token=header.substr(10,8);

	return m_encoder.getEncoded_EMBL_CDS(token.c_str());
}


