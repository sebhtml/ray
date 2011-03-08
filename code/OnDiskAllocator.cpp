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

#include <OnDiskAllocator.h>
#include <assert.h>
#include <sstream>
using namespace std;

//  http://www.linuxquestions.org/questions/programming-9/mmap-tutorial-c-c-511265/
void OnDiskAllocator::constructor(int blockSize){
	ostringstream a;
	a<<"/tmp/cache_pid_"<<getpid();
	m_fileName=a.str();
	m_total=blockSize;
	int i;
	int result;

	m_fd = open(m_fileName.c_str(), O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
	if (m_fd == -1) {
		perror("Error opening file for writing");
	}

    /* Stretch the file size to the size of the (mmapped) array of ints
 *      */
    result = lseek(m_fd,m_total-1, SEEK_SET);
    if (result == -1) {
	close(m_fd);
	perror("Error calling lseek() to 'stretch' the file");
    }
    
    /* Something needs to be written at the end of the file to
 *      * have the file actually have the new size.
 *           * Just writing an empty string at the current file position will do.
 *                *
 *                     * Note:
 *                          *  - The current position in the file is at the end of the stretched 
 *                               *    file due to the call to lseek().
 *                                    *  - An empty string is actually a single '\0' character, so a zero-byte
 *                                         *    will be written at the last byte of the file.
 *                                              */
    result = write(m_fd, "", 1);
    if (result != 1) {
	close(m_fd);
	perror("Error writing last byte of the file");
    }

    /* Now the file is ready to be mmapped.
 *      */
    m_map =(char*)mmap(0, m_total, PROT_READ | PROT_WRITE, MAP_SHARED,m_fd, 0);
    if (m_map == MAP_FAILED) {
	close(m_fd);
	perror("Error mmapping the file");
    }
	m_current=0;
}

void*OnDiskAllocator::allocate(int a){
	uint64_t left=m_total-m_current;
	assert(left>=a);
	void*ret=m_map;
	m_current+=a;
	return ret;
}

void OnDiskAllocator::clear(){
    if (munmap(m_map, m_total) == -1) {
	perror("Error un-mmapping the file");
	/* Decide here whether to close(fd) and exit() or not. Depends... */
    }

    /* Un-mmaping doesn't close the file, so we still need to do that.
 *      */
    close(m_fd);
	remove(m_fileName.c_str());
}
