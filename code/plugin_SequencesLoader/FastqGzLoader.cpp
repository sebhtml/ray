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

#ifdef CONFIG_HAVE_LIBZ

#include "FastqGzLoader.h"

#define CONFIG_ZLIB_USE_READAHEAD

#define CONFIG_ZLIB_MAXIMUM_READ_LENGTH 4096
#define CONFIG_ZLIB_READAHEAD_SIZE 1048576 /* 1 MiB */

#include <fstream>
#include <zlib.h>
#include <stdlib.h>
using namespace std;

int FastqGzLoader::open(string file){
	return openWithPeriod(file,4);
}

int FastqGzLoader::openWithPeriod(string file,int period){

	m_debug=false;

	m_completed=false;

#ifdef CONFIG_ZLIB_USE_READAHEAD
	m_bufferedBytes=0;
	m_readaheadBuffer=NULL;
	m_noMoreBytes=false;
#endif

	m_f=gzopen(file.c_str(),"r");
	char buffer[CONFIG_ZLIB_MAXIMUM_READ_LENGTH];
	m_size=0;
	m_loaded=0;

	int rotatingVariable=0;
	while(readOneSingleLine(buffer,CONFIG_ZLIB_MAXIMUM_READ_LENGTH)){
		if(rotatingVariable==1){
			m_size++;
		}
		rotatingVariable++;
		if(rotatingVariable==period){
			rotatingVariable=0;
		}
	}
	gzclose(m_f);
	m_f=gzopen(file.c_str(),"r");

#ifdef CONFIG_ZLIB_USE_READAHEAD
	m_bufferedBytes=0;
	m_readaheadBuffer=NULL;
	m_noMoreBytes=false;
	m_completed=false;
#endif

	return EXIT_SUCCESS;
}

void FastqGzLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	loadWithPeriod(maxToLoad,reads,seqMyAllocator,4);
}

// a very simple and compact fastq.gz reader
void FastqGzLoader::loadWithPeriod(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator,int period){
	char buffer[CONFIG_ZLIB_MAXIMUM_READ_LENGTH];
	int rotatingVariable=0;
	int loadedSequences=0;

	while(loadedSequences<maxToLoad && readOneSingleLine(buffer,CONFIG_ZLIB_MAXIMUM_READ_LENGTH)){
		if(rotatingVariable==1){
			Read t;
			t.constructor(buffer,seqMyAllocator,true);
			reads->push_back(&t);
		}
		rotatingVariable++;

		// a period is reached for each read.
		if(rotatingVariable==period){
			rotatingVariable=0;
			loadedSequences++;
			m_loaded++;
		}
	}
	if(m_loaded==m_size){
		gzclose(m_f);
	}
}

int FastqGzLoader::getSize(){
	return m_size;
}

/**
 * The initial implementation was using gzgets.
 * But tests on the IBM Blue Gene/Q at SciNet in Toronto 
 * indicated that gzgets is not a good idea in the end.
 *
 * The new implementation does its own read-ahead code.
 */
bool FastqGzLoader::readOneSingleLine(char*buffer,int maximumLength){

#ifdef CONFIG_ZLIB_USE_READAHEAD

	return pullLineWithReadaheadTechnology(buffer,maximumLength);

#else
	char*returnedValue=gzgets(m_f,buffer,maximumLength);
	
	return returnedValue!=Z_NULL;
#endif

}


