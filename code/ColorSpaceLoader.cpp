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
#include<ColorSpaceLoader.h>
#include<fstream>
#include<common_functions.h>
#include<iostream>
#include<string.h>
using namespace std;


#define _ENCODING_CHAR_A '0'
#define _ENCODING_CHAR_T '1'
#define _ENCODING_CHAR_C '2'
#define _ENCODING_CHAR_G '3'

ColorSpaceLoader::ColorSpaceLoader(){
}

int ColorSpaceLoader::load(string file,vector<Read>*reads,MyAllocator*seqMyAllocator){
	ifstream f(file.c_str());
	char bufferForLine[1024];
	int i=0;
	while(!f.eof()){
		f.getline(bufferForLine,1024);
		if(bufferForLine[0]=='#'){
			continue;// skip csfasta comment
		}
		if(bufferForLine[0]=='>'){
			f.getline(bufferForLine,1024);
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
			t.copy(NULL,bufferForLine+2,seqMyAllocator,true);// remove the leading T & first color
			reads->push_back(t);
			i++;
		}
	}
	f.close();
	return EXIT_SUCCESS;
}

