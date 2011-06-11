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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _BzReader
#define _BzReader

#include<bzlib.h>
#include<stdio.h>

class BzReader{
	BZFILE*m_bzFile;
	FILE*m_file;
	char*m_buffer;
	int m_bufferSize;
	int m_bufferPosition;
	
public:
	void open(const char*file);
	char*readLine(char*s, int n);
	void close();
};

#endif
