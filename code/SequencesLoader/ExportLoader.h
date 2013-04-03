/*
    Ray -- Parallel genome assemblies for parallel DNA sequencing
    Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert

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

#ifndef _ExportLoader
#define _ExportLoader

#include "LoaderInterface.h"
#include "ArrayOfReads.h"
#include "Read.h"

#include <RayPlatform/memory/MyAllocator.h>

#include <fstream>
#include <vector>
#include <sstream>
#include <stdio.h>
#include <string>
using namespace std;

/**
 *
 * A loader for export files.
 *
 * This gene was created by a gene duplication of FastqLoader.h.
 *
 * \author Sébastien Boisvert
 */
class ExportLoader: public LoaderInterface{

	int m_loaded;
	int m_size;
	FILE*m_f;
public:
	int open(string file);
	int getSize();
	void load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator);
	void close();
};

#endif

