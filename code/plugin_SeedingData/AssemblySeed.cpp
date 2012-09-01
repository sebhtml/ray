/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

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

#include <plugin_SeedingData/AssemblySeed.h>
#include <application_core/constants.h>
#include <core/statistics.h>
#include <iostream>
#include <map>
using namespace std;

AssemblySeed::AssemblySeed(){
}

int AssemblySeed::size()const{
	return m_vertices.size();
}

Kmer*AssemblySeed::at(int i){
	return &(m_vertices.at(i));
}

CoverageDepth AssemblySeed::getCoverageAt(int position){

	if(m_coverageValues.size()==0)
		return 0;

	return m_coverageValues[position];
}

void AssemblySeed::push_back(Kmer*a){
	m_vertices.push_back(*a);
}

vector<Kmer>*AssemblySeed::getVertices(){
	return &m_vertices;
}

void AssemblySeed::clear(){
	m_vertices.clear();
}

void AssemblySeed::resetCoverageValues(){
	m_coverageValues.clear();
}

void AssemblySeed::computePeakCoverage(){

	bool useMode=false;
	bool useMean=true;

	#ifdef ASSERT
	assert(m_coverageValues.size() == m_vertices.size());
	assert((useMode || useMean) && !(useMode && useMean));
	#endif

	// the default is to use the weighted mean algorithm 

	#ifdef ASSERT
	assert(useMean==true);
	assert(useMode==false);
	#endif

	if(useMode){
		computePeakCoverageUsingMode();
	}else if(useMean){
		computePeakCoverageUsingMean();
	}
}

CoverageDepth AssemblySeed::getPeakCoverage(){
	return m_peakCoverage;
}

void AssemblySeed::addCoverageValue(CoverageDepth value){
	m_coverageValues.push_back(value);
}

void AssemblySeed::computePeakCoverageUsingMode(){

	map<CoverageDepth,int> frequencies;

	for(int i=0;i<(int)m_coverageValues.size();i++){
		frequencies[m_coverageValues[i]]++;
	}

	int best=-1;

	for(map<CoverageDepth,int>::iterator i=frequencies.begin();
		i!=frequencies.end();i++){

		if(frequencies.count(best)==0 || i->second > frequencies[best]){
			best=i->first;
		}

		#ifdef CONFIG_VERBOSITY_FOR_SEEDS
		cout<<i->first<<"	"<<i->second<<endl;
		#endif
	}

	cout<<"mode= "<<best<<" length= "<<m_vertices.size()<<endl;

	m_peakCoverage=best;

}

void AssemblySeed::computePeakCoverageUsingMean(){

	map<CoverageDepth,int> frequencies;

	for(int i=0;i<(int)m_coverageValues.size();i++){
		frequencies[m_coverageValues[i]]++;
	}

	LargeCount sum=0;
	LargeCount count=0;

	for(map<CoverageDepth,int>::iterator i=frequencies.begin();
		i!=frequencies.end();i++){

		CoverageDepth coverage=i->first;
		LargeCount frequency=i->second;

		#ifdef CONFIG_VERBOSITY_FOR_SEEDS
		cout<<coverage<<"	"<<frequency<<endl;
		#endif

		sum+=coverage*frequency;
		count+=frequency;
	}

	#ifdef ASSERT
	assert(m_coverageValues.size()>=1);
	assert(count!=0);
	assert(count>0);
	assert(sum > 0);
	#endif

	CoverageDepth mean=( sum / count );

	cout<<"mean= "<<mean <<" length= "<<m_vertices.size()<<endl;

	m_peakCoverage=mean;
}
