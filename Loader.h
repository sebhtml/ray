/*
 	Ray
    Copyright (C) 2009, 2010  Sébastien Boisvert

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


#ifndef _Loader
#define _Loader

#include<vector>
#include<MyAllocator.h>
#include<Read.h>
#include<fstream>
#include<string>
using namespace std;

class Loader{
	int m_total;
	int m_bases;
	void add(vector<Read*>*reads,string*id,ostringstream*sequence,ostringstream*quality,MyAllocator*seqMyAllocator,MyAllocator*readMyAllocator);
public:
	Loader();
	void load(string file,vector<Read*>*reads,MyAllocator*seqMyAllocator,MyAllocator*readMyAllocator);
	int getBases();
	int getReads();
};

#endif
