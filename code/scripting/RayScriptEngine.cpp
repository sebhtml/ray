/*
 	Ray
    Copyright (C) 2012  Sébastien Boisvert

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

#include <scripting/RayScriptEngine.h>
#include <vector>
#include <scheduling/SwitchMan.h>
#include <core/slave_modes.h>
#include <core/master_modes.h>
#include <core/constants.h>
#include <structures/Kmer.h>
using namespace std;

void RayScriptEngine::configureSwitchMan(SwitchMan*switchMan){
	// configure the switch man
	//
	// this is where steps can be added or removed.

	vector<RayMasterMode> steps;

	#define ITEM(x) \
	steps.push_back(x);

	#include <scripting/master_mode_order.txt>

	#undef ITEM

	for(int i=0;i<(int)steps.size()-1;i++){
		switchMan->addNextMasterMode(steps[i],steps[i+1]);
	}


	#define ITEM(mpiTag,slaveMode) \
	switchMan->addSlaveSwitch(mpiTag,slaveMode);
	
	#include <scripting/slave_switches.txt>

	#undef ITEM

	#define ITEM(masterMode,mpiTag) \
	switchMan->addMasterSwitch(masterMode,mpiTag);
	
	#include <scripting/master_switches.txt>

	#undef ITEM
}

void RayScriptEngine::configureVirtualCommunicator(VirtualCommunicator*virtualCommunicator){
	/** configure the virtual communicator. */
	/* ## concatenates 2 symbols */

	#define ITEM(x,y) \
	virtualCommunicator->setElementsPerQuery( x, y );

	/* define the number of words for particular message tags */

	#include <scripting/tag_sizes.txt>

	#undef ITEM

	/* set reply-map for other tags too */

	#define ITEM(x,y) \
	virtualCommunicator->setReplyType(x,y);

	#include <scripting/reply_tags.txt>

	#undef ITEM

}

void RayScriptEngine::configureMasterHandlers(){
}
