/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

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

#include<stdlib.h>
#include<plugin_SequencesLoader/ColorSpaceLoader.h>
#include<fstream>
#include<application_core/common_functions.h>
#include<iostream>
#include<string.h>
#include <assert.h>
using namespace std;

int ColorSpaceLoader::open(string file){
	m_f=fopen(file.c_str(),"r");
	m_size=0;
	m_loaded=0;
	char bufferForLine[RAY_MAXIMUM_READ_LENGTH];
	while(NULL!=fgets(bufferForLine,RAY_MAXIMUM_READ_LENGTH,m_f)){
		if(bufferForLine[0]=='#'){
			continue;// skip csfasta comment
		}

		if(bufferForLine[0]=='>'){
			char*returnValue=fgets(bufferForLine,RAY_MAXIMUM_READ_LENGTH,m_f);

			assert(returnValue != NULL);

			m_size++;
		}
	}
	fclose(m_f);
	m_f=fopen(file.c_str(),"r");
	return EXIT_SUCCESS;
}

void ColorSpaceLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	char bufferForLine[RAY_MAXIMUM_READ_LENGTH];
	int loadedSequences=0;
	while(m_loaded<m_size&& loadedSequences<maxToLoad){
		if(NULL==fgets(bufferForLine,RAY_MAXIMUM_READ_LENGTH,m_f))
			continue;

		if(bufferForLine[0]=='#'){
			continue;// skip csfasta comment
		}
		// read two lines
		if(bufferForLine[0]=='>'){
			char*returnValue=fgets(bufferForLine,RAY_MAXIMUM_READ_LENGTH,m_f);
			assert(returnValue != NULL);

			for(int j=0;j<(int)strlen(bufferForLine);j++){
				if(bufferForLine[j]==DOUBLE_ENCODING_A_COLOR){
					bufferForLine[j]='A';
				}else if(bufferForLine[j]==DOUBLE_ENCODING_T_COLOR){
					bufferForLine[j]='T';
				}else if(bufferForLine[j]==DOUBLE_ENCODING_C_COLOR){
					bufferForLine[j]='C';
				}else if(bufferForLine[j]==DOUBLE_ENCODING_G_COLOR){
					bufferForLine[j]='G';
				}
			}
			Read t;
			// remove the leading T & first color
			t.constructor(bufferForLine+2,seqMyAllocator,true);
			reads->push_back(&t);
			loadedSequences++;
			m_loaded++;
		}
	}
	if(m_loaded==m_size){
		fclose(m_f);
	}
}

int ColorSpaceLoader::getSize(){
	return m_size;
}
