/*
 	Ray
    Copyright (C)  2010, 2011  Sébastien Boisvert

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

#ifndef _ReusableMemoryStore
#define _ReusableMemoryStore

#include <stdint.h>
#include <map>
using namespace std;

typedef struct{
	void*m_next;
}Element;

/** 
 * the ReusableMemoryStore don't allocate much memory
 * it bins freed chunks by size  and these are linked in list.
 * The m_next pointer is actually in the freed chunk.
 * \author Sébastien Boisvert
 */
class ReusableMemoryStore{
	map<int,Element*> m_toReuse;
public:

	bool hasAddressesToReuse(int size);
	void*reuseAddress(int size);
	void addAddressToReuse(void*ptr,int size);
	void constructor();
	void reset();
	void print();
};

#endif
