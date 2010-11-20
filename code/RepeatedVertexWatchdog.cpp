/*
 	Ray
    Copyright (C) 2010  Sébastien Boisvert

	http://DeNovoAssembler.SourceForge.Net/

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You have received a copy of the GNU General Public License
    along with this program (LICENSE).  
	see <http://www.gnu.org/licenses/>

*/

#include<RepeatedVertexWatchdog.h>
#include<iostream>
#ifdef ASSERT
#include<assert.h>
#endif
using namespace std;

bool RepeatedVertexWatchdog::getApproval(ExtensionData*ed,int wordSize,int minimumCoverage,int maxCoverage,
		u64 currentVertex){
	if(ed->m_currentCoverage==maxCoverage and
	(int)ed->m_EXTENSION_readsInRange.size()<minimumCoverage/2){


		#ifdef SHOW_REPEATED_VERTEX_WATCHDOG
		cout<<"Watchdog says: "<<idToWord(currentVertex,wordSize)<<" is a repeated region for sure!, probably a transposase if they exist in the genome. (VertexCoverage="<<ed->m_currentCoverage<<", MaxCoverage="<<maxCoverage<<" ReadsInRange="<<ed->m_EXTENSION_readsInRange.size()<<", MinimumCoverage="<<minimumCoverage<<")"<<endl;
		#endif
		#ifdef ASSERT
		assert(ed->m_currentCoverage==maxCoverage);
		assert((int)ed->m_EXTENSION_readsInRange.size()<minimumCoverage);
		#endif
		return false;
	}
	return true;
}
