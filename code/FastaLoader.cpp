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

#include<FastaLoader.h>
#include<fstream>

int FastaLoader::open(string file){
	m_f.open(file.c_str());
	m_size=0;

	string buffer;
	while(!m_f.eof()){
		buffer="";
		m_f>>buffer;
		if(buffer=="")
			continue;
		if(buffer[0]=='>'){
			m_size++;
		}
	}
	m_f.close();
	m_f.open(file.c_str());
	return EXIT_SUCCESS;
}

void FastaLoader::load(int maxToLoad,ArrayOfReads*reads,MyAllocator*seqMyAllocator){
	string id;
	ostringstream sequence;
	string buffer;
	int loadedSequences=0;

	while(!m_f.eof() && loadedSequences<maxToLoad-1){
		buffer="";
		m_f>>buffer;
		if(buffer=="")
			continue;
		if(buffer[0]=='>'){
			char bufferForLine[1024];
			m_f.getline(bufferForLine,1024);
			if(id!=""){
				loadedSequences++;
				string sequenceStr=sequence.str();

				Read t;
				t.copy(NULL,sequenceStr.c_str(),seqMyAllocator,true);
				reads->push_back(&t);
			}
			id=buffer;
			sequence.str("");
		}else{
			sequence<< buffer;
		}
	}

	if(m_f.eof()){
		m_f.close();
	}

	string sequenceStr=sequence.str();

	Read t;
	t.copy(NULL,sequenceStr.c_str(),seqMyAllocator,true);
	reads->push_back(&t);
}

int FastaLoader::getSize(){
	return m_size;
}
