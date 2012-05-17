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

#include <plugin_CoverageGatherer/CoverageDistribution.h>
#include <iostream>
#include <fstream>
#include <map>
#ifdef ASSERT
#include <assert.h>
#endif
using namespace std;


CoverageDistribution::CoverageDistribution(map<CoverageDepth,LargeCount>*distributionOfCoverage,string*file){
	if(file!=NULL){
		ofstream f;
		f.open(file->c_str());
		for(map<CoverageDepth,LargeCount>::iterator i=distributionOfCoverage->begin();i!=distributionOfCoverage->end();i++){
			f<<""<<i->first<<" "<<i->second<<endl;
		}
		f.close();
	}
	
	vector<CoverageDepth> x;
	vector<LargeCount> y;
	for(map<CoverageDepth,LargeCount>::iterator i=distributionOfCoverage->begin();i!=distributionOfCoverage->end();i++){
		x.push_back(i->first);
		y.push_back(i->second);
	}

	int windowSize=10;
	CoverageDepth minimumX=1;
	LargeCount minimumY=2*4096;
	LargeCount minimumY2=55000;
	CoverageDepth maximumX=65535-1;
	CoverageDepth safeThreshold=256;

	/** get the votes to find the peak */
	map<int,int> votes;
	for(int i=0;i<(int)x.size();i++){
		int largestPosition=i;
		for(int j=0;j<windowSize;j++){
			int position=i+j;
			if(position >= (int)x.size())
				break;
			if(y.at(position) > y.at(largestPosition))
				largestPosition=position;
		}
	
		if(x[largestPosition]>maximumX)
			continue;

		if(x[largestPosition]<minimumX)
			continue;
	
		if(x[largestPosition] >= safeThreshold && y[largestPosition] < minimumY2)
			continue;
		
		if(y.at(largestPosition)>minimumY)
			votes[largestPosition]++;
	}

	/** check votes */
	int largestPosition=votes.begin()->first;
	for(map<int,int>::iterator i=votes.begin();i!=votes.end();i++){
		if((i->second > votes[largestPosition] || y[i->first] > y[largestPosition]))
			largestPosition=i->first;
		//cout<<"x: "<<x[i->first]<<" votes: "<<i->second<<" y: "<<y[i->first]<<endl;
	}
	
	int minimumPosition=largestPosition;
	int i=largestPosition;
	while(i >= 0){
		if(y[i] <= y[minimumPosition])
			minimumPosition=i;
		i--;
	}

	m_minimumCoverage=x[minimumPosition];
	m_peakCoverage=x[largestPosition];

	m_repeatCoverage=2*m_peakCoverage;
	int diff=m_peakCoverage-m_minimumCoverage;
	int candidate=m_peakCoverage+diff;

	if(candidate<m_repeatCoverage)
		m_repeatCoverage=candidate;
}

int CoverageDistribution::getPeakCoverage(){
	return m_peakCoverage;
}

int CoverageDistribution::getMinimumCoverage(){
	return m_minimumCoverage;
}

int CoverageDistribution::getRepeatCoverage(){
	return m_repeatCoverage;
}
