/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/


#ifndef _Loader
#define _Loader

#include<common_functions.h>
#include<vector>
#include<MyAllocator.h>
#include<Read.h>
#include<fstream>
#include<string>
using namespace std;

/*
 * Loader loads data files. Data can be formated as SFF, FASTA, and FASTQ.
 * Ray makes no use of quality values, so Their encoding is irrelevant.
 */
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
