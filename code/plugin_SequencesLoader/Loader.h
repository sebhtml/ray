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

#ifndef _Loader
#define _Loader

#include <application_core/common_functions.h>
#include <vector>
#include <memory/MyAllocator.h>
#include <plugin_SequencesLoader/Read.h>
#include <fstream>
#include <plugin_SequencesLoader/ArrayOfReads.h>
#include <string>

#ifdef HAVE_LIBZ
#include<plugin_SequencesLoader/FastqGzLoader.h>
#endif

#ifdef HAVE_LIBBZ2
#include<plugin_SequencesLoader/FastqBz2Loader.h>
#endif

#include<plugin_SequencesLoader/FastaLoader.h>
#include<plugin_SequencesLoader/FastqLoader.h>
#include<plugin_SequencesLoader/ColorSpaceLoader.h>
#include<plugin_SequencesLoader/SffLoader.h>

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
 * \author Sébastien Boisvert
 */
class Loader{
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
	LargeCount size();
	Read*at(LargeIndex i);
	void clear();
	void reset();
};

#endif
