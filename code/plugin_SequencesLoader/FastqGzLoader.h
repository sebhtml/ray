/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#ifndef _FastqGzLoader
#define _FastqGzLoader

#ifdef CONFIG_HAVE_LIBZ

#include "LoaderInterface.h"
#include "Read.h"
#include "ArrayOfReads.h"

#include <RayPlatform/memory/MyAllocator.h>

#include <zlib.h>
#include <string>
#include <vector>
using namespace std;


/**
 * This class is responsible for reading .fasta.gz and .fastq.gz files.
 * \author Sébastien Boisvert
 */
class FastqGzLoader: public LoaderInterface{

	bool m_completed;
	bool m_noMoreBytes;
	bool m_debug;

/* stuff for readahead */

	int m_bufferedBytes;
	int m_currentStart;
	int m_firstNewLine;
	char*m_readaheadBuffer;

	gzFile m_f;
	int m_size;
	int m_loaded;

	bool readOneSingleLine(char*buffer,int maximumLength);
	bool pullLineWithReadaheadTechnology(char*buffer,int maximumLength);

public:
	int openWithPeriod(string file,int period);
	void loadWithPeriod(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator,int period);

	int open(string file);
	int getSize();
	void load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator);
	void close();
};

#endif
#endif
