/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

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

#include <core/MachineHelper.h>
#include <communication/mpi_tags.h>
#include <communication/Message.h>

void MachineHelper::call_RAY_MASTER_MODE_LOAD_CONFIG(){

	if(m_argc==2 && m_argv[1][0]!='-'){
		ifstream f(m_argv[1]);
		if(!f){
			cout<<"Rank "<<m_parameters->getRank()<<" invalid input file."<<endl;
			m_parameters->showUsage();
			(*m_aborted)=true;
			f.close();
			m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
			return;
		}
	}

	if(m_parameters->getError()){
		(*m_aborted)=true;
		m_switchMan->setMasterMode(RAY_MASTER_MODE_KILL_ALL_MPI_RANKS);
		return;
	}

	uint64_t*message=(uint64_t*)m_outboxAllocator->allocate(2*sizeof(uint64_t));
	message[0]=m_parameters->getWordSize();
	message[1]=m_parameters->getColorSpaceMode();

	for(int i=0;i<m_parameters->getSize();i++){
		Message aMessage(message,2,i,RAY_MPI_TAG_SET_WORD_SIZE,m_parameters->getRank());
		m_outbox->push_back(aMessage);
	}

	m_switchMan->setMasterMode(RAY_MASTER_MODE_TEST_NETWORK);
}

void MachineHelper::constructor(int argc,char**argv,Parameters*parameters,
SwitchMan*switchMan,RingAllocator*outboxAllocator,
		StaticVector*outbox,bool*aborted
){
	m_argc=argc;
	m_argv=argv;
	m_parameters=parameters;

	m_switchMan=switchMan;
	m_outboxAllocator=outboxAllocator;
	m_outbox=outbox;

	m_aborted=aborted;
}
