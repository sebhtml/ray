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

#include <core/OperatingSystem.h>
#include <iostream>
#include <string>
#include <fstream>
using namespace std;

/*
 * Detect the operating system
 */
#if defined(_WIN32) || defined(WIN32) || defined(_WIN64) || defined(WIN64) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__) 
#define OS_WIN
#else
#define OS_POSIX
#endif

#ifdef OS_POSIX
#include <unistd.h> /* getpid */
#include <time.h> /* gettimeofday*/
#include <sys/time.h>  /* possibly clock_gettime  */
#endif
#ifdef OS_WIN
#include <windows.h> /* GetCurrentProcessId */
#endif

/** print the date, not necessary */
void showDate(){
	#ifdef OS_POSIX
	time_t m_endingTime=time(NULL);
	struct tm * timeinfo;
	timeinfo=localtime(&m_endingTime);
	cout<<"Date: "<<asctime(timeinfo);
	#endif
}

int portableProcessId(){
	#ifdef OS_POSIX
	return getpid();
	#elif defined(OS_WIN)
	return GetCurrentProcessId();
	#else
	return -1;
	#endif
}

/** get the operating system */
string getOperatingSystem(){
	#ifdef OS_WIN
	return "Microsoft Windows (OS_WIN)";
	#else
		#ifdef __linux__
		return "Linux (__linux__) POSIX (OS_POSIX)";
		#else
		return "POSIX (OS_POSIX)";
		#endif
	#endif
	return "Unknown";
}

/** only ported to Linux */
uint64_t getMemoryUsageInKiBytes(){
	uint64_t count=0;

	#ifdef __linux__
	ifstream f("/proc/self/status");
	while(!f.eof()){
		string key;
		f>>key;
		if(key=="VmData:"){
			f>>count;
			break;
		}
	}
	f.close();
	#endif

	return count;
}

/** real-time only ported to real-time POSIX systems */
uint64_t getMilliSeconds(){
	uint64_t milliSeconds=0;

	#ifdef OS_POSIX
	#ifdef HAVE_CLOCK_GETTIME
	timespec temp;
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&temp);
	uint64_t seconds=temp.tv_sec;
	uint64_t nanoseconds=temp.tv_nsec;
	milliSeconds=seconds*1000+nanoseconds/1000/1000;
	#endif
	#endif

	return milliSeconds;
}

/** only ported to POSIX system */
void getMicroSeconds(uint64_t*seconds,uint64_t*microSeconds){
	#ifdef OS_POSIX
	struct timeval theTime;
	gettimeofday(&theTime,NULL);
	(*seconds)=theTime.tv_sec;
	(*microSeconds)=theTime.tv_usec;
	#endif
}
