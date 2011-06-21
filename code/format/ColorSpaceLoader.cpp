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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include<stdlib.h>
#include<format/ColorSpaceLoader.h>
#include<fstream>
#include<core/common_functions.h>
#include<iostream>
#include<string.h>
using namespace std;

#define _ENCODING_CHAR_A '0'
#define _ENCODING_CHAR_T '1'
#define _ENCODING_CHAR_C '2'
#define _ENCODING_CHAR_G '3'

int ColorSpaceLoader::open(string file){
	m_f.open(file.c_str());
	m_size=0;
	m_loaded=0;

	char bufferForLine[1024];
	while(!m_f.eof()){
		m_f.getline(bufferForLine,1024);
		if(bufferForLine[0]=='#'){
			continue;// skip csfasta comment
		}
		if(bufferForLine[0]=='>'){
			m_f.getline(bufferForLine,1024);
			m_size++;
		}
	}

	m_f.close();
	m_f.open(file.c_str());
	return EXIT_SUCCESS;
}

void ColorSpaceLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	char bufferForLine[1024];
	int loadedSequences=0;
	while(m_loaded<m_size&& loadedSequences<maxToLoad){
		m_f.getline(bufferForLine,1024);
		if(bufferForLine[0]=='#'){
			continue;// skip csfasta comment
		}
		// read two lines
		if(bufferForLine[0]=='>'){
			m_f.getline(bufferForLine,1024);
			for(int j=0;j<(int)strlen(bufferForLine);j++){
				if(bufferForLine[j]==_ENCODING_CHAR_A){
					bufferForLine[j]='A';
				}else if(bufferForLine[j]==_ENCODING_CHAR_T){
					bufferForLine[j]='T';
				}else if(bufferForLine[j]==_ENCODING_CHAR_C){
					bufferForLine[j]='C';
				}else if(bufferForLine[j]==_ENCODING_CHAR_G){
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
		m_f.close();
	}
}

int ColorSpaceLoader::getSize(){
	return m_size;
}
