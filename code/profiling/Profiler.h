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

extern const char* PROFILE_SYMBOLS[];

#define MACRO_LIST_ITEM(element) element,

/* for profiler */
enum{
#include <profiling/profiling_macros.h>
PROFILER_NUMBER_OF_OBSERVERS
};

#undef MACRO_LIST_ITEM

class Profiler{
	bool m_hasSomething;
	int m_profile[PROFILER_NUMBER_OF_OBSERVERS];
public:
	void constructor();
	void reset();
	void collect(int symbol);
	void print();
};

#endif

