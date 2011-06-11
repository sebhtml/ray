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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string>
#include <fcntl.h>
#include <map>
#include <vector>
using namespace std;

#ifndef _OnDiskAllocator
#define _OnDiskAllocator

typedef struct{
	void*m_next;
}StoreElement;

/**
 * a class that allocate memory from a file. This class uses POSIX mmap and POSIX munmap.
 * \see http://pubs.opengroup.org/onlinepubs/009695399/functions/mmap.html
 */
class OnDiskAllocator{
	vector<char*> m_pointers;
	vector<int> m_fileDescriptors;
	vector<string> m_fileNames;
	string m_prefix;

	map<int,StoreElement*> m_toReuse;

	uint64_t m_current;
	uint64_t m_chunkSize;

	void addChunk();

	void addAddressToReuse(void*p,int size);
	void*reuseAddress(int size);
	bool hasAddressesToReuse(int size);
public:
	/**
 * the constructor sets the block size
 */
	void constructor(const char*prefix);
/**
 * this function allocates <a> bytes
 */
	void*allocate(int a);
/**
 * this function free an allocation
 */
	void free(void*a,int size);
/**
 * clear everything.
 */
	void clear();
/**
 * reset the allocator
 */
	void reset();
};



#endif
