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
}

int Parameters::getWordSize(){
	return m_wordSize;
}

void Parameters::loadCommandsFromFile(char*file){
	ifstream f(file);
	while(!f.eof()){
		string token;
		f>>token;
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
	int i=0;
	while(i<(int)m_commands.size()){
		string token=m_commands[i];
		if(token=="LoadSingleEndReads" or token=="-s" or token=="--LoadSingleEndReads" or token=="-LoadSingleEndReads"){
			i++;
			token=m_commands[i];
			m_singleEndReadsFile.push_back(token);
			if(token.find(".csfasta")!=string::npos){
				m_colorSpaceMode=true;
			}
		}else if(token=="LoadPairedEndReads" or token=="-p" or token=="--LoadPairedEndReads" or token=="-LoadPairedEndReads"){
			i++;
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
			int meanFragmentLength;
			int standardDeviation;
			i++;
			token=m_commands[i];
			meanFragmentLength=atoi(token.c_str());
			i++;
			token=m_commands[i];
			standardDeviation=atoi(token.c_str());
			m_averageFragmentLengths[m_singleEndReadsFile.size()-1]=meanFragmentLength;
			m_standardDeviations[m_singleEndReadsFile.size()-1]=standardDeviation;
		}else if(token=="SetOutputDirectory"){
			i++;
			token=m_commands[i];
			m_directory=token;
		}else if(token=="SetWordSize" or token=="--SetWordSize" or token=="-w" or token=="-SetWordSize"){
			i++;
			token=m_commands[i];
			m_wordSize=atoi(token.c_str());
		}else if(token=="OutputAmosFile" or token=="--OutputAmosFile" or token=="-a" or token=="-OutputAmosFile"){
			m_amos=true;
		}
		i++;
	}

	m_initiated=true;
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
