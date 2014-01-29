
/*
 	Ray
    Copyright (C) 2010, 2011, 2012, 2013 Sébastien Boisvert

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

#include "Loader.h"
#include "Read.h"

#include <sstream>
#include <iostream>
#include <assert.h>
#include <string>
#include <vector>
#include <stdlib.h>
using namespace std;

void Loader::constructor(const char*prefix,bool show,Rank rank){

	m_rank=rank;

	m_maxToLoad=500000;
	m_currentOffset=0;
	//m_type=FORMAT_NULL;
	ostringstream prefixFull;
	prefixFull<<prefix<<"_Loader";
	m_show=show;
	m_allocator.constructor(4194304,"RAY_MALLOC_TYPE_LOADER_ALLOCATOR",m_show);
	m_reads.constructor(&m_allocator);
}

int Loader::load(string file,bool isGenome){
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

#if 0
	if(isGenome){
		m_fasta.load(file,&m_reads,&m_allocator);
		m_size=m_reads.size();
		return EXIT_SUCCESS;
	}
#endif

	cout<<"Rank "<<m_rank<<" is fetching file "<<file<<" with lazy loading (please wait...)"<<endl;

	m_interface=m_factory.makeLoader(file);

	if(m_interface!=NULL){
		m_interface->open(file);
		m_size=m_interface->getSize();
		return EXIT_SUCCESS;
	}
	
	cout<<"Error: "<<file<<": unknown extension, exiting. (see Ray --help for valid extensions)"<<endl;

	return EXIT_FAILURE;
}

Read*Loader::at(LargeIndex i){
	#ifdef CONFIG_ASSERT
	assert(i<m_size);
	#endif

	if(i>=m_currentOffset+m_reads.size()){
		loadSequences();
	}

	#ifdef CONFIG_ASSERT
	if(i>=m_currentOffset+m_reads.size()){
		cout<<"i= "<<i<<" m_currentOffset= " << m_currentOffset<<" m_reads.size: " << m_reads.size()<<endl;
	}
	assert(i<m_currentOffset+m_reads.size());
	#endif

	return m_reads.at(i-m_currentOffset);
}

LargeCount Loader::size(){
	return m_size;
}

void Loader::clear(){
	m_reads.clear();
	m_allocator.clear();
	m_size=0;
	m_currentOffset=0;
	//m_type=FORMAT_NULL;

	#ifdef CONFIG_ASSERT
	assert(m_reads.size()==0);
	#endif
}

void Loader::loadSequences(){
	//cout << "[DEBUG] loadSequences." << endl;

	m_currentOffset += m_reads.size();
	m_allocator.reset();
	m_reads.reset();

	if(m_interface==NULL) {

		//cout << "[DEBUG] no interface found" << endl;
		return;
	}

	m_interface->load(m_maxToLoad,&m_reads,&m_allocator);

	//cout << "[DEBUG] loaded " << m_reads.size() << " entries into memory" << endl;
}

void Loader::reset(){
	m_allocator.reset();
	m_reads.reset();
	m_size=0;
	m_currentOffset=0;

	if(m_interface!=NULL){
		m_interface->close();
		m_interface=NULL;
	}

	//m_type=FORMAT_NULL;

	#ifdef CONFIG_ASSERT
	assert(m_reads.size()==0);
	#endif
}
