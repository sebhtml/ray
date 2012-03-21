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

#ifndef _Neighbour_h
#define _Neighbour_h

#include <stdint.h> /** for uint64_t **/


/**
 * A folk in the genome neighbourhood.
 */
class Neighbour{

	char m_strand;
	int m_depth;
	uint64_t m_contigName;
	int m_progression;

public:
	Neighbour(char dnaStrand,int depth,uint64_t contig,int progression);

	char getStrand();
	int getDepth();
	uint64_t getContig();
	int getProgression();
};

#endif

