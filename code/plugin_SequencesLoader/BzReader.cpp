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

#ifdef HAVE_LIBBZ2

#define __BzReader_MAXIMUM_LENGTH 2*4096

#include <plugin_SequencesLoader/BzReader.h>
#include <stdlib.h>
#include <assert.h>
#include <application_core/common_functions.h>

void BzReader::open(const char*file){
	m_file=fopen(file,"r");
	m_bzFile=NULL;
	m_buffer=(char*)__Malloc(__BzReader_MAXIMUM_LENGTH*sizeof(char),"RAY_MALLOC_TYPE_BZ2",false);
	m_bufferSize=0;
	m_bufferPosition=0;

	m_nUnused=0;
	m_bytesLoaded=0;
}

char*BzReader::readLine(char*s, int n){

	int error=BZ_OK;
	int verbosity=0;
	int small=0;

	if(m_bzFile==NULL && !feof(m_file)){

		#ifdef __bz2_verbose__
		cout<<"Opening bz2 file"<<endl;
		#endif

		m_bzFile=BZ2_bzReadOpen(&error,m_file,verbosity,small,
			m_unused,m_nUnused);

		if(error!=BZ_OK)
			cout<<"Error: BZ2_bzReadOpen failed."<<endl;
	}

	//cout<<"[BzReader::readLine]"<<endl;

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

	m_bufferSize=0;

	if(m_bzFile!=NULL)
		m_bufferSize=BZ2_bzRead(&error,m_bzFile,m_buffer,__BzReader_MAXIMUM_LENGTH);

	if(error==BZ_STREAM_END){
		#ifdef __bz2_verbose__
		// no more thing to read.
		cout<<"Notice: BZ2_bzRead returned BZ_STREAM_END"<<endl;
		cout<<"Total bytes: "<<m_bytesLoaded<<endl;
		#endif

		// get unused bytes for the next round
		BZ2_bzReadGetUnused ( &error, m_bzFile, &m_unused1, &m_nUnused );

		if(error!=BZ_OK)
			cout<<"Error with BZ2_bzReadGetUnused"<<endl;

		// copy unused bytes
		memcpy(m_unused,m_unused1,m_nUnused);

		#ifdef __bz2_verbose__
		cout<<"Closing file for now."<<endl;
		#endif

   		BZ2_bzReadClose ( &error, m_bzFile );

		m_bzFile=NULL;

		#ifdef __bz2_verbose__
		if(feof(m_file))
			cout<<"Reached end of file, bufferSize is "<<m_bufferSize<<endl;
		#endif

		// nothing more is available.
		if(m_bufferSize==0 && feof(m_file)){
			return NULL;
		}

	}else if(error!=BZ_OK){
		cout<<"Error: BZ2_bzRead did not return BZ_OK or BZ_STREAM_END."<<endl;

		cout<<"bzFile= "<<m_bzFile<<endl;
		processError(error);
		return NULL;
	}

	m_bytesLoaded+=m_bufferSize;

	/* copy up to \n (including it) into secondaryBuffer */
	while(i<n && m_buffer[m_bufferPosition]!='\n' && m_bufferPosition<m_bufferSize){
		if(i==n){
			s[i]='\0';
			return s;
		}
		s[i++]=m_buffer[m_bufferPosition++];
	}

	// don't return more than n bytes
	if(i==n){
		s[i]='\0';
		return s;
	}

	if( i < n && m_bufferPosition<m_bufferSize){
		s[i++]=m_buffer[m_bufferPosition++];
	}

	s[i]='\0';

	// Nothing loaded.
	if(i==0)
		return NULL;


	return s;
}

void BzReader::close(){
	fclose(m_file);
	m_bzFile=NULL;
	m_file=NULL;
	__Free(m_buffer,"RAY_MALLOC_TYPE_BZ2",false);
}

void BzReader::processError(int error){
	switch (error){
		case BZ_PARAM_ERROR:
			cout<<"BZ_PARAM_ERROR"<<endl;
  			cout<<"if b is NULL or buf is NULL or len < 0"<<endl;
			break;
		case BZ_SEQUENCE_ERROR:
			cout<<"BZ_SEQUENCE_ERROR"<<endl;
  			cout<<"if b was opened with BZ2_bzWriteOpen"<<endl;
			 break;
		case BZ_IO_ERROR:
			cout<<"BZ_IO_ERROR"<<endl;
  			cout<<"if there is an error reading from the compressed file"<<endl;
			break;
		case BZ_UNEXPECTED_EOF:
			cout<<"BZ_UNEXPECTED_EOF"<<endl;
			cout<<"  if the compressed file ended before the logical end-of-stream was detected";
			cout<<endl;
			break;
		case BZ_DATA_ERROR:
			cout<<"BZ_DATA_ERROR"<<endl;
  			cout<<"if a data integrity error was detected in the compressed stream"<<endl;
			break;
		case BZ_DATA_ERROR_MAGIC:
			cout<<"BZ_DATA_ERROR_MAGIC"<<endl;
  			cout<<"if the stream does not begin with the requisite header bytes "<<endl;
  			cout<<"(ie, is not a bzip2 data file).  This is really "<<endl;
  			cout<<"a special case of BZ_DATA_ERROR."<<endl;
			break;
		case BZ_MEM_ERROR:
			cout<<"BZ_MEM_ERROR"<<endl;
  			cout<<"if insufficient memory was available"<<endl;
			break;
		case BZ_STREAM_END:
			cout<<"BZ_STREAM_END"<<endl;
  			cout<<"if the logical end of stream was detected."<<endl;
			break;
		case BZ_OK:
			cout<<"BZ_OK"<<endl;
  			cout<<"otherwise."<<endl;
			break;
	}
}

#endif
