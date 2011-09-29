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

#ifndef _Profiler_H
#define _Profiler_H

#include <vector>
#include <stdint.h>
#include <string>
using namespace std;

class Profiler{
	vector<uint64_t> m_timePoints;
	vector<string> m_functions;
	vector<string> m_files;
	vector<int> m_lines;
public:
	void constructor();
	void resetStack();
	void printStack();
	void collect(const char*function,const char*file,int line);

};

/* if CONFIG_PROFILER_COLLECT is defined, MACRO_COLLECT_PROFILING_INFORMATION() will collect profiling information
 * if -run-profiler was provided
 * otherwise, nothing is collected even if -run-profiler is provided 
 * the default is to define CONFIG_PROFILER_COLLECT 
 * and enable the profiler with -run-profiler (default is to not enable it)
 *
 * m_runProfiler must be a boolean in the context where the MACRO_COLLECT_PROFILING_INFORMATION is written.
 * and m_profiler must be the instance of the Profiler class and it must be a pointer to it.
 * */

#ifdef CONFIG_PROFILER_COLLECT

#define MACRO_COLLECT_PROFILING_INFORMATION() \
	if(m_runProfiler && m_profiler != NULL) \
		m_profiler->collect(__func__,__FILE__,__LINE__);

#else
#define MACRO_COLLECT_PROFILING_INFORMATION()
#endif


#endif
