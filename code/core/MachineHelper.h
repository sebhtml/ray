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

#ifndef _MachineHelper_h
#define _MachineHelper_h

#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <scheduling/SwitchMan.h>

/** \author Sébastien Boisvert */
class MachineHelper{
	int m_argc;
	char**m_argv;
	Parameters*m_parameters;
	SwitchMan*m_switchMan;
	RingAllocator*m_outboxAllocator;
	StaticVector*m_outbox;
	bool*m_aborted;
public:
	void constructor(int argc,char**argv,Parameters*parameters,
		SwitchMan*switchMan,RingAllocator*outboxAllocator,
		StaticVector*outbox,bool*aborted);

	void call_RAY_MASTER_MODE_LOAD_CONFIG();
};

#endif

