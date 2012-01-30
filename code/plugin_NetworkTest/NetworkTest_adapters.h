/*
 	Ray
    Copyright (C) 2011, 2012  Sébastien Boisvert

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

#ifndef _NetworkTest_adapters_h
#define _NetworkTest_adapters_h

#include <handlers/MasterModeHandler.h>
#include <handlers/SlaveModeHandler.h>

class NetworkTest;

class Adapter_RAY_MASTER_MODE_TEST_NETWORK: public MasterModeHandler{

	NetworkTest*m_object;

public:
	void setObject(NetworkTest*object);
	void call();
};

class Adapter_RAY_SLAVE_MODE_TEST_NETWORK: public SlaveModeHandler{

	NetworkTest*m_object;

public:
	void setObject(NetworkTest*object);
	void call();
};

#endif

