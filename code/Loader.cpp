
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
#include<sstream>
#include<iostream>
#include<FastaLoader.h>
#include<FastqLoader.h>

#ifdef HAVE_ZLIB_H
#include<FastqGzLoader.h>
#endif

#include<string>
#include<vector>
#include<ColorSpaceLoader.h>
#include<Loader.h>
#include<SffLoader.h>
#include<stdlib.h>
#include<Read.h>
using namespace std;

Loader::Loader(){
	m_bases=0;
	m_total=0;
}

int Loader::getReads(){
	return m_total;
}

void Loader::load(string file,vector<Read*>*reads,MyAllocator*seqMyAllocator,MyAllocator*readMyAllocator){
	{
		ifstream f(file.c_str());
		bool exists=f;
		f.close();
		if(!exists){
			cout<<"Ray: cannot access "<<file<<": No such file or directory"<<endl;
			return;
		}
	}
	if(file.length()<4){
		(cout)<<"Error: "<<file<<endl;
		exit(0);
	}
	string csfastaExtension=".csfasta";
	if(file.length()>=csfastaExtension.length() and
		file.substr(file.length()-csfastaExtension.length(),csfastaExtension.length())==csfastaExtension){
		ColorSpaceLoader loader;
		loader.load(file,reads,seqMyAllocator,readMyAllocator);
		return;
	}
	if(file.substr(file.length()-4,4)==".sff"){
		SffLoader sffLoader;
		sffLoader.load(file,reads,seqMyAllocator,readMyAllocator);
		m_bases=sffLoader.getBases();
		return;
	}
	if(file.substr(file.length()-6,6)==".fasta"){
		FastaLoader loader;
		loader.load(file,reads,seqMyAllocator,readMyAllocator);
		return;
	}

	if(file.substr(file.length()-6,6)==".fastq"){
		FastqLoader loader;
		loader.load(file,reads,seqMyAllocator,readMyAllocator);
		return;
	}

	#ifdef HAVE_ZLIB_H
	if(file.substr(file.length()-9,9)==".fastq.gz"){
		FastqGzLoader loader;
		loader.load(file,reads,seqMyAllocator,readMyAllocator,4);
		return;
	}

	if(file.substr(file.length()-9,9)==".fasta.gz"){
		FastqGzLoader loader;
		loader.load(file,reads,seqMyAllocator,readMyAllocator,2);
		return;
	}


	#endif

}

int Loader::getBases(){
	return m_bases;
}


