/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

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
	map<int,double> m_chiSquaredTableAt0Point05;
	Parameters*m_parameters;
	VERTEX_TYPE m_choice;


public:
	bool isGenuineBubble(VERTEX_TYPE root, vector<vector<VERTEX_TYPE> >*trees,
map<VERTEX_TYPE,int>*coverages);
	void constructor(Parameters*p);

	VERTEX_TYPE getTraversalStartingPoint();

	void printStuff(VERTEX_TYPE root, vector<vector<VERTEX_TYPE> >*trees,
map<VERTEX_TYPE,int>*coverages);
};

#endif

