/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
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

#include "LoaderInterface.h"
#include "LoaderFactory.h"
#include "Read.h"
#include "ArrayOfReads.h"

#include <code/Mock/common_functions.h>

#include <RayPlatform/memory/MyAllocator.h>

#include <string>
#include <fstream>
#include <vector>

using namespace std;

/*
 * Loader loads data files. Data can be formated as SFF, FASTA, and FASTQ.
 * Ray makes no use of quality values, so Their encoding is irrelevant.
 * \author Sébastien Boisvert
 */
class Loader{

	LoaderFactory m_factory;
	LoaderInterface*m_interface;
	Rank m_rank;

	bool m_show;
	int DISTRIBUTION_ALLOCATOR_CHUNK_SIZE;
	ArrayOfReads m_reads;
	MyAllocator m_allocator;

	LargeIndex m_currentOffset;
	int m_maxToLoad;
	LargeCount m_size;


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
