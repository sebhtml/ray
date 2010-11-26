
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
#include<sstream>
#include<iostream>
#include<FastaLoader.h>
#include<FastqLoader.h>

#ifdef HAVE_ZLIB
#include<FastqGzLoader.h>
#endif

#ifdef HAVE_LIBBZ2
#include<FastqBz2Loader.h>
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

int Loader::load(string file,vector<Read*>*reads,MyAllocator*seqMyAllocator,MyAllocator*readMyAllocator){
	{
		ifstream f(file.c_str());
		bool exists=f;
		f.close();
		if(!exists){
			cout<<"Ray: cannot access '"<<file<<"': No such file or directory"<<endl;
			return EXIT_FAILURE;// ERROR
		}
	}
	if(file.length()<4){
		(cout)<<"Error: "<<file<<endl;
		return EXIT_FAILURE;
	}
	string csfastaExtension=".csfasta";
	if(file.length()>=csfastaExtension.length() and
		file.substr(file.length()-csfastaExtension.length(),csfastaExtension.length())==csfastaExtension){
		ColorSpaceLoader loader;
		return loader.load(file,reads,seqMyAllocator,readMyAllocator);
	}
	if(file.substr(file.length()-4,4)==".sff"){
		SffLoader sffLoader;
		sffLoader.load(file,reads,seqMyAllocator,readMyAllocator);
		return m_bases=sffLoader.getBases();
	}
	if(file.substr(file.length()-6,6)==".fasta"){
		FastaLoader loader;
		return loader.load(file,reads,seqMyAllocator,readMyAllocator);
	}

	if(file.substr(file.length()-6,6)==".fastq"){
		FastqLoader loader;
		return loader.load(file,reads,seqMyAllocator,readMyAllocator);
	}

	#ifdef HAVE_ZLIB
	if(file.substr(file.length()-9,9)==".fastq.gz"){
		FastqGzLoader loader;
		return loader.load(file,reads,seqMyAllocator,readMyAllocator,4);
	}

	if(file.substr(file.length()-9,9)==".fasta.gz"){
		FastqGzLoader loader;
		return loader.load(file,reads,seqMyAllocator,readMyAllocator,2);
	}

	#endif


	#ifdef HAVE_LIBBZ2
	if(file.substr(file.length()-10,10)==".fastq.bz2"){
		FastqBz2Loader loader;
		return loader.load(file,reads,seqMyAllocator,readMyAllocator,4);
	}

	if(file.substr(file.length()-10,10)==".fasta.bz2"){
		FastqBz2Loader loader;
		return loader.load(file,reads,seqMyAllocator,readMyAllocator,2);
	}

	#endif
	
	cout<<file<<": unknown extension, exiting. (see Ray --help for valid extensions)"<<endl;

	return EXIT_FAILURE;
}

int Loader::getBases(){
	return m_bases;
}


