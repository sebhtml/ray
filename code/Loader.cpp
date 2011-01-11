
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
    along with this program (COPYING).  
	see <http://www.gnu.org/licenses/>

*/
#include<sstream>
#include<iostream>
#include<assert.h>
#include<string>
#include<vector>
#include<Loader.h>
#include<stdlib.h>
#include<Read.h>
using namespace std;

Loader::Loader(){
	m_maxToLoad=500000;
	DISTRIBUTION_ALLOCATOR_CHUNK_SIZE=m_maxToLoad*100;
	m_currentOffset=0;
	m_type=FORMAT_NULL;
}

int Loader::load(string file,bool isGenome){
	m_allocator.constructor(DISTRIBUTION_ALLOCATOR_CHUNK_SIZE);
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

	if(isGenome){
		m_fasta.load(file,&m_reads,&m_allocator);
		m_size=m_reads.size();
		return EXIT_SUCCESS;
	}

	string csfastaExtension=".csfasta";
	if(file.length()>=csfastaExtension.length() &&
		file.substr(file.length()-csfastaExtension.length(),csfastaExtension.length())==csfastaExtension){
		m_type=FORMAT_CSFASTA;
		int ret=m_color.open(file);
		m_size=m_color.getSize();	
		return ret;
	}
	if(file.substr(file.length()-4,4)==".sff"){
		m_type=FORMAT_SFF;
		int ret=m_sff.open(file);
		m_size=m_sff.getSize();
		return ret;
		
	}
	if(file.substr(file.length()-6,6)==".fasta"){
		m_type=FORMAT_FASTA;
		int ret=m_fastq.open(file,2);
		m_size=m_fastq.getSize();
		return ret;
	}

	if(file.substr(file.length()-6,6)==".fastq"){
		m_type=FORMAT_FASTQ;
		int ret=m_fastq.open(file,4);
		m_size=m_fastq.getSize();
		return ret;
	}

	#ifdef HAVE_ZLIB
	if(file.substr(file.length()-9,9)==".fastq.gz"){
		m_type=FORMAT_FASTQ_GZ;
		int ret=m_fastqgz.open(file,4);
		m_size=m_fastqgz.getSize();
		return ret;
	}

	if(file.substr(file.length()-9,9)==".fasta.gz"){
		m_type=FORMAT_FASTA_GZ;
		int ret=m_fastqgz.open(file,2);
		m_size=m_fastqgz.getSize();
		return ret;
	}
	#endif

	#ifdef HAVE_LIBBZ2
	if(file.substr(file.length()-10,10)==".fastq.bz2"){
		m_type=FORMAT_FASTQ_BZ2;
		int ret=m_fastqbz2.open(file,4);
		m_size=m_fastqbz2.getSize();
		return ret;
	}

	if(file.substr(file.length()-10,10)==".fasta.bz2"){
		m_type=FORMAT_FASTA_BZ2;
		int ret=m_fastqbz2.open(file,2);
		m_size=m_fastqbz2.getSize();
		return ret;
	}
	#endif
	
	cout<<file<<": unknown extension, exiting. (see Ray --help for valid extensions)"<<endl;

	return EXIT_FAILURE;
}

Read*Loader::at(uint64_t i){
	#ifdef ASSERT
	assert(i<m_size);
	#endif

	//cout<<"at "<<i<<endl;
	if(i>=m_currentOffset+m_reads.size()){
		loadSequences();
		//cout<<"Loaded "<<m_reads.size()<<endl;
	}
	#ifdef ASSERT
	if(i>=m_currentOffset+m_reads.size()){
		cout<<"i="<<i<<" offset="<<m_currentOffset<<" Loaded="<<m_reads.size()<<endl;
	}
	assert(i<m_currentOffset+m_reads.size());
	#endif
	return m_reads.at(i-m_currentOffset);
}

uint64_t Loader::size(){
	return m_size;
}

void Loader::clear(){
	m_reads.clear();
	m_allocator.clear();
	m_size=0;
	m_currentOffset=0;
	m_type=FORMAT_NULL;

	#ifdef ASSERT
	assert(m_reads.size()==0);
	#endif
}

void Loader::loadSequences(){
	m_currentOffset+=m_reads.size();
	m_reads.clear();
	m_allocator.clear();
	m_allocator.constructor(DISTRIBUTION_ALLOCATOR_CHUNK_SIZE);

	if(m_type==FORMAT_FASTQ_GZ){
		#ifdef HAVE_ZLIB
		m_fastqgz.load(m_maxToLoad,&m_reads,&m_allocator,4);
		#endif
	}else if(m_type==FORMAT_FASTQ){
		m_fastq.load(m_maxToLoad,&m_reads,&m_allocator,4);
	}else if(m_type==FORMAT_FASTQ_BZ2){
		#ifdef HAVE_LIBBZ2
		m_fastqbz2.load(m_maxToLoad,&m_reads,&m_allocator,4);
		#endif
	}else if(m_type==FORMAT_CSFASTA){
		m_color.load(m_maxToLoad,&m_reads,&m_allocator);
	}else if(m_type==FORMAT_SFF){
		m_sff.load(m_maxToLoad,&m_reads,&m_allocator);
	}else if(m_type==FORMAT_FASTA){
		m_fastq.load(m_maxToLoad,&m_reads,&m_allocator,2);
	}else if(m_type==FORMAT_FASTA_BZ2){
		#ifdef HAVE_LIBBZ2
		m_fastqbz2.load(m_maxToLoad,&m_reads,&m_allocator,2);
		#endif
	}else if(m_type==FORMAT_FASTA_GZ){
		#ifdef HAVE_ZLIB
		m_fastqgz.load(m_maxToLoad,&m_reads,&m_allocator,2);
		#endif
	}
	//cout<<"Offset= "<<m_currentOffset<<" "<<m_reads.size()<<endl;
}
