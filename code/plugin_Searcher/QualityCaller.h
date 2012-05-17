/*
 	Ray
    Copyright (C) 2010, 2011, 2012 Sébastien Boisvert

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

#ifndef _QualityCaller_h
#define _QualityCaller_h

#include <application_core/constants.h>
#include <core/types.h>
#include <stdint.h>
#include <vector>
#include <map>
using namespace std;

/** a simple algorithm that says if  something is a false positive or not
 */
class QualityCaller{

public:
	double computeCorrelation(vector<int>*x,vector<int>*y);
	double computeQuality(map<CoverageDepth,LargeCount>*array1,map<CoverageDepth,LargeCount>*array2);

};

#endif
