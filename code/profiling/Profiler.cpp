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

#include <profiling/Profiler.h>
#include <core/slave_modes.h>
#include <assert.h>
#include <iostream>
using namespace std;

#define MACRO_LIST_ITEM(x) #x,

const char* PROFILE_SYMBOLS[]={
#include <profiling/profiling_macros.h>
"PROFILER_NUMBER_OF_OBSERVERS"
};

#undef MACRO_LIST_ITEM

void Profiler::constructor(){
	reset();
}

void Profiler::reset(){
	for(int i=0;i<PROFILER_NUMBER_OF_OBSERVERS;i++){
		m_profile[i]=0;
	}
	m_hasSomething=false;
}

void Profiler::collect(int symbol){
	#ifdef ASSERT
	assert(symbol < PROFILER_NUMBER_OF_OBSERVERS);
	#endif
	m_profile[symbol] ++;

	m_hasSomething=true;
}

void Profiler::print(){
	if(!m_hasSomething)
		return;

	for(int i=0;i<PROFILER_NUMBER_OF_OBSERVERS;i++){
		int count=m_profile[i];
		if(count==0)
			continue;

		cout<<PROFILE_SYMBOLS[i]<<" "<<count<<endl;
	}
}
