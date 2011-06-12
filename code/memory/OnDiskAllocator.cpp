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


 	Funding:

Sébastien Boisvert has a scholarship from the Canadian Institutes of Health Research (Master's award: 200910MDR-215249-172830 and Doctoral award: 200902CGM-204212-172830).

*/

#include <core/common_functions.h>
#include <memory/OnDiskAllocator.h>
#include <assert.h>
#include <sstream>
#include <iostream>
#ifdef OS_POSIX
#include <sys/mman.h> // POSIX memory managemen
#endif
using namespace std;

//  http://www.linuxquestions.org/questions/programming-9/mmap-tutorial-c-c-511265/
void OnDiskAllocator::constructor(const char*prefix){
	m_prefix=prefix;
	m_chunkSize=1024*1024*128; // 128 MiB
}

void OnDiskAllocator::addChunk(){
	#ifdef OS_POSIX
	int chunkId=m_pointers.size();
	ostringstream a;
	a<<m_prefix<<"_RaySystems_pid_"<<getpid()<<"_chunk_"<<chunkId<<".mmap";
	string fileName=a.str();
	int fd=-1;

	fd = open(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (fd == -1) {
		perror("Error opening file for writing");
	}

	// go at the new end
	int result = lseek(fd,m_chunkSize-1, SEEK_SET);
	if (result == -1) {
		close(fd);
		perror("Error calling lseek() to 'stretch' the file");
	}
    	// write an empty string to update the file
	result = write(fd, "", 1);
	if (result != 1) {
		close(fd);
		perror("Error writing last byte of the file");
	}
	char*map=NULL;
	// MAP_PRIVATE changes are private
	// MAP_SHARED would make changes public (force writting; better)
	map =(char*)mmap(0, m_chunkSize, PROT_READ | PROT_WRITE,MAP_PRIVATE,fd, 0);
	if (map == MAP_FAILED||map==NULL) {
		close(fd);
		perror("Error mmapping the file");
	}

	m_pointers.push_back(map);
	m_fileDescriptors.push_back(fd);
	m_fileNames.push_back(fileName);
	cout<<"mmap "<<fileName<<" ("<<m_chunkSize/1024/1024<<" MiB)"<<endl;
	m_current=0;
	#endif
}

void*OnDiskAllocator::allocate(int a){
	int remainder=a%8;
	int toAdd=8-remainder;
	a+=toAdd;
	assert(a!=0);
	if((uint64_t)a>m_chunkSize){
		cout<<"Error: requested "<<a<<" but chunk size is "<<m_chunkSize<<endl;
	}
	assert((uint64_t)a<=m_chunkSize);
	if(hasAddressesToReuse(a)){
		return reuseAddress(a);
	}

	if(m_pointers.size()==0){
		addChunk();
	}
	uint64_t left=m_chunkSize-m_current;
	if(left<(uint64_t)a){
		addChunk();
		left=m_chunkSize-m_current;
	}
	assert(left>=(uint64_t)a);
	void*ret=m_pointers[m_pointers.size()-1]+m_current;
	m_current+=a;
	return ret;
}

void OnDiskAllocator::clear(){
	for(int i=0;i<(int)m_pointers.size();i++){
		char*map=m_pointers[i];
		if (munmap(map, m_chunkSize) == -1) {
			perror("Error un-mmapping the file");
		}

		int fd=m_fileDescriptors[i];
		close(fd);

		string fileName=m_fileNames[i];
		remove(fileName.c_str());
		cout<<"munmap "<<fileName<<endl;
	}
	m_pointers.clear();
	m_fileNames.clear();
	m_fileDescriptors.clear();
	m_toReuse.clear();
}

void OnDiskAllocator::free(void*a,int b){
	addAddressToReuse(a,b);
}

bool OnDiskAllocator::hasAddressesToReuse(int size){
	bool test=m_toReuse.count(size)>0;
	#ifdef ASSERT
	if(test){
		assert(m_toReuse[size]!=NULL);
	}
	#endif
	return test;
}

void*OnDiskAllocator::reuseAddress(int size){
	#ifdef ASSERT
	assert(m_toReuse.count(size)>0 && m_toReuse[size]!=NULL);
	#endif

	StoreElement*tmp=m_toReuse[size];
	#ifdef ASSERT
	assert(tmp!=NULL);
	#endif
	StoreElement*next=(StoreElement*)tmp->m_next;
	m_toReuse[size]=next;
	if(m_toReuse[size]==NULL){
		m_toReuse.erase(size);
	}
	#ifdef ASSERT
	if(m_toReuse.count(size)>0){
		assert(m_toReuse[size]!=NULL);
	}
	#endif
	return tmp;
}

void OnDiskAllocator::addAddressToReuse(void*p,int size){
	if(size<(int)sizeof(StoreElement)){
		return;
	}
	#ifdef ASSERT
	assert(p!=NULL);
	#endif
	StoreElement*ptr=(StoreElement*)p;
	ptr->m_next=NULL;
	if(m_toReuse.count(size)>0){
		StoreElement*next=m_toReuse[size];
		#ifdef ASSERT
		assert(next!=NULL);
		#endif
		ptr->m_next=next;
	}
	m_toReuse[size]=ptr;
}

void OnDiskAllocator::reset(){
	m_toReuse.clear();
	if(m_pointers.size()>1){
		clear();
	}else{
		m_current=0;
	}
}

