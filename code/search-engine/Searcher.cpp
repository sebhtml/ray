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

void Searcher::constructor(Parameters*parameters,StaticVector*outbox,TimePrinter*timePrinter,SwitchMan*switchMan){
	m_countElementsSlaveStarted=false;
	m_countElementsMasterStarted=false;
	
	m_countContigKmersMasterStarted=false;
	m_countContigKmersSlaveStarted=false;

	m_outbox=outbox;
	m_parameters=parameters;
	m_timePrinter=timePrinter;
	m_switchMan=switchMan;
}

void Searcher::countElements_masterMethod(){

	if(!m_countElementsMasterStarted){
		m_countElementsMasterStarted=true;

		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());
	}

	if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();

		m_timePrinter->printElapsedTime("Counting search category sequences");
		cout<<endl;
	}
}

void Searcher::countElements_slaveMethod(){

	if(!m_countElementsSlaveStarted){

		m_countElementsSlaveStarted=true;

		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());
	}
}

void Searcher::countContigKmers_masterHandler(){

	if(!m_countContigKmersMasterStarted){
		m_countContigKmersMasterStarted=true;

		m_switchMan->openMasterMode(m_outbox,m_parameters->getRank());
	}

	if(m_switchMan->allRanksAreReady()){
		m_switchMan->closeMasterMode();

		m_timePrinter->printElapsedTime("Counting contig biological abundances");
		cout<<endl;
	}
}

void Searcher::countContigKmers_slaveHandler(){

	if(!m_countContigKmersSlaveStarted){

		m_countElementsSlaveStarted=true;

		m_switchMan->closeSlaveModeLocally(m_outbox,m_parameters->getRank());
	}
}

