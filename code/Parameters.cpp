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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include<common_functions.h>
#ifdef ASSERT
#include<assert.h>
#endif
#include<math.h>
#include<Parameters.h>
#include<string>
#include<sstream>
#include<iostream>
#include<vector>
#include<cstdlib>
#include<fstream>
#include<Read.h>
#include<Loader.h>
#include<Parameters.h>
#include<MyAllocator.h>
using namespace std;

Parameters::Parameters(){
	m_prefix="RayOutput";
	m_initiated=false;
	m_directory="assembly";
	m_minimumContigLength=100;
	m_wordSize=21;
	m_colorSpaceMode=false;
	m_amos=false;
	m_error=false;
}

int Parameters::getWordSize(){
	return m_wordSize;
}

void Parameters::loadCommandsFromFile(char*file){
	ifstream f(file);
	while(!f.eof()){
		string token="";
		f>>token;
		if(token!="")
			m_commands.push_back(token);

	}
	f.close();
}

void Parameters::loadCommandsFromArguments(int argc,char**argv){
	for(int i=0;i<argc;i++){
		m_commands.push_back(argv[i]);
	}
}

void Parameters::parseCommands(){
	m_initiated=true;
	int i=0;
	set<string> commands;

	if(m_rank==MASTER_RANK){
		cout<<endl;
		cout<<"Ray command:"<<endl<<endl;

		for(int i=0;i<(int)m_commands.size();i++){
			if(i!=(int)m_commands.size()-1){
				cout<<m_commands[i]<<" \\"<<endl;
			}else{
				cout<<m_commands[i]<<endl;
			}
		}
	}

	set<string> singleReadsCommands;
	singleReadsCommands.insert("-s");
	singleReadsCommands.insert("LoadSingleEndReads");
	singleReadsCommands.insert("-LoadSingleEndReads");
	singleReadsCommands.insert("--LoadSingleEndReads");

	set<string> pairedReadsCommands;
	pairedReadsCommands.insert("-p");
	pairedReadsCommands.insert("LoadPairedEndReads");
	pairedReadsCommands.insert("-LoadPairedEndReads");
	pairedReadsCommands.insert("--LoadPairedEndReads");
	
	set<string> interleavedCommands;
	interleavedCommands.insert("-i");

	set<string> outputAmosCommands;
	outputAmosCommands.insert("-a");
	outputAmosCommands.insert("OutputAmosFile");
	outputAmosCommands.insert("-OutputAmosFile");
	outputAmosCommands.insert("--OutputAmosFile");

	set<string> outputFileCommands;
	outputFileCommands.insert("-o");
	outputFileCommands.insert("OutputFile");
	outputFileCommands.insert("-OutputFile");
	outputFileCommands.insert("--OutputFile");
	
	set<string> kmerSetting;
	kmerSetting.insert("-k");

	vector<set<string> > toAdd;
	toAdd.push_back(singleReadsCommands);
	toAdd.push_back(pairedReadsCommands);
	toAdd.push_back(outputAmosCommands);
	toAdd.push_back(outputFileCommands);
	toAdd.push_back(kmerSetting);
	toAdd.push_back(interleavedCommands);
	for(int i=0;i<(int)toAdd.size();i++){
		for(set<string>::iterator j=toAdd[i].begin();j!=toAdd[i].end();j++){
			commands.insert(*j);
		}
	}

	m_numberOfLibraries=0;

	while(i<(int)m_commands.size()){
		string token=m_commands[i];
		if(singleReadsCommands.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_singleEndReadsFile.push_back(token);
			if(token.find(".csfasta")!=string::npos){
				m_colorSpaceMode=true;
			}
			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"-s (single sequences)"<<endl;
				cout<<" Sequences: "<<token<<endl;
			}
		}else if(outputFileCommands.count(token)>0){
			i++;
			int items=m_commands.size()-i;
			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_prefix=token;
		}else if(interleavedCommands.count(token)>0){
			// make sure there is at least 4 elements left.
			int items=0;
			int k=0;
			for(int j=i+1;j<(int)m_commands.size();j++){
				string cmd=m_commands[j];
				if(commands.count(cmd)==0){
					items++;
				}else{
					break;
				}
				k++;
			}
			if(items!=1 and items!=3){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 or 3 items, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			i++;
			token=m_commands[i];
			if(token.find(".csfasta")!=string::npos){
				m_colorSpaceMode=true;
			}

			string interleavedFile=token;
			int interleavedFileIndex=m_singleEndReadsFile.size();
			m_interleavedFiles.insert(interleavedFileIndex);
			m_singleEndReadsFile.push_back(interleavedFile);

			int meanFragmentLength=0;
			int standardDeviation=0;
			#ifdef ASSERT
			assert(items==1 or items==3);
			#endif

			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"-i (paired-end interleaved sequences)"<<endl;
				cout<<" Sequences: "<<token<<endl;
			}
			if(items==3){
				i++;
				token=m_commands[i];
				meanFragmentLength=atoi(token.c_str());
				i++;
				token=m_commands[i];
				standardDeviation=atoi(token.c_str());
				if(m_rank==MASTER_RANK){
					cout<<" Average length: "<<meanFragmentLength<<endl;
					cout<<" Standard deviation: "<<standardDeviation<<endl;
				}
			}else if(items==1){// automatic detection.
				map<int,int> t;
				m_automaticLibraries.insert(m_numberOfLibraries);
				if(m_rank==MASTER_RANK){
					cout<<" Average length: auto"<<endl;
					cout<<" Standard deviation: auto"<<endl;
				}
			}else{
				#ifdef ASSERT
				assert(false);
				#endif
			}

			m_fileLibrary[interleavedFileIndex]=m_numberOfLibraries;

			m_libraryAverageLength[m_numberOfLibraries]=meanFragmentLength;
			m_libraryDeviation[m_numberOfLibraries]=standardDeviation;

			m_numberOfLibraries++;
		}else if(pairedReadsCommands.count(token)>0){
			// make sure there is at least 4 elements left.
			int items=0;
			int k=0;
			for(int j=i+1;j<(int)m_commands.size();j++){
				string cmd=m_commands[j];
				if(commands.count(cmd)==0){
					items++;
				}else{
					break;
				}
				k++;
			}
			if(items!=2 and items!=4){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 2 or 4 items, you provided "<<items<<endl;
				}
				m_error=true;
				return;
			}
			i++;
			token=m_commands[i];
			if(token.find(".csfasta")!=string::npos){
				m_colorSpaceMode=true;
			}
			string left=token;
			// add left file
			int leftFile=m_singleEndReadsFile.size();
			m_leftFiles.insert(leftFile);
			m_singleEndReadsFile.push_back(left);
			i++;
			token=m_commands[i];
			
			// add right file
			string right=token;
			int rightFile=m_singleEndReadsFile.size();
			m_rightFiles.insert(rightFile);
			m_singleEndReadsFile.push_back(right);


			int meanFragmentLength=0;
			int standardDeviation=0;
			#ifdef ASSERT
			assert(items==4 or items==2);
			#endif

			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"-p (paired-end sequences)"<<endl;
				cout<<" Left sequences: "<<left<<endl;
				cout<<" Right sequences: "<<right<<endl;
			}

			if(items==4){
				i++;
				token=m_commands[i];
				meanFragmentLength=atoi(token.c_str());
				i++;
				token=m_commands[i];
				standardDeviation=atoi(token.c_str());
				if(m_rank==MASTER_RANK){
					cout<<" Average length: "<<meanFragmentLength<<endl;
					cout<<" Standard deviation: "<<standardDeviation<<endl;
				}
			}else if(items==2){// automatic detection.
				m_automaticLibraries.insert(m_numberOfLibraries);
				if(m_rank==MASTER_RANK){
					cout<<" Average length: auto"<<endl;
					cout<<" Standard deviation: auto"<<endl;
				}
			}

			m_fileLibrary[rightFile]=m_numberOfLibraries;
			m_fileLibrary[leftFile]=m_numberOfLibraries;

			m_libraryAverageLength[m_numberOfLibraries]=meanFragmentLength;
			m_libraryDeviation[m_numberOfLibraries]=standardDeviation;

			m_numberOfLibraries++;
		}else if(outputAmosCommands.count(token)>0){
			m_amos=true;
		}else if(kmerSetting.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				if(m_rank==MASTER_RANK){
					cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				}
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_wordSize=atoi(token.c_str());
			if(m_wordSize<15){
				m_wordSize=15;
			}
			if(m_wordSize>32){
				m_wordSize=32;
			}

			if(m_rank==MASTER_RANK){
				cout<<endl;
				cout<<"-k (to set the k-mer size)"<<endl;
				cout<<" Value: "<<m_wordSize<<endl;
			}
		}
		i++;
	}

	if(m_rank==MASTER_RANK){
		cout<<endl;
		cout<<"k-mer size: "<<m_wordSize<<endl;
	}

	uint64_t result=1;
	for(int p=0;p<m_wordSize;p++){
		result*=4;
	}
	if(m_rank==MASTER_RANK){
		cout<<" --> Number of k-mers of size "<<m_wordSize<<": "<<result<<endl;
		cout<<"  *** Note: A lower k-mer size bounds the memory usage. ***"<<endl;
		cout<<endl;
	}
}

