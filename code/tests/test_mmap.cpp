#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>
#include <unistd.h>
#include <string>
#include <fcntl.h>
#include <map>
#include <vector>
#include <sys/mman.h> 
#include <string>
#include <iostream>
using namespace std;

int main(){
	string fileName="test.bin";
	uint64_t m_chunkSize=1024*1024*1024;

	int fd = open(fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	int result = lseek(fd,m_chunkSize-1, SEEK_SET);
	result = write(fd, "", 1);
	char*map =(char*)mmap(0, m_chunkSize, PROT_READ | PROT_WRITE,MAP_PRIVATE,fd, 0);
	cout<<"mmapped"<<endl;
	for(int i=0;i<m_chunkSize;i++){
		map[i]=0;
	}
	cout<<"set to 0"<<endl;
	munmap(map,m_chunkSize);
	map =(char*)mmap(map, m_chunkSize, PROT_READ | PROT_WRITE,MAP_PRIVATE|MAP_FIXED,fd, 0);
	cout<<"munmap/mmap"<<endl;
	map[0]=1;
	map[100]=2;
	map[9000]=8;
	sleep(1000);

	return 0;
}
