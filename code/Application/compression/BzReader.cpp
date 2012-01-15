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

#ifdef HAVE_LIBBZ2

#define __BzReader_MAXIMUM_LENGTH 2*4096

#include <compression/BzReader.h>
#include <stdlib.h>
#include <assert.h>
#include <memory/malloc_types.h>
#include <core/common_functions.h>

void BzReader::open(const char*file){
	int error;
	m_file=fopen(file,"r");
	int verbosity=0;
	int small=0;
	m_bzFile=BZ2_bzReadOpen(&error,m_file,verbosity,small,NULL,0);
	m_buffer=(char*)__Malloc(__BzReader_MAXIMUM_LENGTH*sizeof(char),RAY_MALLOC_TYPE_BZ2,false);
	m_bufferSize=0;
	m_bufferPosition=0;
}

char*BzReader::readLine(char*s, int n){
	#ifdef ASSERT
	assert(n<=__BzReader_MAXIMUM_LENGTH);
	#endif
	int pos=-1;
	for(int i=m_bufferPosition;i<m_bufferSize;i++){
		if(m_buffer[i]=='\n'){
			pos=i;
		}
	}
	if(pos!=-1){
		int i=0;
		while(m_buffer[m_bufferPosition]!='\n' && m_bufferPosition<m_bufferSize){
			if(i==n){
				s[i]='\0';
				return s;
			}
			s[i++]=m_buffer[m_bufferPosition++];
		}
		if(i==n){
			s[i]='\0';
			return s;
		}
		if(m_bufferPosition<m_bufferSize){
			s[i++]=m_buffer[m_bufferPosition++];
		}
		s[i]='\0';
		return s;
	}

	/* copy the leftover of buffer in an other buffer */
	int i=0;
	while(m_bufferPosition<m_bufferSize){
		if(i==n){
			s[i]='\0';
			return s;
		}
		s[i++]=m_buffer[m_bufferPosition++];
	}

	/* read some bytes from the compressed file */
	m_bufferPosition=0;
	int error;
	m_bufferSize=BZ2_bzRead(&error,m_bzFile,m_buffer,__BzReader_MAXIMUM_LENGTH);
	if(m_bufferSize==0){
		return NULL;
	}
	/* copy up to \n (including it) into secondaryBuffer */
	while(m_buffer[m_bufferPosition]!='\n' && m_bufferPosition<m_bufferSize){
		if(i==n){
			s[i]='\0';
			return s;
		}
		s[i++]=m_buffer[m_bufferPosition++];
	}
	if(i==n){
		s[i]='\0';
		return s;
	}
	if(m_bufferPosition<m_bufferSize){
		s[i++]=m_buffer[m_bufferPosition++];
	}
	s[i]='\0';
	return s;
}

void BzReader::close(){
	int error;
	BZ2_bzReadClose(&error,m_bzFile);
	fclose(m_file);
	m_bzFile=NULL;
	m_file=NULL;
	__Free(m_buffer,RAY_MALLOC_TYPE_BZ2,false);
}

#endif
