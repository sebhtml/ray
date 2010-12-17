/*
 	Ray
    Copyright (C)  2010  Sébastien Boisvert

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


#ifndef _CoverageDistribution
#define _CoverageDistribution

#include<map>
#include<string>
#include<stdint.h>
#include<common_functions.h>
using namespace std;

/*
 *
 * Given a distribution of coverage, CoverageDistribution computes 
 * the minimum coverage, and the peak coverage.
 */
class CoverageDistribution{
	int m_minimumCoverage;
	int m_peakCoverage;
public:
	void writeFile(map<int,uint64_t>*distributionOfCoverage,string*file);
	CoverageDistribution(map<int,uint64_t>*distributionOfCoverage,string*file);
	int getMinimumCoverage();
	int getPeakCoverage();
};

#endif