bool FastqGzLoader::pullLineWithReadaheadTechnology(char*buffer,int maximumLength){

	if(m_readaheadBuffer==NULL && !m_completed){

		if(m_debug)
			cout<<"Starting system."<<endl;

		m_readaheadBuffer=(char*)malloc(2*CONFIG_ZLIB_READAHEAD_SIZE);
		m_bufferedBytes=0;
		m_currentStart=0;
		m_firstNewLine=0;

		#ifdef ASSERT
		assert(m_readaheadBuffer!=NULL);
		#endif
	}

	#ifdef ASSERT
	assert(m_readaheadBuffer!=NULL);
	#endif

	int availableBytes=2*CONFIG_ZLIB_READAHEAD_SIZE-m_bufferedBytes+m_currentStart;

/* check for available bytes */
	if(!m_noMoreBytes && availableBytes>=CONFIG_ZLIB_READAHEAD_SIZE){
	
/* make sure there is space available */

		if(m_debug)
			cout<<"Moving data around"<<endl;

		int destination=0;

/* move data on the left to make room */
		for(int i=m_currentStart;i<m_bufferedBytes;i++){
			m_readaheadBuffer[destination++]=m_readaheadBuffer[i];
		}

		m_bufferedBytes-=m_currentStart;
		m_firstNewLine-=m_currentStart;
		m_currentStart-=m_currentStart;

		if(m_debug)
			cout<<"Reading data from disk"<<endl;

		#ifdef ASSERT
		assert(m_bufferedBytes<=2*CONFIG_ZLIB_READAHEAD_SIZE);
	
		if(m_bufferedBytes>CONFIG_ZLIB_READAHEAD_SIZE){
			cout<<"Error: there is too much data: m_bufferedBytes: "<<m_bufferedBytes<<" CONFIG_ZLIB_READAHEAD_SIZE:";
			cout<<CONFIG_ZLIB_READAHEAD_SIZE<<endl;
		}

		assert(m_bufferedBytes<=CONFIG_ZLIB_READAHEAD_SIZE);

		assert(m_f!=NULL);
		assert(m_readaheadBuffer!=NULL);

		#endif

		int bytes=gzread(m_f,m_readaheadBuffer+m_bufferedBytes,CONFIG_ZLIB_READAHEAD_SIZE);
		m_bufferedBytes+=bytes;

		if(bytes==0)
			m_noMoreBytes=true;

		#ifdef ASSERT
		assert(m_bufferedBytes<=2*CONFIG_ZLIB_READAHEAD_SIZE);
		#endif
	}


	if(m_debug)
		cout<<"Searching for new line at m_firstNewLine "<<m_firstNewLine<<endl;
	
/* find the next new line */
	while(m_firstNewLine<m_bufferedBytes && m_readaheadBuffer[m_firstNewLine]!='\n')
		m_firstNewLine++;

	if(m_firstNewLine==m_bufferedBytes)
		m_firstNewLine--;

	if(m_debug)
		cout<<"seek offset if at m_firstNewLine "<<m_firstNewLine<<endl;

	#ifdef ASSERT
	assert(m_firstNewLine<m_bufferedBytes);
	assert(m_readaheadBuffer[m_firstNewLine]=='\n' || m_firstNewLine==m_bufferedBytes-1);
	#endif

	#ifdef ASSERT
	assert(m_bufferedBytes<=2*CONFIG_ZLIB_READAHEAD_SIZE);
	#endif

	if(m_debug){
		cout<<"Copying data m_currentStart="<<m_currentStart<<" m_firstNewLine="<<m_firstNewLine;
		cout<<" m_bufferedBytes: "<<m_bufferedBytes<<endl;
	}

/* copy the line */
	int bytesToCopy=m_firstNewLine-m_currentStart+1;

/* nothing more to read */
	if(m_bufferedBytes==0 || bytesToCopy==0){

		if(m_debug){
			cout<<" m_bufferedBytes: "<<m_bufferedBytes<<" m_firstNewLine= ";
			cout<<m_firstNewLine<<" m_currentStart: "<<m_currentStart;
			cout<<endl;
		}

		free(m_readaheadBuffer);
		m_readaheadBuffer=NULL;
		m_bufferedBytes=0;
		m_completed=true;

		return false;
	}


	char*source=m_readaheadBuffer+m_currentStart;

	#ifdef ASSERT
	if(bytesToCopy==0){
		cout<<"Error: nothing to copy, m_bufferedBytes= "<<m_bufferedBytes;
		cout<<" m_firstNewLine "<<m_firstNewLine<<" m_currentStart "<<m_currentStart;
		cout<<endl;
	}

	assert(bytesToCopy>0);
	assert(m_currentStart<m_bufferedBytes);

	int last=m_currentStart+bytesToCopy-1;

	if(last>=m_bufferedBytes){
		cout<<"Error: start="<<m_currentStart<<" bytes="<<bytesToCopy<<" last= "<<last<<" buffered: "<<m_bufferedBytes<<endl;
	}

	assert(m_currentStart+bytesToCopy-1<m_bufferedBytes);
	#endif /* ASSERT */

	memcpy(buffer,source,bytesToCopy);
	buffer[bytesToCopy]='\0';

	if(m_debug)
		cout<<"Line @"<<m_currentStart<<"= "<< buffer<<endl;

/* advance the current start and find the new new line */
	if(m_debug)
		cout<<"Start + "<<bytesToCopy<<endl;

	m_currentStart+=bytesToCopy;
	m_firstNewLine++; // the next call will find the new line position

	if(m_debug)
		cout<<"Finding the next new line in "<<m_bufferedBytes<<" bytes starting at "<<m_firstNewLine<<""<<endl;

	return true;
}

void FastqGzLoader::close(){
}

#endif

