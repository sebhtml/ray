/*
 	Ray
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

#include "ExportLoader.h"

#include <code/Mock/constants.h>

#include <fstream>
#include <stdlib.h>
using namespace std;

ExportLoader::ExportLoader() {
	addExtension("export.txt");
	addExtension("qseq.txt");
}

int ExportLoader::open(string file){
	m_f=fopen(file.c_str(),"r");
	m_size=0;
	m_loaded=0;
	char buffer[RAY_MAXIMUM_READ_LENGTH];

	while(NULL!=fgets(buffer,RAY_MAXIMUM_READ_LENGTH,m_f)){
		m_size++;
	}

	fclose(m_f);
	m_f=fopen(file.c_str(),"r");
	return EXIT_SUCCESS;
}

void ExportLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	char buffer[RAY_MAXIMUM_READ_LENGTH];
	int loadedSequences=0;

	while(loadedSequences<maxToLoad && NULL!=fgets(buffer,RAY_MAXIMUM_READ_LENGTH,m_f)){

// find the 9th column
// index 8 if 0-based
// 1 -> 1 \t
// 2 -> 2 \t
// 8 -> 8 \t
// 
// find the 8th \t 
// then find the 9th \t

		int theLength=strlen(buffer);
		int positionFor8thTabulation=0;
		int positionFor9thTabulation=0;
		int tabulations=0;
		int position=0;

		while(position<theLength){
			if(buffer[position]=='\t')
				tabulations++;

			if(tabulations==8 && positionFor8thTabulation==0)
				positionFor8thTabulation=position;
			if(tabulations==9 && positionFor9thTabulation==0)
				positionFor9thTabulation=position;

			position++;
		}

		buffer[positionFor9thTabulation]='\0';
		char*dnaSequence=buffer+positionFor8thTabulation+1;

#ifdef DEBUG_EXPORT_FORMAT
		cout<<" ExportLoader -> <sequence>"<<dnaSequence<<"</sequence>"<<endl;
#endif

		Read t;
		t.constructor(dnaSequence,seqMyAllocator,true);
		reads->push_back(&t);

		loadedSequences++;
		m_loaded++;
	}

	if(m_loaded==m_size){
		fclose(m_f);
	}
}

int ExportLoader::getSize(){
	return m_size;
}

void ExportLoader::close(){
}
