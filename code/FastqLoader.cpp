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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/

#include<FastqLoader.h>
#include<fstream>

int FastqLoader::load(string file,vector<Read*>*reads,MyAllocator*seqMyAllocator,MyAllocator*readMyAllocator){
	ifstream f(file.c_str());
	string id;
	ostringstream sequence;
	ostringstream quality;
	int seq=0;
	int qual=1;
	int mode=seq;
	string buffer;
	while(!f.eof()){
		buffer="";
		f>>buffer;
		if(buffer=="")
			continue;
		if(buffer[0]=='@'&&!(mode==qual&&quality.str().length()==0)){
			char bufferForLine[1024];
			f.getline(bufferForLine,1024);
			if(id!=""){
				Read*t=(Read*)readMyAllocator->allocate(sizeof(Read));
				t->copy(NULL,sequence.str().c_str(),readMyAllocator);// remove the leading T & first color
				reads->push_back(t);
			}
			id=buffer;
			sequence.str("");
			quality.str("");
			mode=seq;
		}else if(buffer[0]=='+'&&!(mode==qual&&quality.str().length()==0)){
			char bufferForLine[1024];
			f.getline(bufferForLine,1024);
			mode=qual;
		}else if(mode==qual){
			quality<< buffer;
		}else if(mode==seq){
			sequence<< buffer;
		}
	}
	Read*t=(Read*)readMyAllocator->allocate(sizeof(Read));
	t->copy(NULL,sequence.str().c_str(),readMyAllocator);// remove the leading T & first color
	reads->push_back(t);
	f.close();
	return EXIT_SUCCESS;
}

