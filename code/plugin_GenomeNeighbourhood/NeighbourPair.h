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

#ifndef _NeighbourPair_h
#define _NeighbourPair_h

#include <stdint.h> /** for uint64_t **/
#include <application_core/constants.h>


/**
 * A folk in the genome neighbourhood.
 */
class NeighbourPair{

	PathHandle m_contigName1;
	Strand m_strand1;
	int m_progression1;

	PathHandle m_contigName2;
	Strand m_strand2;
	int m_progression2;

	int m_depth;

public:
	NeighbourPair(PathHandle contig1,Strand strand1,int progression1,PathHandle contig2,
		Strand strand2,int progression2,
		int depth);

	PathHandle getContig1();
	Strand getStrand1();
	int getProgression1();

	PathHandle getContig2();
	Strand getStrand2();
	int getProgression2();

	int getDepth();
};

#endif

