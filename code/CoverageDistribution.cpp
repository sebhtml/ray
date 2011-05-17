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

#include <CoverageDistribution.h>
#include <iostream>
#include <fstream>
#include <map>
#include <math.h>
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

	m_minimumCoverage=1;
	m_peakCoverage=1;
	
	vector<int> x;
	vector<uint64_t> y;
	for(map<int,uint64_t>::iterator i=distributionOfCoverage->begin();i!=distributionOfCoverage->end();i++){
		x.push_back(i->first);
		y.push_back(i->second);
	}

	FindPeak(&x,&y,&m_minimumCoverage,&m_peakCoverage);
}

int CoverageDistribution::getPeakCoverage(){
	return m_peakCoverage;
}

int CoverageDistribution::getMinimumCoverage(){
	return m_minimumCoverage;
}

void CoverageDistribution::FindPeak(vector<int>*x,vector<uint64_t>*y,int*minimumCoverage,int*peakCoverage){
	// compute derivatives
	vector<double> derivatives;
	for(int i=0;i<(int)x->size()-1;i++){
		uint64_t xi=x->at(i);
		int64_t yi=y->at(i);
		int64_t xi1=x->at(i+1);
		int64_t yi1=y->at(i+1);
		int64_t dy=yi1-yi;
		uint64_t dx=xi1-xi;
		double derivative=(0.0+dy)/(0.0+dx);
		derivatives.push_back(derivative);
		//cout<<i<<" "<<xi<<" "<<yi<<" "<<dx<<" "<<dy<<" "<<derivative<<endl;
	}

	// find the peak with custom scores	
	uint64_t maxScore=0;
	int bestI=0;
	for(int i=0;i<(int)derivatives.size()-1;i++){
		int64_t yi=y->at(i);
		int step=256;

		// compute the left score
		uint64_t leftScore=1;
		int j=i-1;
		int min=i-step;
		if(min<0){
			min=0;
		}
		int o=1;
		while(j>=min){
			double derivative=derivatives[j];
			if(derivative>0){
				o++;
			}else{
				o=1;
			}
			leftScore+=o*o;
			j--;
		}
		
		// compute the right score
		uint64_t rightScore=1;
		j=i+1;
		int max=i+step;
		if(max>(int)derivatives.size()-1){
			max=derivatives.size()-1;
		}
		o=1;
		while(j<=max){
			double derivative=derivatives[j];
			if(derivative<0){
				o++;
			}else{
				o=1;
			}
			rightScore+=o*o;
			j++;

		}
		uint64_t score=(uint64_t)(log(yi)*leftScore*rightScore);
		//cout<<i<<" "<<leftScore<<" "<<rightScore<<" "<<score<<endl;

		if(score>maxScore){
			maxScore=score;
			bestI=i;
		}
	}

	int minI=bestI;
	int i=bestI;
	uint64_t minValue=y->at(minI);
	while(i>=0){
		uint64_t yi=y->at(i);
		if(yi<minValue){
			minValue=yi;
			minI=i;
		}
		i--;
	}
	*minimumCoverage=x->at(minI);
	*peakCoverage=x->at(bestI);
}

