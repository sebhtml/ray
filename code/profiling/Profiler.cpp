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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include <core/OperatingSystem.h>
#include <profiling/Profiler.h>
#include <core/slave_modes.h>
#include <assert.h>
#include <iostream>
#include <iomanip>
using namespace std;

void Profiler::constructor(){
}

void Profiler::resetStack(){
	m_timePoints.clear();
	m_functions.clear();
	m_files.clear();
	m_lines.clear();
}

void Profiler::collect(const char*function,const char*file,int line){
	m_timePoints.push_back(getMicroSecondsInOne());
	m_functions.push_back(function);
	m_files.push_back(file);
	m_lines.push_back(line);
}

void Profiler::printStack(){
	cout<<"Number of calls in the stack: "<<m_timePoints.size()<<endl;
	uint64_t start=0;
	uint64_t total=0;
	if(m_timePoints.size() > 0){
		uint64_t start=m_timePoints[0];

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
		uint64_t diffWithStart=current-start;
		uint64_t diffWithLast=current-lastPoint;

		double ratio=0;

		if(total > 0)
			ratio=100.00*diffWithLast/total;

		cout<<i<<"	"<<current<<" microseconds	+"<<diffWithLast<<" from previous ("<<setprecision(2)<<fixed<< ratio<<"%)";
		cout<<"	+"<<diffWithStart<<" from first	in "<<m_functions[i]<<" inside "<<m_files[i]<<" at line "<<m_lines[i]<<endl;
	}
}
