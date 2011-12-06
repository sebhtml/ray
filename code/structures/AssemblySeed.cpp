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

#include <structures/AssemblySeed.h>
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
	map<int,int> frequencies;

	for(int i=0;i<(int)m_coverageValues.size();i++){
		frequencies[m_coverageValues[i]]++;
	}

	int best=-1;

	for(map<int,int>::iterator i=frequencies.begin();
		i!=frequencies.end();i++){

		if(frequencies.count(best)==0 || i->second > frequencies[best]){
			best=i->first;
		}

		cout<<i->first<<"	"<<i->second<<endl;
	}

	cout<<"mode= "<<best<<" length= "<<m_vertices.size()<<endl;
}

int AssemblySeed::getPeakCoverage(){
	return m_peakCoverage;
}

void AssemblySeed::addCoverageValue(int value){
	m_coverageValues.push_back(value);
}
