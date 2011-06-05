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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/




#ifndef _BzReader
#define _BzReader

#ifdef HAVE_LIBBZ2
#include<bzlib.h>
#endif
#include<stdio.h>

class BzReader{
	#ifdef HAVE_LIBBZ2
	BZFILE*m_bzFile;
	#endif
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
