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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _SffLoader
#define _SffLoader

#include<string>
#include<vector>
#include<plugin_SequencesLoader/Read.h>
#include<stdio.h>
#include<fstream>
#include<plugin_SequencesLoader/ArrayOfReads.h>
#include<memory/MyAllocator.h>
using namespace std;

/**
 * This class allows one to use SFF file directly.
 * see http://454.com
 * \author Sébastien Boisvert
 */
class SffLoader{
	int m_size;
	int m_loaded;
	FILE*m_fp;
	uint16_t m_number_of_flows_per_read;
	char*key_sequence;
	uint16_t key_length;
	char*flow_chars;

	int openSff(string file);

public:
	int open(string file);
	int getSize();
	void load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator);
};

#endif
