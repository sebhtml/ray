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

#ifndef _SwitchMan_H
#define _SwitchMan_H

#include <map>
#include <core/master_modes.h>
using namespace std;

class SwitchMan{
	int m_target;
	int m_counter;

	map<int,int> m_switches;

	void runAssertions();

public:
	void constructor(int numberOfRanks);
	void reset();
	bool allRanksAreReady();
	void addReadyRank();

	int getNextMasterMode(int currentMode);

	void addNextMasterMode(int a,int b);

};

#endif
