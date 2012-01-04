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

#ifndef _Searcher_h
#define _Searcher_h

#include <core/Parameters.h>
#include <scheduling/SwitchMan.h>
#include <communication/VirtualCommunicator.h>
#include <memory/RingAllocator.h>
#include <structures/StaticVector.h>
#include <assembler/TimePrinter.h>
#include <vector>
#include <string>

/**
 * This class searches for sequences in the de Bruijn graph
 * It outputs biological abundance readouts
 **/
class Searcher{
	Parameters*m_parameters;
	SwitchMan*m_switchMan;
	VirtualCommunicator*m_virtualCommunicator;
	StaticVector*m_outbox;
	StaticVector*m_inbox;
	RingAllocator*m_outboxAllocator;
	TimePrinter*m_timePrinter;

	vector<string> categoryFiles;

	bool m_countElementsMasterStarted;
	bool m_countElementsSlaveStarted;
public:

	void countElements_masterMethod();
	void countElements_slaveMethod();

	void constructor(Parameters*parameters,StaticVector*outbox,TimePrinter*timePrinter,SwitchMan*switchMan);
};

#endif

