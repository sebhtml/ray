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

#ifdef DEBUG
#include<assert.h>
#endif
#include<math.h>
#include<Parameters.h>
#include<string>
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
	m_contigsFile="Ray-Contigs.fasta";
	m_amosFile="Ray-Contigs.afg";
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

	#ifdef DEBUG_PARAMETERS
	for(int i=0;i<(int)m_commands.size();i++){
		cout<<i<<" '"<<m_commands[i]<<"'"<<endl;
	}

	#endif

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
	for(int i=0;i<(int)toAdd.size();i++)
		for(set<string>::iterator j=toAdd[i].begin();j!=toAdd[i].end();j++)
			commands.insert(*j);

	while(i<(int)m_commands.size()){
		string token=m_commands[i];
		if(singleReadsCommands.count(token)>0){
			#ifdef DEBUG_PARAMETERS
			cout<<"OpCode="<<token<<endl;
			#endif
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_singleEndReadsFile.push_back(token);
			if(token.find(".csfasta")!=string::npos){
				m_colorSpaceMode=true;
			}
			cout<<endl;
			cout<<"LoadSingleEndReads"<<endl;
			cout<<" Sequences: "<<token<<endl;
		}else if(outputFileCommands.count(token)>0){
			i++;
			int items=m_commands.size()-i;
			if(items<1){
				cout<<"Error: "<<token<<" needs 1 item, you provided "<<items<<endl;
				m_error=true;
				return;
			}
			token=m_commands[i];
			m_contigsFile=token;
		}else if(pairedReadsCommands.count(token)>0){
			#ifdef DEBUG_PARAMETERS
			cout<<"OpCode="<<token<<endl;
			#endif
			// make sure there is at least 4 elements left.
			int items=0;
			int k=0;
			for(int j=i+1;j<(int)m_commands.size();j++){
				string cmd=m_commands[j];
				if(commands.count(cmd)==0){
					#ifdef DEBUG_PARAMETERS
					cout<<"Option"<<k<<"="<<"'"<<cmd<<"'"<<endl;
					#endif
					items++;
				}else{
					break;
				}
				k++;
			}
			#ifdef DEBUG_PARAMETERS
			cout<<"Left: "<<items<<endl;
			#endif
			if(items!=2 and items!=4){
				cout<<"Error: "<<token<<" needs 2 or 4 items, you provided "<<items<<endl;
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
			#ifdef DEBUG
			assert(items==4 or items==2);
			#endif
			if(items==4){
				#ifdef DEBUG_PARAMETERS
				cout<<"PairedMode: UserProvidedDistance"<<endl;
				#endif
				i++;
				token=m_commands[i];
				meanFragmentLength=atoi(token.c_str());
				i++;
				token=m_commands[i];
				standardDeviation=atoi(token.c_str());
			}else if(items==2){// automatic detection.
				#ifdef DEBUG_PARAMETERS
				cout<<"PairedMode: AutomaticDistanceDetection"<<endl;
				#endif
				int library=m_observedDistances.size();
				meanFragmentLength=library;
				standardDeviation=_AUTOMATIC_DETECTION;
				map<int,int> t;
				m_automaticRightFiles[rightFile]=library;
				m_observedDistances.push_back(t);
			}else{
				#ifdef DEBUG
				assert(false);
				#endif
			}
			m_averageFragmentLengths[m_singleEndReadsFile.size()-1]=meanFragmentLength;
			m_standardDeviations[m_singleEndReadsFile.size()-1]=standardDeviation;
			cout<<endl;
			cout<<"LoadPairedEndReads"<<endl;
			cout<<" Left sequences: "<<left<<endl;
			cout<<" Right sequences: "<<right<<endl;
			if(items==4){
				cout<<" Average length: "<<meanFragmentLength<<endl;
				cout<<" Standard deviation: "<<standardDeviation<<endl;
			}else if(items==2){
				cout<<" Average length: auto"<<endl;
				cout<<" Standard deviation: auto"<<endl;
			}
			#ifdef DEBUG_PARAMETERS
			cout<<"Library: "<<meanFragmentLength<<" : "<<standardDeviation<<endl;
			#endif
		}else if(outputAmosCommands.count(token)>0){
			int items=0;
			int k=0;
			for(int j=i+1;j<(int)m_commands.size();j++){
				string cmd=m_commands[j];
				if(commands.count(cmd)==0){
					#ifdef DEBUG_PARAMETERS
					cout<<"Option"<<k<<"="<<"'"<<cmd<<"'"<<endl;
					#endif
					items++;
				}else{
					break;
				}
				k++;
			}
			if(items==0){
				m_amos=true;
			}else if(items==1){
				m_amos=true;
				i++;
				token=m_commands[i];
				m_amosFile=token;
			}else{
				cout<<"Error: "<<token<<" needs 0 or 1 item, you provided "<<items<<endl;
				m_error=true;
				return;
			}
		}else if(kmerSetting.count(token)>0){
			i++;
			int items=m_commands.size()-i;

			if(items<1){
				cout<<"Error: "<<token<<" needs 1 item, you provided only "<<items<<endl;
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
			cout<<endl;
			cout<<"-k (to set the k-mer size)"<<endl;
			cout<<" Value: "<<m_wordSize<<endl;

		}
		i++;
	}

	cout<<endl;
	cout<<"k-mer size: "<<m_wordSize<<endl;
	cout<<endl;

}

void Parameters::load(int argc,char**argv){
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
	return getContigsFile();
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
	return m_averageFragmentLengths[i];
}

int Parameters::getStandardDeviation(int i){
	return m_standardDeviations[i];
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

string Parameters::getContigsFile(){
	return m_contigsFile;
}

string Parameters::getCoverageDistributionFile(){
	return getContigsFile()+"-TheCoverageDistribution.tab";
}

string Parameters::getAmosFile(){
	return m_amosFile;
}

string Parameters::getEngineName(){
	return "Ray";
}

string Parameters::getVersion(){
	return "0.1.0 codename 'Adenine'";
}

vector<string> Parameters::getCommands(){
	return m_commands;
}

bool Parameters::getError(){
	return m_error;
}


void Parameters::addDistance(int library,int distance,int count){
	m_observedDistances[library][distance]=count;
}

void Parameters::computeAverageDistances(){
	for(int i=0;i<(int)m_observedDistances.size();i++){
		u64 sum=0;
		int library=i;
		#ifdef DUMP_LIBRARIES
		cout<<"LIBRARY"<<i<<endl;
		#endif
		int n=0;
		for(map<int,int>::iterator j=m_observedDistances[library].begin();
			j!=m_observedDistances[library].end();j++){
			int d=j->first;
			int count=j->second;
			#ifdef DUMP_LIBRARIES
			cout<<d<<"	"<<count<<endl;
			#endif
			sum+=d*count;
			n+=count;
		}
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

		m_observedAverageDistances.push_back(average);
		m_observedStandardDeviations.push_back(standardDeviation);
		#define SHOW_LIBRARY_COMPUTATIONS
		#ifdef SHOW_LIBRARY_COMPUTATIONS
		cout<<"Rank 0: Library"<<library<<": "<<average<<","<<standardDeviation<<endl;
		#endif
	}	
	m_observedDistances.clear();
}

int Parameters::getObservedAverageDistance(int library){
	return m_observedAverageDistances[library];
}

int Parameters::getObservedStandardDeviation(int library){
	return m_observedStandardDeviations[library];
}

void Parameters::setNumberOfSequences(int n){
	m_numberOfSequencesInFile.push_back(n);
}

int Parameters::getNumberOfSequences(int file){
	#ifdef DEBUG
	assert(file<(int)m_numberOfSequencesInFile.size());
	#endif
	return m_numberOfSequencesInFile[file];
}

int Parameters::getNumberOfFiles(){
	return m_singleEndReadsFile.size();
}

bool Parameters::isAutomatic(int file){
	return m_automaticRightFiles.count(file)>0;
}

int Parameters::getLibrary(int file){
	return m_automaticRightFiles[file];
}
