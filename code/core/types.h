/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

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

#ifndef _types_h
#define _types_h

typedef int Rank;

/* although RayMPITag is defined, routing tag are not valid RayMPITag instances. */
typedef int Tag;

typedef int RoutingTag;

typedef int Distance;


class MessageProcessor; /* needed to define MessageProcessorHandler */
class Message;
class MessageTagHandler;

/* define method pointers with 1 argument of type Message* */
typedef void (MessageProcessor::*MessageProcessorHandler) (Message*message);

/* new interface */
typedef void (MessageTagHandler::*MessageTagHandlerMethod) (Message*message);


/**
* Main class of the application. Runs the main program loop on each MPI rank.
*/

class Machine;

typedef void (Machine::*MachineSlaveHandler) ();

typedef void (Machine::*MachineMasterHandler) ();



#endif
