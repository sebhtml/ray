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

#include<FastqBz2Loader.h>
#include<fstream>
#include<BzReader.h>

// a very simple and compact fastq.gz reader
int FastqBz2Loader::load(string file,vector<Read>*reads,MyAllocator*seqMyAllocator,int period){
	BzReader reader;
	reader.open(file.c_str());
	char buffer[4096];
	int rotatingVariable=0;
	while(NULL!=reader.readLine(buffer,4096)){
		if(rotatingVariable==1){
			Read t;
			t.copy(NULL,buffer,seqMyAllocator,true);
			reads->push_back(t);
		}
		rotatingVariable++;
		if(rotatingVariable==period){
			rotatingVariable=0;
		}
	}
	reader.close();
	return EXIT_SUCCESS;
}

