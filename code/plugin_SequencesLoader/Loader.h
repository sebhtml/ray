/*
 	Ray
    Copyright (C)  2010, 2011, 2012, 2013 Sébastien Boisvert

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

#include "Read.h"
#include "ArrayOfReads.h"
#include "FastaLoader.h"
#include "ExportLoader.h"
#include "FastqLoader.h"
#include "ColorSpaceLoader.h"
#include "SffLoader.h"

#ifdef CONFIG_HAVE_LIBZ
#include "FastqGzLoader.h"
#endif

#ifdef CONFIG_HAVE_LIBBZ2
#include "FastqBz2Loader.h"
#endif

#include <code/plugin_Mock/common_functions.h>

#include <RayPlatform/memory/MyAllocator.h>

#include <string>
#include <fstream>
#include <vector>

using namespace std;

enum{
FORMAT_NULL,
FORMAT_CSFASTA,
FORMAT_SFF,
FORMAT_FASTA,
FORMAT_FASTQ,
FORMAT_FASTA_GZ,
FORMAT_FASTQ_GZ,
FORMAT_FASTA_BZ2,
FORMAT_FASTQ_BZ2,
FORMAT_EXPORT
};

/*
 * Loader loads data files. Data can be formated as SFF, FASTA, and FASTQ.
 * Ray makes no use of quality values, so Their encoding is irrelevant.
 * \author Sébastien Boisvert
 */
class Loader{
	
	Rank m_rank;

	bool m_show;
	int m_type;
	int DISTRIBUTION_ALLOCATOR_CHUNK_SIZE;
	ArrayOfReads m_reads;
	MyAllocator m_allocator;

	LargeIndex m_currentOffset;
	int m_maxToLoad;
	LargeCount m_size;

	SffLoader m_sff;
	ColorSpaceLoader m_color;
	FastqLoader m_fastq;	
	FastaLoader m_fasta;	
	ExportLoader m_export;

	#ifdef CONFIG_HAVE_LIBZ
	FastqGzLoader m_fastqgz;
	#endif

	#ifdef CONFIG_HAVE_LIBBZ2
	FastqBz2Loader m_fastqbz2;
	#endif

	void loadSequences();

public:
	void constructor(const char*prefix,bool show,Rank rank);
	int load(string file,bool isGenome);
	LargeCount size();
	Read*at(LargeIndex i);
	void clear();
	void reset();
};

#endif
