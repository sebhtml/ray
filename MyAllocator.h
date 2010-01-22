/*
 	OpenAssembler -- a de Bruijn DNA assembler for mixed high-throughput technologies
    Copyright (C) 2009  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/


#ifndef _MyAllocator
#define _MyAllocator

#include<vector>
using namespace std;

class MyAllocator{
	vector<void*>m_chunks;
	void*m_currentChunk;
	int m_currentPosition;
	unsigned int m_jobs;
	int m_CHUNK_SIZE;
public:
	MyAllocator();
	void print();
	void clear();
	void constructor();
	void*allocate(int s);
	~MyAllocator();
	int getChunkSize();
	int getNumberOfChunks();
};


#endif
