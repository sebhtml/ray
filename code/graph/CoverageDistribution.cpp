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

#include <graph/CoverageDistribution.h>
#include <iostream>
#include <fstream>
#include <map>
using namespace std;

CoverageDistribution::CoverageDistribution(map<int,uint64_t>*distributionOfCoverage,string*file){
	if(file!=NULL){
		ofstream f;
		f.open(file->c_str());
		for(map<int,uint64_t>::iterator i=distributionOfCoverage->begin();i!=distributionOfCoverage->end();i++){
			f<<""<<i->first<<" "<<i->second<<endl;
		}
		f.close();
	}
	
	vector<int> x;
	vector<uint64_t> y;
	for(map<int,uint64_t>::iterator i=distributionOfCoverage->begin();i!=distributionOfCoverage->end();i++){
		x.push_back(i->first);
		y.push_back(i->second);
	}

	findPeak(&x,&y);
}

int CoverageDistribution::getPeakCoverage(){
	return m_peakCoverage;
}

int CoverageDistribution::getMinimumCoverage(){
	return m_minimumCoverage;
}

void CoverageDistribution::findPeak(vector<int>*x,vector<uint64_t>*y){
	m_minimumCoverage=1;
	m_peakCoverage=1;
	m_repeatCoverage=1;

	vector<double> derivatives;
	int n=x->size();
	derivatives.push_back(0);
	for(int i=1;i<n;i++){
		int a=(y->at(i)-y->at(i-1));
		int b=(x->at(i)-x->at(i-1));
		double derivative=(0.0+a)/(0.0+b);
		//cout<<x->at(i)<<" "<<derivative<<endl;
		derivatives.push_back(derivative);
	}
	
	vector<int> maximums;

	for(int i=1;i<n;i++){
		if(x->at(i)<4){
			continue;
		}
		if(derivatives[i-1]>0&&derivatives[i]<0){
			//cout<<"MaximumAt "<<x->at(i)<<" Self="<<derivatives[i]<<" Previous="<<derivatives[i-1]<<endl;
			maximums.push_back(i);
		}
	}
	if(maximums.size()==0){
		return;
	}
	int peakI=maximums[0];
	for(int i=0;i<(int)maximums.size();i++){
		if(y->at(maximums[i])>y->at(peakI)){
			peakI=maximums[i];
			//cout<<"NewPeak= "<<x->at(peakI)<<endl;
		}
	}
	int minI=peakI;
	for(int i=0;i<peakI;i++){
		if(y->at(i)<y->at(minI)){
			minI=i;
		}
	}
	int maxI=peakI;
	for(int i=peakI;i<n;i++){
		if(y->at(i)<y->at(minI)/2){
			maxI=i;
			break;
		}else if(derivatives[i]>0){
			maxI=i;
			break;
		}
	}
	m_peakCoverage=x->at(peakI);
	m_minimumCoverage=x->at(minI);
	m_repeatCoverage=x->at(maxI);
}

int CoverageDistribution::getRepeatCoverage(){
	return m_repeatCoverage;
}
