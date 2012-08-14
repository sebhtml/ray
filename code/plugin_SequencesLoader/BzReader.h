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

#ifndef _BzReader 
#define _BzReader

#ifdef HAVE_LIBBZ2

#include <bzlib.h>
#include <stdint.h>
#include <stdio.h>

/**
 * \author Sébastien Boisvert
 */
class BzReader{
	BZFILE*m_bzFile;
	FILE*m_file;
	char*m_buffer;
	int m_bufferSize;
	int m_bufferPosition;
	uint64_t m_bytesLoaded;
	
	char m_unused[BZ_MAX_UNUSED];
	int m_nUnused;
	void*m_unused1;

	void processError(int error);

public:
	void open(const char*file);
	char*readLine(char*s, int n);
	void close();
};

#endif /* HAVE_LIBBZ2 */

#endif /* _BzReader */
