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

#ifdef HAVE_LIBZ

#include<plugin_SequencesLoader/FastqGzLoader.h>
#include<fstream>
#include<zlib.h>
#include <stdlib.h>

int FastqGzLoader::open(string file,int period){
	m_f=gzopen(file.c_str(),"r");
	char buffer[4096];
	m_size=0;
	m_loaded=0;

	int rotatingVariable=0;
	while(Z_NULL!=gzgets(m_f,buffer,4096)){
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
	return EXIT_SUCCESS;
}

// a very simple and compact fastq.gz reader
void FastqGzLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator,int period){
	char buffer[4096];
	int rotatingVariable=0;
	int loadedSequences=0;

	while(loadedSequences<maxToLoad && Z_NULL!=gzgets(m_f,buffer,4096)){
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

#endif

