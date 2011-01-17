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

#include<set>
#include<iostream>
#include<EarlyStoppingTechnology.h>
using namespace std;

void EarlyStoppingTechnology::addDirections(vector<Direction>*directions){
	forwardTechnology(directions);
	reverseTechnology(directions);
}

/*
 *    --------------------------------------------------------------->
 *                                          <-----------------------------------------------------------
 */
void EarlyStoppingTechnology::reverseTechnology(vector<Direction>*directions){ 
	for(int i=0;i<(int)directions->size();i++){
		uint64_t pathWave=directions->at(i).getWave();
		int progression=directions->at(i).getProgression();

		// must follow 
		if(m_reverseObservations.count(pathWave)>0 
		&& ((m_reverseObservations[pathWave][m_reverseObservations[pathWave].size()-1]!=progression+1))){
			continue;
		}

		m_reverseObservations[pathWave].push_back(progression);

		if((int)m_reverseObservations[pathWave].size()>=m_reverseThreshold
		&& pathWave > m_selfWave){
			//cout<<"Rank "<<m_rank<<": Ray Early-Stopping Technology was triggered, Case 3: Forward-Reverse."<<endl;
			m_alarm=true;
		}
	}
}

/*
 *
 *   --------------------------------------------------------------------->
 *   						--------------------------------------------->
 *
 */
void EarlyStoppingTechnology::forwardTechnology(vector<Direction>*directions){
	for(int i=0;i<(int)directions->size();i++){
		int pathWave=directions->at(i).getWave();
		int progression=directions->at(i).getProgression();

		// must follow 
		if(m_forwardObservations.count(pathWave)>0 
		&& ((m_forwardObservations[pathWave][m_forwardObservations[pathWave].size()-1]!=progression-1))){
			continue;
		}

		// must start at 0
		if(m_forwardObservations.count(pathWave)==0 && progression!=0){
			continue;
		}

		m_forwardObservations[pathWave].push_back(progression);

		if((int)m_forwardObservations[pathWave].size()>=m_forwardThreshold){
			//cout<<"Rank "<<m_rank<<": Ray Early-Stopping Technology was triggered, Case 2: Forward-Forward ."<<endl;
			m_alarm=true;
		}
	}
}

bool EarlyStoppingTechnology::isAlarmed(){
	// check if self-wave overlaps with another wave for at least
	// 1000 vertices.A
	//
	// current position is the last.
	return false;
	return m_alarm;
}

void EarlyStoppingTechnology::constructor(uint64_t selfWave,int rank){
	m_rank=rank;
	m_forwardObservations.clear();
	m_reverseObservations.clear();
	m_selfWave=selfWave;
	m_forwardThreshold=1000000; // overlap threshold is 1000
	m_reverseThreshold=1000000;
	m_alarm=false;
}
