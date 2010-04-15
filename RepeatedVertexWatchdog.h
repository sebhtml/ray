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
    along with this program (gpl-3.0.txt).  
	see <http://www.gnu.org/licenses/>

*/

#ifndef _RepeatedVertexWatchdog
#define _RepeatedVertexWatchdog

#include<ExtensionData.h>

class RepeatedVertexWatchdog{
public:
	bool getApproval(ExtensionData*ed,int wordSize,int minimumCoverage,int maxCoverage,
		u64 currentVertex);
};

#endif

