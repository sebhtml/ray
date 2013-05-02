/*
 *  Ray -- Parallel genome assemblies for parallel DNA sequencing
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

#ifndef _LoaderInterface_h
#define _LoaderInterface_h

#include "Read.h"
#include "ArrayOfReads.h"

#include <string>
using namespace std;

/**
 * This is a interface for implementing new file formats.
 *
 * \author Sébastien Boisvert
 */
class LoaderInterface {
private:
	vector<string> m_extensions;
	bool hasSuffix(const char* fileName,const char* suffix);

public:
	virtual int open(string file) = 0;
	virtual int getSize() = 0;
	virtual void load(int maxToLoad,ArrayOfReads*reads,
		MyAllocator*seqMyAllocator) = 0;
	virtual void close() = 0;
	bool checkFileType(const char* fileName);
	void addExtension(const char* fileName);
};

#endif
