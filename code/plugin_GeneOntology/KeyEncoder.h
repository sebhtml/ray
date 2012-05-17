/*
 	Ray
    Copyright (C) 2012  Sébastien Boisvert

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

#ifndef _KeyEncoder

#define _KeyEncoder

#include <map>
#include <stdint.h>
#include <plugin_GeneOntology/types.h>
#include <plugin_Searcher/VirtualKmerColor.h>
using namespace std;

class KeyEncoder{

	map<char,int> m_mapping;

	void populateMap();

	PhysicalKmerColor encode_EMBL_CDS(const char*identifier);
public:

	PhysicalKmerColor getEncoded_EMBL_CDS(const char*identifier);

	GeneOntologyIdentifier /**/ encodeGeneOntologyHandle(const char*identifier);
};

#endif