void Parameters::constructor(int argc,char**argv,int rank){
	m_rank=rank;
	if(argc==2){
		m_input=argv[1];
		loadCommandsFromFile(argv[1]);
	}else{
		loadCommandsFromArguments(argc,argv);
	}
	parseCommands();
}

bool Parameters::isInitiated(){
	return m_initiated;
}

vector<string> Parameters::getAllFiles(){
	vector<string> l;
	for(int i=0;i<(int)m_singleEndReadsFile.size();i++)
		l.push_back(m_singleEndReadsFile[i]);
	return l;
}

string Parameters::getDirectory(){
	return m_directory;
}

string Parameters::getOutputFile(){
	return getPrefix()+".fasta";
}

int Parameters::getMinimumContigLength(){
	return m_minimumContigLength;
}

bool Parameters::isLeftFile(int i){
	return m_leftFiles.count(i)>0;
}

bool Parameters::isRightFile(int i){
	return m_rightFiles.count(i)>0;
}

int Parameters::getFragmentLength(int i){
	return m_libraryAverageLength[i];
}

int Parameters::getStandardDeviation(int i){
	return m_libraryDeviation[i];
}

bool Parameters::getColorSpaceMode(){
	return m_colorSpaceMode;
}

bool Parameters::useAmos(){
	return m_amos;
}

