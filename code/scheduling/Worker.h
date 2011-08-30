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

#ifndef _Worker_h
#define _Worker_h

#include <stdint.h>
#include <core/Parameters.h>
#include <memory/RingAllocator.h>
#include <communication/VirtualCommunicator.h>

/** a general worker class 
 * \author Sébastien Boisvert
 */
class Worker{

/* protected means things that inherit Worker can also use them */
protected:


	/** flag indicating if the worker is done */
	bool m_isDone;

	/** global Ray run-time parameters */
	Parameters*m_parameters;

	/** the worker identifier */
	uint64_t m_workerId;

	/** the ring allocator to send messages */
	RingAllocator*m_outboxAllocator;

	/** the virtual communicator */
	VirtualCommunicator*m_virtualCommunicator;


public:

	/** work a little bit 
	 * the class Worker provides no implementation for that 
	*/
	virtual void work() = 0;

	/** is the worker done doing its things */
	bool isDone();

	/** get the worker number */
	uint64_t getWorkerIdentifier();


	virtual ~Worker(){}
};

#endif
