/*
 	Ray
    Copyright (C) 2011  Sébastien Boisvert

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

#ifndef _GridTableIterator
#define _GridTableIterator

#include <application_core/Parameters.h>
#include <application_core/common_functions.h>
#include <structures/MyHashTableIterator.h>
#include <plugin_VerticesExtractor/GridTable.h>

/**
 * \author Sébastien Boisvert
 */
class GridTableIterator{
	MyHashTableIterator<Kmer,Vertex> m_iterator;
	bool m_mustProcessOtherKey;
	Kmer m_currentKey;
	Vertex*m_currentEntry;
	Parameters*m_parameters;
public:
	void constructor(GridTable*a,int wordSize,Parameters*b);
	bool hasNext();
	Vertex*next();
	Kmer*getKey();
};

#endif
