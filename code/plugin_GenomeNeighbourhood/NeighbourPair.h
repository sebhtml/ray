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


/**
 * A folk in the genome neighbourhood.
 */
class NeighbourPair{

	uint64_t m_contigName1;
	char m_strand1;
	int m_progression1;

	uint64_t m_contigName2;
	char m_strand2;
	int m_progression2;

	int m_depth;

public:
	NeighbourPair(uint64_t contig1,char strand1,int progression1,uint64_t contig2,char strand2,int progression2,
		int depth);

	uint64_t getContig1();
	char getStrand1();
	int getProgression1();

	uint64_t getContig2();
	char getStrand2();
	int getProgression2();

	int getDepth();
};

#endif

