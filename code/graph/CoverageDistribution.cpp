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
#ifdef ASSERT
#include <assert.h>
#endif
using namespace std;

vector<uint64_t> CoverageDistribution::smoothData(vector<uint64_t>*y){
	vector<uint64_t> output;
	int len=y->size();
	
	int window=4;
	if((int)y->size()<=window){
		return *y;
	}
	int current=0;
	int first=0;
	int last=current+window;
	if(last>len-1)
		last=len-1;

	uint64_t sum=0;
	for(int j=first;j<=last;j++)
		sum+=y->at(j);

	while(current<len){
		int weight=last-first+1;
		uint64_t average=(sum-y->at(current))/(weight-1);
		output.push_back(average);
		current++;
		int newFirst=current-window;
		if(newFirst<0)
			newFirst=0;
		if(newFirst!=first)
			sum-=y->at(first);
		first=newFirst;
		int newLast=current+window;
		if(newLast>len-1)
			newLast=len-1;
		if(newLast!=last)
			sum+=y->at(newLast);
		last=newLast;
	}
	return output;
}

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
	vector<uint64_t> smoothed=smoothData(&y);
	smoothed=smoothData(&smoothed);
	#ifdef DEBUG_CoverageDistribution
	for(int current=0;current<(int)x.size();current++){
		cout<<"AVERAGE\t"<<x.at(current)<<"\t"<<y.at(current)<<"\t"<<smoothed.at(current)<<endl;
	}
	#endif

	#ifdef ASSERT
	if(x.size()!=smoothed.size()){
		cout<<x.size()<<" vs "<<smoothed.size()<<endl;
	}
	assert(x.size()==smoothed.size());
	assert(x.size()==y.size());
	#endif
	findPeak(&x,&smoothed,&y);
}

int CoverageDistribution::getPeakCoverage(){
	return m_peakCoverage;
}

int CoverageDistribution::getMinimumCoverage(){
	return m_minimumCoverage;
}

void CoverageDistribution::findPeak(vector<int>*x,vector<uint64_t>*y,vector<uint64_t>*rawValues){
	m_minimumCoverage=1;
	m_peakCoverage=1;
	m_repeatCoverage=1;

	vector<double> derivatives;
	int n=x->size();
	derivatives.push_back(0);
	for(int i=1;i<n;i++){
		int64_t a=(y->at(i)-y->at(i-1));
		int b=(x->at(i)-x->at(i-1));
		double derivative=(0.0+a)/(0.0+b);
		#ifdef DEBUG_CoverageDistribution
		cout<<x->at(i)<<" "<<derivative<<endl;
		#endif
		derivatives.push_back(derivative);
	}
	
	vector<int> maximums;
/*
 * The number of previous derivative that must go strictly up
 */
	int parameter=2;

	for(int i=1;i<n;i++){
		int firstToVerify=i-parameter;
		if(firstToVerify<1){
			continue;
		}
		int lastToVerify=i-1;
		bool goingUp=true;
		for(int j=firstToVerify;j<=lastToVerify;j++){
			if(derivatives[j]<0){
				goingUp=false;
			}
		}
		if(!goingUp){
			continue;
		}
		if(derivatives[i-1]>0&&derivatives[i]<0){
			#ifdef DEBUG_CoverageDistribution
			cout<<"MaximumAt "<<x->at(i)<<" Self="<<derivatives[i]<<" Previous="<<derivatives[i-1]<<endl;
			#endif
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
			#ifdef DEBUG_CoverageDistribution
			cout<<"NewPeak= "<<x->at(peakI)<<endl;
			#endif
		}
	}

	/* refine the peak */
	while(peakI-1>=0 && peakI-1<(int)rawValues->size() &&rawValues->at(peakI-1)>rawValues->at(peakI))
		peakI--;

	while(peakI+1>=0 && peakI+1<(int)rawValues->size() &&rawValues->at(peakI+1)>rawValues->at(peakI))
		peakI++;

	int minI=peakI;
	for(int i=0;i<peakI;i++){
		if(y->at(i)<y->at(minI)){
			minI=i;
		}
	}


	m_peakCoverage=x->at(peakI);
	m_minimumCoverage=x->at(minI);
	m_repeatCoverage=2*m_peakCoverage;
}

int CoverageDistribution::getRepeatCoverage(){
	return m_repeatCoverage;
}
