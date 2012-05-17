/*
 	Ray
    Copyright (C) 2012 Sébastien Boisvert

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

#include <application_core/constants.h>
#include <core/types.h>

#include <stdint.h> /* for uint64_t */
#include <fstream>
using namespace std;

#ifndef _ContigSearchEntry_h
#define _ContigSearchEntry_h

class ContigSearchEntry{
	PathHandle m_name;
	int m_length;
	int m_modeCoverage;
	double m_meanCoverage;
	int m_coloredKmers;

public:
	ContigSearchEntry(PathHandle name,int length,int mode,double mean,int coloredKmers);

	PathHandle getName();
	int getLength();
	int getMode();
	double getMean();
	int getTotal();

	void write(ofstream*f,LargeCount total,int kmerLength);
};

#endif
