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


#ifndef _SffLoader
#define _SffLoader

#include<string>
#include<vector>
#include<Read.h>
#include<fstream>
#include<MyAllocator.h>
using namespace std;

class SffLoader{
	int m_bases;
public:
	SffLoader();
	int getBases();
	void load(string file,vector<Read*>*reads,MyAllocator*seqMyAllocator,MyAllocator*readMyAllocator);
};

#endif
