/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#include <core/OperatingSystem.h>
#include <profiling/Profiler.h>
#include <core/slave_modes.h>
#include <assert.h>
#include <iostream>
#include <iomanip>
using namespace std;

void Profiler::constructor(bool isEnabled){
	m_enabled = isEnabled;
	m_threshold = 64;
}

void Profiler::resetStack(){
	m_timePoints.clear();
	m_functions.clear();
	m_files.clear();
	m_lines.clear();
}

void Profiler::collect(const char*function,const char*file,int line){
	m_timePoints.push_back(getThreadMicroseconds());
	m_functions.push_back(function);
	m_files.push_back(file);
	m_lines.push_back(line);
}

void Profiler::printStack(){
	if(m_timePoints.size() == 0){
		cout<<"The stack is empty."<<endl;
		return;
	}

	cout<<"Number of calls in the stack: "<<m_timePoints.size()<<endl;
	uint64_t start=0;
	uint64_t total=0;
	if(m_timePoints.size() > 0){
		start=m_timePoints[0];

		total=m_timePoints[m_timePoints.size()-1];

		if(m_timePoints.size() > 1)
			total -= start;
	}

	for(int i=0;i<(int)m_timePoints.size();i++){
		int last=i-1;
		if(last<0)
			last=0;
		uint64_t current=m_timePoints[i];
		uint64_t lastPoint=m_timePoints[last];
		//uint64_t diffWithStart=current-start;
		uint64_t diffWithLast=current-lastPoint;

		double ratio=0;

		if(total > 0)
			ratio=100.00*diffWithLast/total;

		cout<<i<<"	"<<current<<" microseconds	+ "<<diffWithLast<<" from previous ("<<setprecision(2)<<fixed<< ratio<<"%)";
		//cout<<"	+"<<diffWithStart<<" from first";
		cout<<"	in "<<m_functions[i]<<" inside "<<m_files[i]<<" at line "<<m_lines[i]<<endl;
	}
	cout<<"End of stack"<<endl;
}

bool Profiler::isEnabled(){
	return m_enabled;
}

int Profiler::getThreshold(){
	return m_threshold;
}

void Profiler::addGranularity(int mode,int microseconds){
	m_granularityValues[microseconds] ++ ;
	m_observedGranularities[mode][microseconds] ++;
}

void Profiler::printGranularities(int rank){
	cout<<"Rank "<<rank<<" granularity of processData calls"<<endl;
	for(map<int,int>::iterator i = m_granularityValues.begin();i!= m_granularityValues.end();i++){
		int microSeconds=i->first;
		int count=i->second;
		cout<<" "<<microSeconds<<" "<<count<<endl;
	}
	cout<<"Rank "<<rank<<" END of granularity of processData calls"<<endl;
	m_granularityValues.clear();
}

/* report a summary of granularities */
void Profiler::printAllGranularities(){
	cout<<"Summary of granularities"<<endl;

	for(map<int,map<int,uint64_t> >::iterator i=m_observedGranularities.begin();
		i!=m_observedGranularities.end();i++){
		cout<<"SlaveMode= "<<SLAVE_MODES[i->first]<<endl;
		cout<<"Sampled granularities:"<<endl;

		uint64_t total=0;
		for(map<int,uint64_t>::iterator j=i->second.begin();j!=i->second.end();j++){
			total += j->second;
		}

		uint64_t cumulativeValue=0;

		for(map<int,uint64_t>::iterator j=i->second.begin();j!=i->second.end();j++){
			int granularity=j->first;
			uint64_t count=j->second;
			cumulativeValue += count;
			double ratio=100.0*count;
			double ratio2=100.0*cumulativeValue;

			if(total > 0){
				ratio /= total;
				ratio2 /= total;
			}

			cout<<"	"<<granularity<<"	"<<count<<"	"<<setprecision(2)<<fixed<<ratio<<"%";
			cout<<"	CumulativeRatio: "<<setprecision(2)<<fixed<<ratio2<<"%";
			cout<<endl;
		}
		cout<<"/End of sampled granularities"<<endl;
		cout<<endl;
	}

}

void Profiler::clearGranularities(){
	m_observedGranularities.clear();
}