string Parameters::getInputFile(){
	return m_input;
}


string Parameters::getParametersFile(){
	return "Ray-Parameters.txt";
}

string Parameters::getPrefix(){
	return m_prefix;
}

string Parameters::getCoverageDistributionFile(){
	return getPrefix()+".CoverageDistribution.txt";
}

string Parameters::getAmosFile(){
	return getPrefix()+".AMOS.afg";
}

vector<string> Parameters::getCommands(){
	return m_commands;
}

bool Parameters::getError(){
	return m_error;
}


void Parameters::addDistance(int library,int distance,int count){
	m_observedDistances[library][distance]+=count;
}

string Parameters::getLibraryFile(int library){
	ostringstream s;
	s<<getPrefix();
	s<<"."<<"Library"<<library<<".txt";
	return s.str();
}

void Parameters::computeAverageDistances(){
	cout<<"computeAverageDistances"<<endl;
	cout<<endl;
	for(int i=0;i<(int)m_observedDistances.size();i++){
		u64 sum=0;
		int library=i;
		int n=0;
		string fileName=getLibraryFile(library);
		ofstream f(fileName.c_str());
		for(map<int,int>::iterator j=m_observedDistances[library].begin();
			j!=m_observedDistances[library].end();j++){
			int d=j->first;
			int count=j->second;
			f<<d<<"\t"<<count<<endl;
			sum+=d*count;
			n+=count;
		}
		f.close();
		int average;
		int standardDeviation;
		if(n>0){
			average=sum/n;
			sum=0;
			for(map<int,int>::iterator j=m_observedDistances[library].begin();
				j!=m_observedDistances[library].end();j++){
				int d=j->first;
				int count=j->second;
				int diff=d-average;
				sum+=diff*diff*count;
			}
			sum/=n;
			standardDeviation=(int)sqrt(sum);
		}else{
			average=0;
			standardDeviation=0;
		}

		addLibraryData(library,average,standardDeviation);

		cout<<"Rank 0: library "<<library<<" has an average size of "<<average<<" with a standard variation of "<<standardDeviation<<endl;
	}	
	cout<<endl;
}

void Parameters::addLibraryData(int library,int average,int deviation){
	m_libraryAverageLength[library]=average;
	m_libraryDeviation[library]=deviation;
}

void Parameters::setNumberOfSequences(int n){
	m_numberOfSequencesInFile.push_back(n);
}

int Parameters::getNumberOfLibraries(){
	return m_numberOfLibraries;
}

int Parameters::getNumberOfSequences(int file){
	#ifdef ASSERT
	assert(file<(int)m_numberOfSequencesInFile.size());
	#endif
	return m_numberOfSequencesInFile[file];
}

int Parameters::getNumberOfFiles(){
	return m_singleEndReadsFile.size();
}

bool Parameters::isAutomatic(int library){
	return m_automaticLibraries.count(library)>0;
}

int Parameters::getLibrary(int file){
	return m_fileLibrary[file];
}

bool Parameters::isInterleavedFile(int i){
	return m_interleavedFiles.count(i)>0;
}

string Parameters::getReceivedMessagesFile(){
	string outputForMessages=getPrefix()+".ReceivedMessages.txt";
	return outputForMessages;
}

void Parameters::printFinalMessage(){
	for(int i=0;i<(int)m_observedDistances.size();i++){
		cout<<"Rank "<<MASTER_RANK<<" wrote "<<getLibraryFile(i)<<endl;
	}
}
