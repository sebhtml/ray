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

#ifndef _Profiler_H
#define _Profiler_H

#include <vector>
#include <stdint.h>
#include <string>
#include <map>
using namespace std;

/**
 * when enabled, this profiler collects data and 
 * reports some things.
 * To collect something somewhere, add the line
 *
 * MACRO_COLLECT_PROFILING_INFORMATION();
 *
 * Also, the code (your application and RayPlatform) 
 * must be compiled with -D CONFIG_PROFILER_COLLECT
 *
 * To compile the profiling code.
 */
class Profiler{
	vector<uint64_t> m_timePoints;
	vector<string> m_functions;
	vector<string> m_files;
	vector<int> m_lines;

	bool m_enabled;

	int m_threshold;

	map<int,int> m_granularityValues;

	/* store the granularities for each slave mode */
	map<int,map<int,uint64_t> > m_observedGranularities;

public:
	void constructor(bool enabled);
	void resetStack();
	void printStack();

/** collect information */
	void collect(const char*function,const char*file,int line);

	bool isEnabled();
	int getThreshold();

/** add information about a task */
	void addGranularity(int mode,int microseconds);

	void printGranularities(int rank);
	void printAllGranularities();

	void clearGranularities();
};

/* if CONFIG_PROFILER_COLLECT is defined, MACRO_COLLECT_PROFILING_INFORMATION() will collect profiling information
 * if -run-profiler was provided
 * otherwise, nothing is collected even if -run-profiler is provided 
 * the default is to define CONFIG_PROFILER_COLLECT 
 * and enable the profiler with -run-profiler (default is to not enable it)
 *
 * m_profiler must be a boolean in the context where the MACRO_COLLECT_PROFILING_INFORMATION is written.
 * and m_profiler must be the instance of the Profiler class and it must be a pointer to it.
 * */

#ifdef CONFIG_PROFILER_COLLECT

#define MACRO_COLLECT_PROFILING_INFORMATION() \
	if(m_profiler != NULL && m_profiler->isEnabled()) \
		m_profiler->collect(__func__,__FILE__,__LINE__);

#else
#define MACRO_COLLECT_PROFILING_INFORMATION()
#endif


#endif
