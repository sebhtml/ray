/*
 	Ray
    Copyright (C) 2012  Sébastien Boisvert

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

#include <scripting/ScriptEngine.h>
#include <vector>
#include <scheduling/SwitchMan.h>
#include <core/slave_modes.h>
#include <core/master_modes.h>
using namespace std;

void ScriptEngine::configureSwitchMan(SwitchMan*switchMan){
	// configure the switch man
	//
	// this is where steps can be added or removed.

	vector<MasterMode> steps;

	#define ITEM(x) \
	steps.push_back(x);

	#include <master_mode_order.txt>

	#undef ITEM

	for(int i=0;i<(int)steps.size()-1;i++){
		switchMan->addNextMasterMode(steps[i],steps[i+1]);
	}


	#define ITEM(mpiTag,slaveMode) \
	switchMan->addSlaveSwitch(mpiTag,slaveMode);
	
	#include <slave_switches.txt>

	#undef ITEM

	#define ITEM(masterMode,mpiTag) \
	switchMan->addMasterSwitch(masterMode,mpiTag);
	
	#include <master_switches.txt>

	#undef ITEM
}


