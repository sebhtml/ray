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

#ifndef _DistributionWriter_h
#define _DistributionWriter_h

#include <application_core/constants.h>
#include <core/types.h> /* for Rank */

#include <map>
#include <stdint.h>
#include <fstream>
#include <string>
#include <sstream>
using namespace std;

/**
 *
 * this class writes xml files for frequencies observed.
 */
class DistributionWriter{
	bool m_gotFile;
	string m_base;
	Rank m_rank;
	ofstream m_output;
	ostringstream m_output_Buffer;
	int m_operations;

	void openFile();


public:
	DistributionWriter();
	void setBase(const char*base);
	void setRank(Rank rank);

	void write(int directory,int file,int sequence,
		map<CoverageDepth,LargeCount>*all,map<CoverageDepth,LargeCount>*uniquelyColored,map<CoverageDepth,LargeCount>*uniquelyColoredAndAssembled,
	const char*directoryName,const char*fileName);

	void close();
};

#endif
