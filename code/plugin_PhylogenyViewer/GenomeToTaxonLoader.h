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

#ifndef _GenomeToTaxonLoader_h
#define _GenomeToTaxonLoader_h


#include <application_core/constants.h>
#include <core/types.h>
#include <plugin_PhylogenyViewer/types.h>

#include <string>
#include <stdint.h>
#include <fstream>
using namespace std;

/**
 * A loader for genome-to-taxon entries.
 *
 * \author Sébastien Boisvert
 */
class GenomeToTaxonLoader{
	
	ifstream m_stream;

	int STEPPING;

	LargeCount m_size;
	LargeIndex m_current;

public:
	void load(string file);
	bool hasNext();
	void getNext(GenomeIdentifier*genome,TaxonIdentifier*taxon);
};

#endif
