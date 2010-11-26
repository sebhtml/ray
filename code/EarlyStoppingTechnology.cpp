/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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


#include<set>
#include<iostream>
#include<EarlyStoppingTechnology.h>
using namespace std;

void EarlyStoppingTechnology::addDirections(vector<Direction>*directions){
	cout<<"EarlyStoppingTechnology::addDirections m_selfWave="<<m_selfWave<<endl;
	cout<<"CachedStoreSize="<<m_observations.size()<<endl;
	// for each direction, add it if it is not there, and increment the observations
	for(int i=0;i<(int)directions->size();i++){
		cout<<directions->at(i).getWave()<<" "<<directions->at(i).getProgression()<<endl;
	}
	cout<<endl;
	for(int i=0;i<(int)directions->size();i++){
		int pathWave=directions->at(i).getWave();
		int progression=directions->at(i).getProgression();

		// must follow 
		if(m_observations.count(pathWave)>0 && m_observations[pathWave][m_observations[pathWave].size()-1]!=progression-1){
			cout<<"skipping last="<< m_observations[pathWave][m_observations[pathWave].size()-1]<<" but progression="<<progression<<" pathWave="<<pathWave<<endl;
			continue;
		}

		m_observations[pathWave].push_back(progression);
		cout<<"adding progression="<<progression<<" to pathWave="<<pathWave<<endl;

		if((int)m_observations[pathWave].size()>=m_lengthThreshold && pathWave!=m_selfWave){
			cout<<"checking... pathWaveSize="<<m_observations[pathWave].size()<<" pathWave="<<pathWave<<" selfWave="<<m_selfWave<<" selfWaveSize="<<m_observations[m_selfWave].size()<<endl;
			int lastForSelf=m_observations[m_selfWave][m_observations[m_selfWave].size()-1];
			int lastForOther=m_observations[pathWave][m_observations[pathWave].size()-1];
			int distance=lastForOther-lastForSelf;
			if(distance>m_distanceThreshold){
				m_alarm=true;
			}
		}
	}
}

bool EarlyStoppingTechnology::isAlarmed(){
	// check if self-wave overlaps with another wave for at least
	// 1000 vertices.A
	//
	// current position is the last.
	if(m_alarm){
		cout<<"AH !"<<endl;
	}
	return m_alarm;
}

void EarlyStoppingTechnology::constructor(int selfWave){
	m_observations.clear();
	m_selfWave=selfWave;
	m_lengthThreshold=500;
	m_distanceThreshold=500;
	m_alarm=false;
}
