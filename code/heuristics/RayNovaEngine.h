/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#ifndef _RayNovaEngine_H
#define _RayNovaEngine_H

#include <vector>
#include<map>
using namespace std;

/** This the new Ray NovaEngine(*) 
 *NovaEngine decides where to go in the k-mer graph
 */
class RayNovaEngine{
public:
	/** choose where to go */
	int choose(vector<map<int,int> >*distances);
};

#endif

