/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

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


#include<CoverageDistribution.h>
#include<iostream>
#include<fstream>
#include<map>
using namespace std;

CoverageDistribution::CoverageDistribution(map<int,VERTEX_TYPE>*distributionOfCoverage,string*file){
	COVERAGE_TYPE max=0;
	max--;

	m_peakCoverage=100;
	m_minimumCoverage=1;
	ofstream f(file->c_str());

	for(map<int,VERTEX_TYPE>::iterator i=distributionOfCoverage->begin();i!=distributionOfCoverage->end();i++){
		f<<""<<i->first<<" "<<i->second<<endl;
		int coverage=i->first;
		if(coverage==max)
			continue;
		if(
		coverage!=1&&
		(distributionOfCoverage->count(coverage-1)==0||
			(*distributionOfCoverage)[coverage-1]<=(*distributionOfCoverage)[coverage]) &&
		(distributionOfCoverage->count(coverage+1)==0||
			(*distributionOfCoverage)[coverage+1]<=(*distributionOfCoverage)[coverage]) &&
		(*distributionOfCoverage)[coverage]>(*distributionOfCoverage)[m_peakCoverage]){
			m_peakCoverage=coverage;
		}
	}

	for(int coverage=1;coverage<=m_peakCoverage;coverage++){
		if(
		distributionOfCoverage->count(coverage-1)>0 &&
			(*distributionOfCoverage)[coverage-1]>=(*distributionOfCoverage)[coverage] &&
		distributionOfCoverage->count(coverage+1)>0 &&
			(*distributionOfCoverage)[coverage+1]>=(*distributionOfCoverage)[coverage] &&

		(*distributionOfCoverage)[coverage]<(*distributionOfCoverage)[m_minimumCoverage]&&
		coverage < m_peakCoverage&&
		(m_minimumCoverage==1|| coverage<m_minimumCoverage)){
			m_minimumCoverage=coverage;
		}
	}

	if((*distributionOfCoverage)[m_peakCoverage]==0){
		m_peakCoverage=0;
	}
	cout<<"Rank 0 informs you that MinimumCoverage="<<m_minimumCoverage<<endl;
	cout<<"Rank 0 informs you that PeakCoverage="<<m_peakCoverage<<endl;

	f.close();
}

int CoverageDistribution::getPeakCoverage(){
	return m_peakCoverage;
}

int CoverageDistribution::getMinimumCoverage(){
	return m_minimumCoverage;
}

