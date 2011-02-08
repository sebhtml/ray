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

#ifndef _GridAllocator
#define _GridAllocator

#include <stdint.h>
#include <Vertex.h>
#include <MyAllocator.h>

#include <map>
using namespace std;

class GridAllocator{
	map<int,Vertex*> m_toReuse;
	MyAllocator m_allocator;
public:
	void constructor();
	void free(Vertex*a,int b);
	Vertex*allocate(int a,uint16_t*reserved);
	MyAllocator*getAllocator();
};

#endif
