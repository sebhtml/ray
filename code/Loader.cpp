
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


#include<string>
#include<vector>
#include<Loader.h>
#include<stdlib.h>
#include<Read.h>
using namespace std;

Loader::Loader(){
	DISTRIBUTION_ALLOCATOR_CHUNK_SIZE=300000000;
	m_allocator.constructor(DISTRIBUTION_ALLOCATOR_CHUNK_SIZE);
}

int Loader::load(string file){
	ifstream f(file.c_str());
	bool exists=f;
	f.close();
	if(!exists){
		cout<<"Ray: cannot access '"<<file<<"': No such file or directory"<<endl;
		return EXIT_FAILURE;// ERROR
	}

	if(file.length()<4){
		(cout)<<"Error: "<<file<<endl;
		return EXIT_FAILURE;
	}
	string csfastaExtension=".csfasta";
	if(file.length()>=csfastaExtension.length() &&
		file.substr(file.length()-csfastaExtension.length(),csfastaExtension.length())==csfastaExtension){
		return m_color.load(file,&m_reads,&m_allocator);
	}
	if(file.substr(file.length()-4,4)==".sff"){
		return m_sff.load(file,&m_reads,&m_allocator);
	}
	if(file.substr(file.length()-6,6)==".fasta"){
		return m_fasta.load(file,&m_reads,&m_allocator);
	}

	if(file.substr(file.length()-6,6)==".fastq"){
		return m_fastq.load(file,&m_reads,&m_allocator);
	}

	#ifdef HAVE_ZLIB
	if(file.substr(file.length()-9,9)==".fastq.gz"){
		return m_fastqgz.load(file,&m_reads,&m_allocator,4);
	}

	if(file.substr(file.length()-9,9)==".fasta.gz"){
		return m_fastqgz.load(file,&m_reads,&m_allocator,2);
	}
	#endif

	#ifdef HAVE_LIBBZ2
	if(file.substr(file.length()-10,10)==".fastq.bz2"){
		return m_fastqbz2.load(file,&m_reads,&m_allocator,4);
	}

	if(file.substr(file.length()-10,10)==".fasta.bz2"){
		return m_fastqbz2.load(file,&m_reads,&m_allocator,2);
	}
	#endif
	
	cout<<file<<": unknown extension, exiting. (see Ray --help for valid extensions)"<<endl;

	return EXIT_FAILURE;
}

Read*Loader::at(int i){
	return m_reads.at(i);
}

int Loader::size(){
	return m_reads.size();
}

void Loader::clear(){
	m_reads.clear();
}
