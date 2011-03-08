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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <map>
#include <vector>
#include <sys/mman.h>
using namespace std;

#ifndef _OnDiskAllocator
#define _OnDiskAllocator

typedef struct{
	void*m_next;
}StoreElement;

/**
 * a class that allocate memory from a file
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
};



#endif
