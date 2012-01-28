/*
 	Ray
    Copyright (C) 2010, 2011, 2012  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _types_h
#define _types_h

#include <stdint.h>

typedef int Rank;

typedef int MessageTag;

typedef int SlaveMode;

typedef int MasterMode;

typedef int RoutingTag;

typedef int Distance;

#define MAXIMUM_NUMBER_OF_MASTER_HANDLERS 64
#define MAXIMUM_NUMBER_OF_SLAVE_HANDLERS  64
#define MAXIMUM_NUMBER_OF_TAG_HANDLERS 256

typedef uint64_t PluginHandle;

#define INVALID_HANDLE -9876

#endif
