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
	commands.insert("LoadSingleEndReads");
	commands.insert("-s");
	commands.insert("--LoadSingleEndReads");
	commands.insert("-LoadSingleEndReads");

	commands.insert("LoadPairedEndReads");
	commands.insert("-p");
	commands.insert("--LoadPairedEndReads");
	commands.insert("-LoadPairedEndReads");
	
	commands.insert("OutputAmosFile");
	commands.insert("-a");
	commands.insert("--OutputAmosFile");
	commands.insert("-OutputAmosFile");

	while(i<(int)m_commands.size()){
		string token=m_commands[i];
		if(token=="LoadSingleEndReads" or token=="-s" or token=="--LoadSingleEndReads" or token=="-LoadSingleEndReads"){
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
		}else if(token=="LoadPairedEndReads" or token=="-p" or token=="--LoadPairedEndReads" or token=="-LoadPairedEndReads"){
			#ifdef DEBUG_PARAMETERS
			cout<<"OpCode="<<token<<endl;
			#endif
			i++;
			// make sure there is at least 4 elements left.
			int items=0;
			int k=0;
			for(int j=i;j<(int)m_commands.size();j++){
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
			token=m_commands[i];
			if(token.find(".csfasta")!=string::npos){
				m_colorSpaceMode=true;
			}
			string left=token;
			// add left file
			m_leftFiles.insert(m_singleEndReadsFile.size());
			m_singleEndReadsFile.push_back(left);
			i++;
			token=m_commands[i];
			
			// add right file
			string right=token;
			m_rightFiles.insert(m_singleEndReadsFile.size());
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
				meanFragmentLength=m_observedDistances.size();
				standardDeviation=_AUTOMATIC_DETECTION;
				vector<int> t;
				m_observedDistances.push_back(t);
			}else{
				#ifdef DEBUG
				assert(false);
				#endif
			}
			m_averageFragmentLengths[m_singleEndReadsFile.size()-1]=meanFragmentLength;
			m_standardDeviations[m_singleEndReadsFile.size()-1]=standardDeviation;
			#ifdef DEBUG_PARAMETERS
			cout<<"Library: "<<meanFragmentLength<<" : "<<standardDeviation<<endl;
			#endif
		}else if(token=="OutputAmosFile" or token=="--OutputAmosFile" or token=="-a" or token=="-OutputAmosFile"){
			m_amos=true;
		}
		i++;
	}

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
	return "Ray-Contigs.fasta";
}

string Parameters::getCoverageDistributionFile(){
	return "Ray-CoverageDistribution.txt";
}

string Parameters::getAmosFile(){
	return "Ray-Contigs.afg";
}

string Parameters::getEngineName(){
	return "Parallel_Ray_Engine";
}

string Parameters::getVersion(){
	return "0.0.7";
}

vector<string> Parameters::getCommands(){
	return m_commands;
}

bool Parameters::getError(){
	return m_error;
}


void Parameters::addDistance(int library,int distance){
	m_observedDistances[library].push_back(distance);
}

void Parameters::computeAverageDistances(){
	for(int i=0;i<(int)m_observedDistances.size();i++){
		u64 sum=0;
		int library=i;
		int n=m_observedDistances[i].size();
		for(int j=0;j<n;j++){
			sum+=m_observedDistances[i][j];
		}
		int average;
		int standardDeviation;
		if(n>0){
			average=sum/n;
			sum=0;
			for(int j=0;j<n;j++){
				int diff=m_observedDistances[i][j]-average;
				sum+=diff*diff;
			}
			sum/=n;
			standardDeviation=sqrt(sum);
		}else{
			average=0;
			standardDeviation=0;
		}
		m_observedAverageDistances.push_back(average);
		m_observedStandardDeviations.push_back(standardDeviation);
		#ifdef SHOW_LIBRARY_COMPUTATIONS
		cout<<"Library"<<library<<": "<<average<<","<<standardDeviation<<endl;
		#endif
	}	
}

int Parameters::getObservedAverageDistance(int library){
	return m_observedAverageDistances[library];
}

int Parameters::getObservedStandardDeviation(int library){
	return m_observedStandardDeviations[library];
}

