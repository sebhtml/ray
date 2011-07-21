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

/** only this file knows the operating system */
#include <string>
#include <core/constants.h> 
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

void getMicroSeconds(uint64_t*seconds,uint64_t*microSeconds);
