/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#include<FastaLoader.h>
#include<fstream>

int FastaLoader::load(string file,vector<Read*>*reads,MyAllocator*seqMyAllocator,MyAllocator*readMyAllocator){
	string id;
	ostringstream sequence;
	string buffer;
	ifstream f(file.c_str());
	while(!f.eof()){
		buffer="";
		f>>buffer;
		if(buffer=="")
			continue;
		if(buffer[0]=='>'){
			char bufferForLine[1024];
			f.getline(bufferForLine,1024);
			if(id!=""){
				string sequenceStr=sequence.str();

				Read*t=(Read*)readMyAllocator->allocate(sizeof(Read));
				t->copy(NULL,sequenceStr.c_str(),readMyAllocator);// remove the leading T & first color
				reads->push_back(t);
			}
			id=buffer;
			sequence.str("");
		}else{
			sequence<< buffer;
		}
	}
	string sequenceStr=sequence.str();
	ostringstream quality;
	for(int i=0;i<(int)sequenceStr.length();i++){
		quality<< "F";
	}
	Read*t=(Read*)readMyAllocator->allocate(sizeof(Read));
	t->copy(NULL,sequenceStr.c_str(),readMyAllocator);// remove the leading T & first color
	reads->push_back(t);

	f.close();
	return EXIT_SUCCESS;
}
