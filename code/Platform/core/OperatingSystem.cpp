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

#include <core/OperatingSystem.h>
#include <iostream>
#include <string>
#include <fstream>
#include <assert.h>
using namespace std;

/*
 * Detect the operating system
 */

/* GNU/Linux */
#if defined(__linux__)
#define OS_POSIX

/* the GNU stack provides all we need */
#elif defined(__GNUC__)
#define OS_POSIX

/* assume Mac OS X and not old Mac OS */
#elif defined(__APPLE__) || defined(MACOSX)
#define OS_POSIX

/* assume Solaris or Linux or another POSIX system on Sun or Sun Sparc */
#elif defined(__sparc__) || defined(__sun__)
#define OS_POSIX

/* random UNIX system */
#elif defined(__unix__)
#define OS_POSIX

/* Windows 32 bits */
#elif defined(_WIN32) || defined(WIN32)
#define OS_WIN

/* Windoes 64 bits */
#elif defined(_WIN64) || defined(WIN64)
#define OS_WIN

/* Cygwin in Windows */
#elif defined(__CYGWIN__)
#define OS_POSIX

/* MinGW is not tested but probably works */
#elif defined(__MINGW32__)
#define OS_WIN

/* this will never be picked up because WIN32 or WIN64 will be picked up */
#elif defined(__BORLANDC__) 
#define OS_WIN

/* SGI IRIX I guess */
#elif defined(__sgi)
#define OS_POSIX

/* what is your operating system ? */
/* assume it is a POSIX one */
#else
#warning "What is your operating system ?"
#warning "Assuming it is a POSIX system..."
#define OS_POSIX

#endif

/* include some files */

#ifdef OS_POSIX

#include <unistd.h> /* getpid */
#include <time.h> /* possibly clock_gettime  */
#include <sys/time.h>  /* gettimeofday*/ 
#include <sys/stat.h>	/* mkdir */
#include <sys/types.h> /* mode_t */

#elif defined OS_WIN

#include <windows.h> /* GetCurrentProcessId */
			/* CreateDirectory */
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
	return getMicroseconds()/1000;
}

/** only ported to POSIX system */
uint64_t getMicroseconds(){
	#ifdef OS_POSIX

	struct timeval theTime;
	gettimeofday(&theTime,NULL);
	uint64_t seconds=theTime.tv_sec;
	uint64_t microSeconds=theTime.tv_usec;

	return seconds*1000*1000+microSeconds;

	#elif defined (OS_WIN)
	
	/* TODO: get microseconds is not implemented on Windows */
	// could start with this: http://www.decompile.com/cpp/faq/windows_timer_api.htm

	return 0;

	#endif
}

uint64_t getThreadMicroseconds(){
	#ifdef OS_POSIX

	#ifdef CONFIG_CLOCK_GETTIME
	struct timespec timeValue;

	int returnValue=clock_gettime(CLOCK_THREAD_CPUTIME_ID,&timeValue);

	if(returnValue != 0){
	}

	uint64_t seconds=timeValue.tv_sec;
	uint64_t nanoSeconds=timeValue.tv_nsec;

	return seconds*1000*1000 + nanoSeconds / 1000;

	#else

	return getMicroseconds();

	#endif

	#elif defined OS_WIN
	return getMicroseconds();
	#endif
}

/**
 * \see http://pubs.opengroup.org/onlinepubs/009695399/functions/mkdir.html
 * \see http://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
 * \see http://www.computing.net/answers/programming/c-createdirectory/13483.html
 *
 * \see http://msdn.microsoft.com/en-us/library/aa363855(v=vs.85).aspx
 */
void createDirectory(const char*directory){
	#ifdef OS_POSIX

	/* 
 * S_IRWXU
    read, write, execute/search by owner 
 *
 * S_IRWXG
 *  read, write, execute/search by group 
 *     */
	mode_t mode=S_IRWXU | S_IRWXG;

	#ifdef ASSERT
	int status=
	#endif

	mkdir(directory,mode);

	#ifdef ASSERT
	if(status!=0){
		cout<<"mkdir returned status "<<status<<" with directory= "<<directory<<endl;
	}

	assert(status==0);
	#endif
	
	#elif defined(OS_WIN)

	LPSECURITY_ATTRIBUTES attr=NULL;
	CreateDirectory(directory,attr);

	#endif
}

/** \see http://pubs.opengroup.org/onlinepubs/009695399/functions/stat.html
 * \see http://blog.kowalczyk.info/article/Check-if-file-exists-on-Windows.html */
bool fileExists(const char*file){
	#ifdef OS_POSIX
	struct stat st;
	int returnValue=stat(file,&st);
	
	bool theFileExists=(returnValue == 0);
	return theFileExists;
	
	#elif defined(OS_WIN)
	/* Return TRUE if file 'fileName' exists */
	DWORD fileAttr = GetFileAttributes(fileName);
	if(0xFFFFFFFF == fileAttr)
		return false;
    	return true;
	
	#else
	/* not implemented */
	#endif
}

void showMemoryUsage(int rank){
	uint64_t count=getMemoryUsageInKiBytes();
	cout<<"Rank "<<rank<<": assembler memory usage: "<<count<<" KiB"<<endl;
	cout.flush();
}

void printTheSeconds(int difference,ostream*stream){
	int minutes=difference/60;
	int seconds=difference%60;
	int hours=minutes/60;
	minutes=minutes%60;
	int days=hours/24;
	hours=hours%24;

	bool printed=false;

	if(days>0){
		(*stream)<<days<<" days";
		printed=true;
	}
	if(hours>0){
		if(printed){
			(*stream)<<", ";
		}
		printed=true;
		(*stream)<<hours<<" hours";
	}
	if(minutes>0){
		if(printed){
			(*stream)<<", ";
		}
		printed=true;
		(*stream)<<minutes<<" minutes";
	}

	if(printed){
		(*stream)<<", ";
	}
	(*stream)<<seconds<<" seconds";

}
