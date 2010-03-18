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
	m_outputFile="Contigs.fasta";
	m_colorSpaceMode=false;
	m_amos=false;
}

int Parameters::getWordSize(){
	return m_wordSize;
}

void Parameters::load(string file){
	ifstream f(file.c_str());
	m_input=file;
	while(!f.eof()){
		string token;
		f>>token;
		if(token=="LoadSingleEndReads"){
			f>>token;
			m_singleEndReadsFile.push_back(token);
			if(token.find(".csfasta")!=string::npos){
				m_colorSpaceMode=true;
			}
		}else if(token=="LoadPairedEndReads"){
			f>>token;
			if(token.find(".csfasta")!=string::npos){
				m_colorSpaceMode=true;
			}
			string left=token;
			// add left file
			m_leftFiles.insert(m_singleEndReadsFile.size());
			m_singleEndReadsFile.push_back(left);
			f>>token;
			// add right file
			string right=token;
			m_rightFiles.insert(m_singleEndReadsFile.size());
			m_singleEndReadsFile.push_back(right);
			int meanFragmentLength;
			int standardDeviation;
			f>>meanFragmentLength>>standardDeviation;
			m_averageFragmentLengths[m_singleEndReadsFile.size()-1]=meanFragmentLength;
			m_standardDeviations[m_singleEndReadsFile.size()-1]=standardDeviation;
		}else if(token=="SetOutputDirectory"){
			f>>token;
			m_directory=token;
		}else if(token=="SetWordSize"){
			f>>token;
			m_wordSize=atoi(token.c_str());
		}else if(token=="OutputAmosFile"){
			m_amos=true;
		}
	}
	f.close();

	m_initiated=true;
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
	return m_outputFile;
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

