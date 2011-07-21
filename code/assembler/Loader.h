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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _Loader
#define _Loader

#include <core/common_functions.h>
#include <vector>
#include <memory/MyAllocator.h>
#include <structures/Read.h>
#include <fstream>
#include <structures/ArrayOfReads.h>
#include <string>

#ifdef HAVE_LIBZ
#include<format/FastqGzLoader.h>
#endif

#ifdef HAVE_LIBBZ2
#include<format/FastqBz2Loader.h>
#endif

#include<format/FastaLoader.h>
#include<format/FastqLoader.h>
#include<format/ColorSpaceLoader.h>
#include<format/SffLoader.h>

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
FORMAT_FASTQ_BZ2
};

/*
 * Loader loads data files. Data can be formated as SFF, FASTA, and FASTQ.
 * Ray makes no use of quality values, so Their encoding is irrelevant.
 */
class Loader{
	bool m_show;
	int m_type;
	int DISTRIBUTION_ALLOCATOR_CHUNK_SIZE;
	ArrayOfReads m_reads;
	MyAllocator m_allocator;

	uint64_t m_currentOffset;
	int m_maxToLoad;
	uint64_t m_size;

	SffLoader m_sff;
	ColorSpaceLoader m_color;
	FastqLoader m_fastq;	
	FastaLoader m_fasta;	

	#ifdef HAVE_LIBZ
	FastqGzLoader m_fastqgz;
	#endif

	#ifdef HAVE_LIBBZ2
	FastqBz2Loader m_fastqbz2;
	#endif

	void loadSequences();

public:
	void constructor(const char*prefix,bool show);
	int load(string file,bool isGenome);
	uint64_t size();
	Read*at(uint64_t i);
	void clear();
	void reset();
};

#endif
