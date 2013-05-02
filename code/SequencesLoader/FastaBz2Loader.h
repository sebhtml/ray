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

#ifndef _FastaBz2Loader
#define _FastaBz2Loader

#ifdef CONFIG_HAVE_LIBBZ2

#include "FastqBz2Loader.h"
#include "LoaderInterface.h"
#include "ArrayOfReads.h"
#include "Read.h"

#include <RayPlatform/memory/MyAllocator.h>

#include <string>
#include <vector>
using namespace std;

/**
 * \author Sébastien Boisvert
 */
class FastaBz2Loader: public LoaderInterface{
	FastqBz2Loader m_fastqBz2Loader;
public:
	FastaBz2Loader();
	int open(string file);
	int getSize();
	void load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator);
	void close();
};

#endif
#endif

