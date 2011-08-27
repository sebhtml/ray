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
#include <assert.h>
#include <structures/Kmer.h>
#include <structures/Vertex.h>
#include <structures/Direction.h>
#include <structures/PairedRead.h>
#include <structures/Read.h>

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
#include <time.h> /* gettimeofday*/
#include <sys/time.h>  /* possibly clock_gettime  */
#include <sys/stat.h>	/* mkdir */
#include <sys/types.h> /* mode_t */
#endif
#ifdef OS_WIN
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

/**
 * \see http://pubs.opengroup.org/onlinepubs/009695399/functions/mkdir.html
 * \see http://pubs.opengroup.org/onlinepubs/7908799/xsh/sysstat.h.html
 * \see http://www.computing.net/answers/programming/c-createdirectory/13483.html
 *
 * \see http://msdn.microsoft.com/en-us/library/aa363855(v=vs.85).aspx
 */
void createDirectory(char*directory){
	#ifdef OS_POSIX

	/* read, write for owner */
	/* read, write for group */
	mode_t mode=S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
	int status=mkdir(directory,mode);

	assert(status==0);
	
	#elif defined(OS_WIN)

	LPSECURITY_ATTRIBUTES attr=NULL;
	CreateDirectory(directory,attr);

	#endif
}

void showRayVersionShort(){
	cout<<"Ray version "<<RAY_VERSION<<" ";

	#if defined(RAY_64_BITS)
	cout<<"RAY_64_BITS"<<endl;
	#elif defined(RAY_32_BITS)
	cout<<"RAY_32_BITS"<<endl;
	#endif

	cout<<"License: GNU General Public License"<<endl;

	cout<<endl;
	cout<<"MAXKMERLENGTH: "<<MAXKMERLENGTH<<endl;
	cout<<"KMER_U64_ARRAY_SIZE: "<<KMER_U64_ARRAY_SIZE<<endl;

	cout<<"MAXIMUM_MESSAGE_SIZE_IN_BYTES: "<<MAXIMUM_MESSAGE_SIZE_IN_BYTES<<" bytes"<<endl;
	cout<<"FORCE_PACKING (don't align addresses on 8 bytes): ";

	#ifdef FORCE_PACKING
	cout<<"y";
	#else
	cout<<"n";
	#endif
	cout<<endl;

	cout<<"ASSERT: ";
	#ifdef ASSERT
	cout<<"y"<<endl;
	#else
	cout<<"n"<<endl;
	#endif

	cout<<"HAVE_CLOCK_GETTIME (real-time Operating System): ";
	#ifdef HAVE_CLOCK_GETTIME
	cout<<"y"<<endl;
	#else
	cout<<"n"<<endl;
	#endif

	// show libraries
	cout<<"HAVE_LIBZ (gz support): ";
	#ifdef HAVE_LIBZ
	cout<<"y"<<endl;
	#else
	cout<<"n"<<endl;
	#endif

	cout<<"HAVE_LIBBZ2 (bz2 support): ";
	#ifdef HAVE_LIBBZ2
	cout<<"y"<<endl;
	#else
	cout<<"n"<<endl;
	#endif

}

void showRayVersion(MessagesHandler*messagesHandler,bool fullReport){
	showRayVersionShort();

	cout<<endl;
	cout<<"Rank "<<MASTER_RANK<<": Operating System: ";
	cout<<getOperatingSystem()<<endl;

	// show OS, only Linux
	cout<<"Linux (__linux__): ";
	#ifdef __linux__
	cout<<"y"<<endl;
	#else
	cout<<"n"<<endl;
	#endif


	cout<<"GNU system (__GNUC__): ";
	#ifdef __GNUC__
	cout<<"y"<<endl;
	#else
	cout<<"n"<<endl;
	#endif


	cout<<"Message-passing interface"<<endl;
	cout<<endl;
	cout<<"Rank "<<MASTER_RANK<<": Message-Passing Interface implementation: ";
	cout<<messagesHandler->getMessagePassingInterfaceImplementation()<<endl;

	int version;
	int subversion;
	messagesHandler->version(&version,&subversion);

	cout<<"Rank "<<MASTER_RANK<<": Message-Passing Interface standard version: "<<version<<"."<<subversion<<""<<endl;


	cout<<endl;

	#define SHOW_SIZEOF

	if(!fullReport)
		return;

	#ifdef SHOW_SIZEOF
	cout<<"Size of structures"<<endl;
	cout<<endl;
	cout<<"KMER_BYTES "<<KMER_BYTES<<endl;
	cout<<"KMER_UINT64_T "<<KMER_UINT64_T<<endl;
	cout<<"KMER_UINT64_T_MODULO "<<KMER_UINT64_T_MODULO<<endl;

	cout<<" sizeof(Vertex)="<<sizeof(Vertex)<<endl;
	cout<<" sizeof(Direction)="<<sizeof(Direction)<<endl;
	cout<<" sizeof(ReadAnnotation)="<<sizeof(ReadAnnotation)<<endl;
	cout<<" sizeof(Read)="<<sizeof(Read)<<endl;
	cout<<" sizeof(PairedRead)="<<sizeof(PairedRead)<<endl;
	#endif

	cout<<endl;

	cout<<"Number of MPI ranks: "<<messagesHandler->getSize()<<endl;
	cout<<"Ray master MPI rank: "<<MASTER_RANK<<endl;
	cout<<"Ray slave MPI ranks: 0-"<<messagesHandler->getSize()-1<<endl;
	cout<<endl;


	#ifdef SHOW_ITEMS
	int count=0;
	#define MACRO_LIST_ITEM(x) count++;
	#include <core/master_mode_macros.h>
	#undef MACRO_LIST_ITEM
	cout<<"Ray master modes ( "<<count<<" )"<<endl;
	#define MACRO_LIST_ITEM(x) printf(" %i %s\n",x,#x);fflush(stdout);
	#include <core/master_mode_macros.h>
	#undef MACRO_LIST_ITEM
	cout<<endl;
	count=0;
	#define MACRO_LIST_ITEM(x) count++;
	#include <core/slave_mode_macros.h>
	#undef MACRO_LIST_ITEM
	cout<<"Ray slave modes ( "<<count<<" )"<<endl;
	#define MACRO_LIST_ITEM(x) printf(" %i %s\n",x,#x);fflush(stdout);
	#include <core/slave_mode_macros.h>
	#undef MACRO_LIST_ITEM
	cout<<endl;
	count=0;
	#define MACRO_LIST_ITEM(x) count++;
	#include <communication/mpi_tag_macros.h>
	#undef MACRO_LIST_ITEM
	cout<<"Ray MPI tags ( "<<count<<" )"<<endl;
	#define MACRO_LIST_ITEM(x) printf(" %i %s\n",x,#x);fflush(stdout);
	#include <communication/mpi_tag_macros.h>
	#undef MACRO_LIST_ITEM
	#endif
	cout<<endl;
}
