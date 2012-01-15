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

#ifndef _ScaffoldingVertex_h
#define _ScaffoldingVertex_h

#include <stdint.h>
#include <fstream>
using namespace std;

class ScaffoldingVertex{
	uint64_t m_name;
	int m_length;
public:
	ScaffoldingVertex(uint64_t name,int length);
	ScaffoldingVertex();
	uint64_t getName();
	int getLength();
	void read(ifstream*f);
};

#endif

