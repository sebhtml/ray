/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#include <scheduling/SwitchMan.h>
#include <assert.h>
#include <vector>
using namespace std;

void SwitchMan::constructor(int numberOfCores){
	m_target=numberOfCores;
	reset();
	#ifdef ASSERT
	runAssertions();
	#endif

}

void SwitchMan::reset(){
	m_counter=0;
	#ifdef ASSERT
	runAssertions();
	#endif
}

bool SwitchMan::allRanksAreReady(){
	#ifdef ASSERT
	runAssertions();
	#endif
	return m_counter==m_target;
}

void SwitchMan::addReadyRank(){
	m_counter++;

	#ifdef ASSERT
	runAssertions();
	#endif
}

void SwitchMan::runAssertions(){
	assert(m_counter<=m_target);
	assert(m_counter>=0);
}


int SwitchMan::getNextMasterMode(int currentMode){
	#ifdef ASSERT
	assert(m_switches.count(currentMode)>0);
	#endif

	return m_switches[currentMode];
}

void SwitchMan::addNextMasterMode(int a,int b){
	#ifdef ASSERT
	assert(m_switches.count(a)==0);
	#endif

	m_switches[a]=b;
}
