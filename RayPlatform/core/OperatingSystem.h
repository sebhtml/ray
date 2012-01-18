/*
 	Ray
    Copyright (C) 2010, 2011  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You have received a copy of the GNU Lesser General Public License
    along with this program (lgpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _OperatingSystem_h
#define _OperatingSystem_h

/* EXIT_SUCCESS 0 (defined in stdlib.h) */
#define EXIT_NEEDS_ARGUMENTS 5
#define EXIT_NO_MORE_MEMORY 42

/*
 Open-MPI eager threshold is 4k (4096), and this include Open-MPI's metadata.
 tests show that 4096-100 bytes are sent eagerly, too.
 divide that by eight and you get the number of 64-bit integers 
 allowed in a single eager communication

 * "4096 is rendezvous. For eager, try 4000 or lower. "
 *  --Eugene Loh  (Oracle)
 *  http://www.open-mpi.org/community/lists/devel/2010/11/8700.php
 *
 */

#define MAXIMUM_MESSAGE_SIZE_IN_BYTES 4000

#define MASTER_RANK 0

/* the maximum of processes is utilized to construct unique hyperfusions IDs */
// with routing enabled, MAX_NUMBER_OF_MPI_PROCESSES is 4096
#define MAX_NUMBER_OF_MPI_PROCESSES 1000000
#define INVALID_RANK MAX_NUMBER_OF_MPI_PROCESSES



/** only this file knows the operating system */
#include <string>
//#include <core/constants.h> 
#include <communication/MessagesHandler.h>
using namespace std;

/** show memory usage */
uint64_t getMemoryUsageInKiBytes();

string getOperatingSystem();

void showDate();

/**
 * get the process identifier 
 */
int portableProcessId();

uint64_t getMilliSeconds();

void showMemoryUsage(int rank);

uint64_t getMicroseconds();

uint64_t getThreadMicroseconds();

/** create a directory */
void createDirectory(const char*directory);

bool fileExists(const char*file);

void printTheSeconds(int seconds,ostream*stream);



#endif
