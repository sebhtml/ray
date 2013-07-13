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

#include "FastqLoader.h"

#include <code/Mock/constants.h>

#include <fstream>
#include <stdlib.h>
using namespace std;

FastqLoader::FastqLoader() {
	addExtension(".fastq");
	addExtension(".fq");

	m_f = NULL;
}

int FastqLoader::open(string file){
	return openWithPeriod(file,4);
}

int FastqLoader::openWithPeriod(string file,int period){
	m_f=fopen(file.c_str(),"r");

	//cout << "[DEBUG] counting entries" << endl;

	m_lineReader.initialize();

	m_size=0;
	m_loaded=0;
	int rotatingVariable=0;
	char buffer[RAY_MAXIMUM_READ_LENGTH];

	while(NULL!= m_lineReader.readLine(buffer,RAY_MAXIMUM_READ_LENGTH,m_f)){

		/*
		if(m_size > 7000000)
			//cout << "[DEBUG] buffer= " << buffer << endl;
			*/

		if(rotatingVariable==1){
			m_size++;
		}
		rotatingVariable++;
		if(rotatingVariable==period){
			rotatingVariable=0;
		}

		/*
		if(m_size % 1000 == 0)
			//cout << "[DEBUG] m_size= " << m_size << endl;
			*/
	}

	//cout << "[DEBUG] m_size= " << m_size << endl;

	fclose(m_f);
	m_f=fopen(file.c_str(),"r");

	m_lineReader.reset();

	return EXIT_SUCCESS;
}

void FastqLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){

	//cout << "[DEBUG] loading fastq file maxToLoad= " << maxToLoad << endl;

	loadWithPeriod(maxToLoad,reads,seqMyAllocator,4);
}

void FastqLoader::loadWithPeriod(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator,int period){
	char buffer[RAY_MAXIMUM_READ_LENGTH];
	int rotatingVariable=0;
	int loadedSequences=0;

	//cout << "[DEBUG] loadWithPeriod m_loaded= " << m_loaded << " m_size= " << m_size << endl;

	while(loadedSequences<maxToLoad && NULL != m_lineReader.readLine(buffer,RAY_MAXIMUM_READ_LENGTH,m_f)){

		// pick up the sequence data
		if(rotatingVariable == 1){
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
		close();
	}
}

int FastqLoader::getSize(){
	return m_size;
}

void FastqLoader::close(){
	if(m_f != NULL) {
		fclose(m_f);
		m_f = NULL;
	}

	m_lineReader.destroy();
}


