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

#ifndef _BubbleTool
#define _BubbleTool

#include<vector>
#include<common_functions.h>
#include<map>
#include<Parameters.h>
using namespace std;

class BubbleTool{
	Parameters*m_parameters;
	uint64_t m_choice;
public:
	bool isGenuineBubble(uint64_t root, vector<vector<uint64_t> >*trees,
map<uint64_t,int>*coverages);
	void constructor(Parameters*p);

	uint64_t getTraversalStartingPoint();

	void printStuff(uint64_t root, vector<vector<uint64_t> >*trees,
map<uint64_t,int>*coverages);
};

#endif

