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

#include<common_functions.h>
#include<vector>
#include<MyAllocator.h>
#include<Read.h>
#include<fstream>
#include<ArrayOfReads.h>
#include<string>

#ifdef HAVE_ZLIB
#include<FastqGzLoader.h>
#endif

#ifdef HAVE_LIBBZ2
#include<FastqBz2Loader.h>
#endif

#include<FastaLoader.h>
#include<FastqLoader.h>
#include<ColorSpaceLoader.h>
#include<SffLoader.h>

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
	int m_type;
	int DISTRIBUTION_ALLOCATOR_CHUNK_SIZE;
	ArrayOfReads m_reads;
	MyAllocator m_allocator;

	int m_currentOffset;
	int m_maxToLoad;
	int m_size;

	SffLoader m_sff;
	ColorSpaceLoader m_color;
	FastaLoader m_fasta;
	FastqLoader m_fastq;	

	#ifdef HAVE_ZLIB
	FastqGzLoader m_fastqgz;
	#endif

	#ifdef HAVE_LIBBZ2
	FastqBz2Loader m_fastqbz2;
	#endif

	void loadSequences();

public:
	Loader();
	int load(string file);
	int size();
	Read*at(int i);
	void clear();
};

#endif
