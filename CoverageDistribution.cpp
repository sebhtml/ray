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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/


#include<CoverageDistribution.h>
#include<iostream>
#include<fstream>
#include<map>
using namespace std;

CoverageDistribution::CoverageDistribution(map<int,VERTEX_TYPE>distributionOfCoverage,string m_assemblyDirectory){
	m_coverage_mean=100;
	m_minimumCoverage=1;

	for(map<int,VERTEX_TYPE>::iterator i=distributionOfCoverage.begin();i!=distributionOfCoverage.end();i++){
		#ifdef SHOW_PROGRESS
		cout<<"DISTRIBUTION "<<i->first<<" "<<i->second<<endl;
		#endif
		int coverage=i->first;
		if(
		coverage!=1&&
		(distributionOfCoverage.count(coverage-1)==0||
			distributionOfCoverage[coverage-1]<=distributionOfCoverage[coverage]) &&
		(distributionOfCoverage.count(coverage+1)==0||
			distributionOfCoverage[coverage+1]<=distributionOfCoverage[coverage]) &&
		distributionOfCoverage[coverage]>distributionOfCoverage[m_coverage_mean]){
			m_coverage_mean=coverage;
		}
	}

	for(int coverage=1;coverage<=m_coverage_mean;coverage++){
		if(
		distributionOfCoverage.count(coverage-1)>0 &&
			distributionOfCoverage[coverage-1]>=distributionOfCoverage[coverage] &&
		distributionOfCoverage.count(coverage+1)>0 &&
			distributionOfCoverage[coverage+1]>=distributionOfCoverage[coverage] &&

		distributionOfCoverage[coverage]<distributionOfCoverage[m_minimumCoverage]&&
		coverage < m_coverage_mean &&
		(m_minimumCoverage==1|| coverage<m_minimumCoverage)){
			m_minimumCoverage=coverage;
		}
	}

}

int CoverageDistribution::getPeakCoverage(){
	return m_coverage_mean;
}

int CoverageDistribution::getMinimumCoverage(){
	return m_minimumCoverage;
}

