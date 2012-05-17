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
#include <application_core/constants.h>


/**
 * A folk in the genome neighbourhood.
 */
class Neighbour{

	Strand m_strand;
	int m_depth;
	PathHandle m_contigName;
	int m_progression;

public:
	Neighbour(Strand dnaStrand,int depth,PathHandle contig,int progression);

	Strand getStrand();
	int getDepth();
	PathHandle getContig();
	int getProgression();
};

#endif

