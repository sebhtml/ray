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

#include <profiling/Derivative.h>
#include <core/statistics.h>
#include <core/slave_modes.h>
#include <core/OperatingSystem.h>


Derivative::Derivative(){
}

void Derivative::addX(int x){
	m_xValues.push_back(x);
	m_timeValues.push_back(getMicroseconds());
}

int Derivative::getLastSlope(){
	if(m_xValues.size()<2)
		return 0;

	int n=m_xValues.size();

	int slope=1000*1000*(m_xValues[n-1]-m_xValues[n-2]+0.0)/(m_timeValues[n-1]-m_timeValues[n-2]+0.0);

	if(slope<0)
		return 0;

	return slope;
}

void Derivative::printStatus(const char*mode,int modeIdentifier){
	if(m_xValues.size()<2)
		return;

	int value=getLastSlope();

	cout<<"Speed "<<mode<<" "<<value<<" units/second"<<endl;

	m_data[modeIdentifier].push_back(value);
}

void Derivative::clear(){
	m_xValues.clear();
	m_timeValues.clear();
}

void Derivative::printEstimatedTime(int total){
	int n=m_xValues.size();

	if(m_xValues.size()<2)
		return;

	int last=m_xValues[n-1];

	int remaining=total-last;

	double slope=getLastSlope();

	int remainingSeconds=remaining/slope;

	cout<<"Estimated remaining time for this step: ";

	printTheSeconds(remainingSeconds,&cout);

	cout<<endl;
}

void Derivative::writeFile(ostream*f){
	for(map<int,vector<int> >::iterator i=m_data.begin();
		i!=m_data.end();i++){
		int average=getAverage(&(i->second));
		*f<<"AverageSpeed "<<SLAVE_MODES[i->first]<<"	"<<average<<" units/second"<<endl;
	}
}
