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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _ColorSpaceLoader
#define _ColorSpaceLoader

#include <string>
#include <vector>
#include <plugin_SequencesLoader/ArrayOfReads.h>
#include <fstream>
#include <memory/MyAllocator.h>
#include <stdio.h>
#include <plugin_SequencesLoader/ColorSpaceDecoder.h>
#include <plugin_SequencesLoader/Read.h>
using namespace std;

/**
 *
 * \author Sébastien Boisvert
 */
class ColorSpaceLoader{
	ColorSpaceDecoder m_decoder;
	FILE*m_f;
	int m_size;
	int m_loaded;
public:
	void load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator);
	int open(string file);
	int getSize();
};


#endif
