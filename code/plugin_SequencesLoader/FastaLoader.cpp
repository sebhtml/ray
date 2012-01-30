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

#include<plugin_SequencesLoader/FastaLoader.h>
#include<fstream>
#include <stdlib.h>

/** load sequences */
int FastaLoader::load(string file,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
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

				Read t;
				t.constructor(sequenceStr.c_str(),seqMyAllocator,true);
				reads->push_back(&t);
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
	Read t;
	t.constructor(sequenceStr.c_str(),seqMyAllocator,true);
	reads->push_back(&t);

	f.close();
	return EXIT_SUCCESS;
}
