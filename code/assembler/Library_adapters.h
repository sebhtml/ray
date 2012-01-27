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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _Library_adapters_h
#define _Library_adapters_h

#include <handlers/SlaveModeHandler.h>
#include <handlers/MasterModeHandler.h>

class Library;

class Adapter_RAY_SLAVE_MODE_AUTOMATIC_DISTANCE_DETECTION: public SlaveModeHandler{
	Library*m_object;

public:
	void setObject(Library*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_SEND_LIBRARY_DISTANCES: public SlaveModeHandler{
	Library*m_object;

public:
	void setObject(Library*object);
	void call();
};

class Adapter_RAY_MASTER_MODE_UPDATE_DISTANCES: public MasterModeHandler{
	Library*m_object;

public:
	void setObject(Library*object);
	void call();
};


#endif

