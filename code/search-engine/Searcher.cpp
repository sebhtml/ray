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

#include <search-engine/Searcher.h>

void Searcher::constructor(Parameters*parameters,StaticVector*outbox,int*masterMode,TimePrinter*timePrinter,SwitchMan*switchMan,
	int*slaveMode){
	m_countElementsSlaveStarted=false;
	m_countElementsMasterStarted=false;

	m_outbox=outbox;
	m_parameters=parameters;
	m_masterMode=masterMode;
	m_timePrinter=timePrinter;
	m_switchMan=switchMan;
	m_slaveMode=slaveMode;
}

void Searcher::countElements_masterMethod(){

	if(!m_countElementsMasterStarted){
		m_countElementsMasterStarted=true;

		cout<<"Opening master mode outbox= "<<m_outbox<<endl;

		m_switchMan->openMasterMode(RAY_MPI_TAG_COUNT_SEARCH_ELEMENTS,m_outbox,m_parameters->getRank());
	}

	if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode(m_masterMode);

		m_timePrinter->printElapsedTime("Counting search category elements");
		cout<<endl;
	}
}

void Searcher::countElements_slaveMethod(){

	if(!m_countElementsSlaveStarted){

		m_countElementsSlaveStarted=true;

		cout<<"Searcher, slave step "<<endl;

		m_switchMan->closeSlaveModeLocally(m_outbox,m_slaveMode,m_parameters->getRank());
	}
}

